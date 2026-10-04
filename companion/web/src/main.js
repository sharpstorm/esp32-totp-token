import jsQR from "jsqr";

/* =====================================================================
 * Protocol constants (docs/BLE_PROTOCOL.md, protocol v2)
 * ===================================================================== */
const SERVICE_UUID = "4f545000-7837-4c91-82c3-4f1f5cb0f982";
const TX_UUID = "4f545001-7837-4c91-82c3-4f1f5cb0f982";
const RX_UUID = "4f545002-7837-4c91-82c3-4f1f5cb0f982";
const PROTO_VERSION = 0x02, DIR_C2D = 0x01, DIR_D2C = 0x02;
const IV_LEN = 12, TAG_LEN = 16, MAX_PLAINTEXT = 256;
const MIN_PACKET = IV_LEN + 1 + TAG_LEN; // shorter RX values are keepalives
const Cmd = { LIST: 1, GET: 2, DELETE: 3, PUT: 4, SET_TIME: 5, GET_TIME: 6, PING: 7, UPDATE: 8,
  WIFI_GET: 9, WIFI_SET: 10, WIFI_SCAN: 11, WIFI_RESULTS: 12 };
const OK = 0x00;
const LIMITS = { NAME_MAX: 255, PUT_SUM: 253, UPDATE_SUM: 252, SSID_MAX: 32, PASS_MIN: 8, PASS_MAX: 63 };
const TIMEOUT = { DEFAULT: 5000, PING: 4000, WIFI_SCAN: 15000 };

const enc = new TextEncoder();
const dec = new TextDecoder();
const utf8Len = (s) => enc.encode(s).length;
const concat = (...parts) => { const out = new Uint8Array(parts.reduce((n, p) => n + p.length, 0)); let o = 0; for (const p of parts) { out.set(p, o); o += p.length; } return out; };
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

class AppError extends Error { constructor(msg, kind) { super(msg); this.kind = kind; } }

/* =====================================================================
 * Base32
 * ===================================================================== */
const B32 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
function b32Encode(bytes) {
  let out = "", bits = 0, val = 0;
  for (const b of bytes) { val = (val << 8) | b; bits += 8; while (bits >= 5) { out += B32[(val >>> (bits - 5)) & 31]; bits -= 5; } val &= (1 << bits) - 1; }
  if (bits > 0) out += B32[(val << (5 - bits)) & 31];
  return out;
}
function b32Decode(str) {
  const out = []; let bits = 0, val = 0;
  for (const ch of str) { const i = B32.indexOf(ch); if (i < 0) throw new AppError("bad base32", "data"); val = (val << 5) | i; bits += 5; if (bits >= 8) { out.push((val >>> (bits - 8)) & 255); bits -= 8; val &= (1 << bits) - 1; } }
  return new Uint8Array(out);
}
/** Normalise and strictly validate; returns {ok, value|error}. */
function checkSecret(input) {
  const v = String(input || "").replace(/[\s \-=]/g, "").toUpperCase();
  if (!v) return { ok: false, error: "Enter the secret key." };
  if (!/^[A-Z2-7]+$/.test(v)) return { ok: false, error: "Only letters A–Z and digits 2–7 are allowed." };
  const rem = v.length % 8;
  if (rem === 1 || rem === 3 || rem === 6) return { ok: false, error: "This key looks incomplete. Check you copied all of it." };
  const bytes = Math.floor(v.length * 5 / 8);
  if (bytes < 1) return { ok: false, error: "This key is too short." };
  if (bytes > 255) return { ok: false, error: "This key is too long for the token." };
  return { ok: true, value: v };
}
function checkName(name) {
  const n = utf8Len(name);
  if (!name.trim()) return "Enter a name.";
  if (n > LIMITS.NAME_MAX) return `Too long (${n} of ${LIMITS.NAME_MAX} bytes).`;
  return null;
}
function checkBudget(name, b32, mode) {
  const max = mode === "put" ? LIMITS.PUT_SUM : LIMITS.UPDATE_SUM;
  const n = utf8Len(name) + b32.length;
  return n > max ? `Name and key together are too long for the token (${n} of ${max}). Shorten the name.` : null;
}

/* =====================================================================
 * AES-128-GCM channel (mirrors src/BLE/AesCrypto.cpp)
 *   packet = IV(12) || ct || tag(16); IV = rand(8) || u32be counter
 *   AAD = [0x02, direction]
 * ===================================================================== */
class Channel {
  static async create(rawKey, sendDir, recvDir) {
    const c = new Channel();
    c.key = await crypto.subtle.importKey("raw", rawKey, "AES-GCM", false, ["encrypt", "decrypt"]);
    c.sendAad = new Uint8Array([PROTO_VERSION, sendDir]);
    c.recvAad = new Uint8Array([PROTO_VERSION, recvDir]);
    c.tx = 0; c.lastRx = -1;
    return c;
  }
  async seal(pt, prefix) {
    if (pt.length < 1 || pt.length > MAX_PLAINTEXT) throw new AppError("Command too long.", "format");
    if (this.tx >= 0xffffffff) throw new AppError("Reconnect to continue.", "counter");
    const counter = this.tx++;
    const iv = new Uint8Array(IV_LEN);
    iv.set(prefix || crypto.getRandomValues(new Uint8Array(8)));
    new DataView(iv.buffer).setUint32(8, counter);
    const ct = new Uint8Array(await crypto.subtle.encrypt({ name: "AES-GCM", iv, additionalData: this.sendAad, tagLength: 128 }, this.key, pt));
    return concat(iv, ct);
  }
  async open(pkt) {
    if (pkt.length < MIN_PACKET) throw new AppError("short", "format");
    const iv = pkt.slice(0, IV_LEN);
    const counter = new DataView(iv.buffer).getUint32(8);
    if (counter <= this.lastRx) throw new AppError("replay", "replay");
    let pt;
    try { pt = await crypto.subtle.decrypt({ name: "AES-GCM", iv, additionalData: this.recvAad, tagLength: 128 }, this.key, pkt.slice(IV_LEN)); }
    catch { throw new AppError("auth", "auth"); }
    if (counter <= this.lastRx) throw new AppError("replay", "replay");
    this.lastRx = counter;
    return new Uint8Array(pt);
  }
}
function parseKeyHex(input) {
  const v = String(input || "").replace(/[\s\-:]/g, "");
  if (!/^[0-9a-fA-F]{32}$/.test(v)) return null;
  const out = new Uint8Array(16);
  for (let i = 0; i < 16; i++) out[i] = parseInt(v.substr(i * 2, 2), 16);
  return out;
}

/* =====================================================================
 * Command encoding / decoding
 * ===================================================================== */
const P = {
  list: (start) => start ? new Uint8Array([Cmd.LIST, start]) : new Uint8Array([Cmd.LIST]),
  get: (i) => new Uint8Array([Cmd.GET, i]),
  del: (i) => new Uint8Array([Cmd.DELETE, i]),
  put(name, b32) { const n = enc.encode(name), s = enc.encode(b32); return concat(new Uint8Array([Cmd.PUT, n.length, s.length]), n, s); },
  update(i, name, b32) { const n = enc.encode(name), s = enc.encode(b32 || ""); return concat(new Uint8Array([Cmd.UPDATE, i, n.length, s.length]), n, s); },
  setTime(unix) { const b = new Uint8Array(5); b[0] = Cmd.SET_TIME; new DataView(b.buffer).setUint32(1, unix); return b; },
  getTime: () => new Uint8Array([Cmd.GET_TIME]),
  ping: () => new Uint8Array([Cmd.PING]),
  wifiGet: () => new Uint8Array([Cmd.WIFI_GET]),
  wifiSet(ssid, pass) { const s = enc.encode(ssid), p = enc.encode(pass); return concat(new Uint8Array([Cmd.WIFI_SET, s.length, p.length]), s, p); },
  wifiScan: () => new Uint8Array([Cmd.WIFI_SCAN]),
  wifiResults: (start) => new Uint8Array([Cmd.WIFI_RESULTS, start]),
};
const protoErr = () => new AppError("The token sent an unexpected reply.", "protocol");
function readStatus(p) { if (p.length < 2) throw protoErr(); if (p[1] !== OK) throw new AppError("The token rejected the request.", "fail"); }
function isFail(p) { return p.length === 2 && p[1] === 0x01; }
function readPage(p, parseEntry) {
  if (isFail(p)) throw new AppError("The token couldn't send this list.", "fail");
  if (p.length < 4) throw protoErr();
  const total = p[1], start = p[2], n = p[3], items = [];
  let pos = 4;
  for (let i = 0; i < n; i++) { const r = parseEntry(p, pos); if (!r) throw protoErr(); items.push(r.item); pos = r.pos; }
  return { total, start, items };
}
const nameEntry = (p, pos) => { if (pos >= p.length) return null; const len = p[pos++]; if (pos + len > p.length) return null; return { item: dec.decode(p.subarray(pos, pos + len)), pos: pos + len }; };
const netEntry = (p, pos) => { if (pos + 3 > p.length) return null; const rssi = (p[pos] << 24) >> 24, auth = p[pos + 1], len = p[pos + 2]; pos += 3; if (pos + len > p.length) return null; return { item: { rssi, auth, ssid: dec.decode(p.subarray(pos, pos + len)) }, pos: pos + len }; };
function readGet(p) {
  if (p.length < 4 || p[1] !== OK) throw new AppError("The token couldn't read this account.", "fail");
  let pos = 2; const nl = p[pos++]; if (pos + nl + 1 > p.length) throw protoErr();
  const name = dec.decode(p.subarray(pos, pos + nl)); pos += nl;
  const sl = p[pos++]; if (pos + sl > p.length) throw protoErr();
  return { name, secret: dec.decode(p.subarray(pos, pos + sl)) };
}
async function allPages(fetchPage) {
  const items = []; let start = 0, total = 0;
  for (let guard = 0; guard < 300; guard++) {
    const r = await fetchPage(start);
    total = r.total;
    if (r.start !== start) throw protoErr();
    if (r.items.length === 0) break;
    items.push(...r.items); start += r.items.length;
    if (start >= total) break;
  }
  return { items, total, incomplete: items.length < total };
}

/* =====================================================================
 * BLE client: GATT, one-at-a-time request queue, timeouts
 * ===================================================================== */
const client = {
  state: "disconnected", device: null, tx: null, rx: null, channel: null, keyHex: null,
  epoch: 0, tail: Promise.resolve(), pending: null, rxChain: Promise.resolve(), inFlight: 0,
  listeners: new Set(),
  on(fn) { this.listeners.add(fn); },
  emit(ev) { for (const fn of this.listeners) { try { fn(ev); } catch (e) { console.error(e); } } },
  setState(s, reason) { this.state = s; this.emit({ type: "state", state: s, reason }); },

  async connect(reuse) {
    if (this.state !== "disconnected") return;
    this.setState("connecting");
    try {
      let device = reuse ? this.device : null;
      if (!device) {
        try { device = await navigator.bluetooth.requestDevice({ filters: [{ services: [SERVICE_UUID] }] }); }
        catch (e) { if (e && e.name === "NotFoundError") throw new AppError("", "cancelled"); throw e; }
      }
      if (this.device && this.device !== device) this.device.removeEventListener("gattserverdisconnected", this.onGattDown);
      this.device = device;
      device.removeEventListener("gattserverdisconnected", this.onGattDown);
      device.addEventListener("gattserverdisconnected", this.onGattDown);
      const server = await device.gatt.connect();
      const service = await server.getPrimaryService(SERVICE_UUID);
      this.tx = await service.getCharacteristic(TX_UUID);
      this.rx = await service.getCharacteristic(RX_UUID);
      // Reading a MITM-protected attribute makes the OS pair now (PIN prompt),
      // which is when the token shows its key.
      try { await this.rx.readValue(); }
      catch (e) {
        if (this.state === "disconnected") throw new AppError("The token disconnected.", "down");
        if (e && (e.name === "NetworkError" || e.name === "NotAllowedError" || e.name === "SecurityError"))
          throw new AppError("Pairing failed or was cancelled. Reconnect and enter the PIN shown on the token.", "pair");
      }
      if (this.state === "disconnected") throw new AppError("The token disconnected.", "down");
      this.setState("awaiting-key");
    } catch (e) {
      try { this.device && this.device.gatt && this.device.gatt.disconnect(); } catch {}
      this.reset();
      this.setState("disconnected", e.kind === "cancelled" ? undefined : msg(e));
      if (e.kind !== "cancelled") throw e;
    }
  },

  async submitKey(input) {
    const raw = parseKeyHex(input);
    if (!raw) throw new AppError("The key is 32 characters: digits 0–9 and letters A–F.", "key");
    const hex = input.replace(/[\s\-:]/g, "").toUpperCase();
    // A retry with the same key keeps the counters: the token may already
    // have accepted counter 0 even though its reply was lost.
    if (!this.channel || this.keyHex !== hex) { this.channel = await Channel.create(raw, DIR_C2D, DIR_D2C); this.keyHex = hex; }
    this.setState("verifying");
    try {
      if (!this.notifying) {
        this.rx.addEventListener("characteristicvaluechanged", this.onNotify);
        await this.rx.startNotifications();
        this.notifying = true;
      }
      readStatus(await this.request(P.ping(), TIMEOUT.PING));
    } catch (e) {
      if (this.state === "verifying") this.setState("awaiting-key");
      if (e.kind === "timeout") throw new AppError("The token didn't respond. Check the key and try again.", "key");
      throw e;
    }
    if (this.state === "verifying") this.setState("ready");
  },

  disconnect() {
    try { this.device && this.device.gatt && this.device.gatt.disconnect(); } catch {}
    this.down("Disconnected.");
  },
  onGattDown: () => client.down("The token disconnected."),
  down(reason) {
    if (this.state === "disconnected" && !this.rx) return;
    this.reset();
    this.setState("disconnected", reason);
  },
  reset() {
    this.epoch++;
    if (this.rx) this.rx.removeEventListener("characteristicvaluechanged", this.onNotify);
    this.rx = this.tx = this.channel = this.keyHex = null; this.notifying = false;
    this.rxChain = Promise.resolve();
    const p = this.pending; this.pending = null;
    if (p) { clearTimeout(p.timer); p.reject(new AppError("The token disconnected.", "down")); }
  },

  onNotify: (ev) => {
    const dv = ev.target.value; if (!dv) return;
    const bytes = new Uint8Array(dv.buffer.slice(dv.byteOffset, dv.byteOffset + dv.byteLength));
    if (bytes.length < MIN_PACKET) return; // keepalive
    const epoch = client.epoch;
    client.rxChain = client.rxChain.then(() => client.handlePacket(bytes, epoch));
  },
  async handlePacket(bytes, epoch) {
    if (!this.channel || epoch !== this.epoch) return;
    let pt; try { pt = await this.channel.open(bytes); } catch { return; } // forged/replayed: drop
    const p = this.pending;
    if (epoch === this.epoch && p && pt[0] === p.cmd) { this.pending = null; clearTimeout(p.timer); p.resolve(pt); }
  },

  request(pt, timeoutMs = TIMEOUT.DEFAULT) {
    const epoch = this.epoch;
    if (++this.inFlight === 1) this.emit({ type: "busy", busy: true });
    const run = async () => {
      try { return await this.exec(pt, timeoutMs, epoch); }
      finally { if (--this.inFlight === 0) this.emit({ type: "busy", busy: false }); }
    };
    const result = this.tail.then(run, run);
    this.tail = result.then(() => {}, () => {});
    return result;
  },
  async exec(pt, timeoutMs, epoch) {
    const live = () => epoch === this.epoch && this.channel && this.tx;
    if (!live()) throw new AppError("Not connected.", "down");
    await sleep(20); // let the token's main loop clear its command slot
    const pkt = await this.channel.seal(pt);
    if (!live()) throw new AppError("Not connected.", "down");
    const cmd = pt[0], tx = this.tx;
    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => { if (this.pending && this.pending.timer === timer) this.pending = null; reject(new AppError("The token didn't respond in time.", "timeout")); }, timeoutMs);
      this.pending = { cmd, resolve, reject, timer };
      tx.writeValueWithResponse(pkt).catch((e) => {
        if (this.pending && this.pending.timer === timer) { this.pending = null; clearTimeout(timer); reject(epoch !== this.epoch ? new AppError("The token disconnected.", "down") : new AppError(msg(e), "io")); }
      });
    });
  },

  // ---- high level ----
  async list() { return allPages(async (s) => readPage(await this.request(P.list(s)), nameEntry)); },
  async get(i) { return readGet(await this.request(P.get(i))); },
  async del(i) { readStatus(await this.request(P.del(i))); },
  async put(name, b32) { readStatus(await this.request(P.put(name, b32))); },
  async update(i, name, b32) { readStatus(await this.request(P.update(i, name, b32))); },
  async getTime() { const p = await this.request(P.getTime()); if (p.length < 5) throw protoErr(); return new DataView(p.buffer, p.byteOffset).getUint32(1); },
  async setTime(unix) { readStatus(await this.request(P.setTime(unix))); },
  async wifiGet() { const p = await this.request(P.wifiGet()); if (p.length < 4 || p[1] !== OK) throw protoErr(); return { configured: p[2] === 1, ssid: p[2] === 1 ? dec.decode(p.subarray(4, 4 + p[3])) : "" }; },
  async wifiSet(ssid, pass) { readStatus(await this.request(P.wifiSet(ssid, pass))); },
  async wifiScan() {
    const p = await this.request(P.wifiScan(), TIMEOUT.WIFI_SCAN);
    if (p.length < 3 || p[1] !== OK) throw new AppError("The token's Wi-Fi scan failed.", "fail");
    if (p[2] === 0) return { items: [], total: 0 };
    return allPages(async (s) => readPage(await this.request(P.wifiResults(s)), netEntry));
  },
};

/* =====================================================================
 * otpauth:// and Google Authenticator (otpauth-migration://) parsing
 * ===================================================================== */
function unsupportedReason({ type, algorithm, digits, period }) {
  if (type === "hotp") return "Counter-based (HOTP) codes aren't supported. The token only does time-based codes.";
  if (type !== "totp") return "Unknown code type. The token only does time-based codes.";
  if (algorithm !== "SHA1") return `Uses ${algorithm}. The token only supports SHA-1.`;
  if (digits !== 6) return `Uses ${digits}-digit codes. The token only shows 6 digits.`;
  if (period !== 30) return `Uses a ${period}-second period. The token only supports 30 seconds.`;
  return null;
}
function composeName(issuer, account) {
  issuer = (issuer || "").trim(); account = (account || "").trim();
  if (!issuer) return account || "Unnamed";
  if (!account) return issuer;
  if (account.toLowerCase().startsWith(issuer.toLowerCase() + ":")) account = account.slice(issuer.length + 1).trim();
  if (account.toLowerCase() === issuer.toLowerCase()) return issuer;
  return `${issuer}: ${account}`;
}
function parseOtpauth(url) {
  const u = new URL(url);
  const type = u.host.toLowerCase();
  const label = decodeURIComponent(u.pathname.replace(/^\/+/, ""));
  const q = u.searchParams;
  let issuer = q.get("issuer") || "", account = label;
  const colon = label.indexOf(":");
  if (colon >= 0) { if (!issuer) issuer = label.slice(0, colon); account = label.slice(colon + 1); }
  return [{
    name: composeName(issuer, account),
    secret: q.get("secret") || "",
    type, algorithm: (q.get("algorithm") || "SHA1").toUpperCase().replace("-", ""),
    digits: parseInt(q.get("digits") || "6", 10), period: parseInt(q.get("period") || "30", 10),
  }];
}
// Minimal protobuf reader for Google Authenticator's MigrationPayload.
function pbReader(buf) {
  let pos = 0;
  const varint = () => { let res = 0, mul = 1, b; do { if (pos >= buf.length) throw new AppError("Truncated export", "data"); b = buf[pos++]; res += (b & 0x7f) * mul; mul *= 128; } while (b & 0x80); return res; };
  const bytes = () => { const n = varint(); if (pos + n > buf.length) throw new AppError("Truncated export", "data"); const out = buf.subarray(pos, pos + n); pos += n; return out; };
  const skip = (wt) => { if (wt === 0) varint(); else if (wt === 2) bytes(); else if (wt === 1) pos += 8; else if (wt === 5) pos += 4; else throw new AppError("Unreadable export", "data"); };
  return { more: () => pos < buf.length, tag: () => { const t = varint(); return { f: Math.floor(t / 8), wt: t & 7 }; }, varint, bytes, skip };
}
const GA_ALG = { 0: "SHA1", 1: "SHA1", 2: "SHA256", 3: "SHA512", 4: "MD5" };
const GA_DIGITS = { 0: 6, 1: 6, 2: 8 };
function parseMigration(url) {
  const u = new URL(url);
  let data = u.searchParams.get("data");
  if (!data) throw new AppError("This export link has no data.", "data");
  data = data.replace(/-/g, "+").replace(/_/g, "/").replace(/\s/g, "");
  const bin = atob(data); const buf = Uint8Array.from(bin, (c) => c.charCodeAt(0));
  const r = pbReader(buf); const entries = []; const meta = {};
  while (r.more()) {
    const { f, wt } = r.tag();
    if (f === 1 && wt === 2) {
      const e = pbReader(r.bytes()); const o = { secret: new Uint8Array(0), name: "", issuer: "", alg: 0, digits: 0, type: 0 };
      while (e.more()) {
        const t = e.tag();
        if (t.f === 1 && t.wt === 2) o.secret = e.bytes();
        else if (t.f === 2 && t.wt === 2) o.name = dec.decode(e.bytes());
        else if (t.f === 3 && t.wt === 2) o.issuer = dec.decode(e.bytes());
        else if (t.f === 4 && t.wt === 0) o.alg = e.varint();
        else if (t.f === 5 && t.wt === 0) o.digits = e.varint();
        else if (t.f === 6 && t.wt === 0) o.type = e.varint();
        else e.skip(t.wt);
      }
      entries.push({
        name: composeName(o.issuer, o.name), secret: b32Encode(o.secret),
        type: o.type === 1 ? "hotp" : o.type === 2 ? "totp" : "unknown",
        algorithm: GA_ALG[o.alg] || "unknown", digits: GA_DIGITS[o.digits] || 0, period: 30,
      });
    } else if (wt === 0 && f >= 2 && f <= 5) {
      const v = r.varint(); meta[["", "", "version", "batchSize", "batchIndex", "batchId"][f]] = v;
    } else r.skip(wt);
  }
  return { entries, meta };
}
/** Returns {entries, meta?} or throws with a user-facing message. */
function parsePayload(text) {
  const t = String(text || "").trim();
  if (/^otpauth-migration:/i.test(t)) return parseMigration(t);
  if (/^otpauth:/i.test(t)) return { entries: parseOtpauth(t) };
  throw new AppError("That isn't an authenticator QR code or link.", "data");
}

/* =====================================================================
 * TOTP (for the "current code" preview after revealing a secret)
 * ===================================================================== */
async function totp(b32, t = Date.now()) {
  const key = await crypto.subtle.importKey("raw", b32Decode(b32), { name: "HMAC", hash: "SHA-1" }, false, ["sign"]);
  const counter = Math.floor(t / 1000 / 30);
  const msgBuf = new Uint8Array(8); new DataView(msgBuf.buffer).setUint32(4, counter >>> 0); new DataView(msgBuf.buffer).setUint32(0, Math.floor(counter / 2 ** 32));
  const h = new Uint8Array(await crypto.subtle.sign("HMAC", key, msgBuf));
  const off = h[19] & 15;
  const code = ((h[off] & 127) << 24 | h[off + 1] << 16 | h[off + 2] << 8 | h[off + 3]) % 1e6;
  return String(code).padStart(6, "0");
}

/* =====================================================================
 * UI helpers
 * ===================================================================== */
const $ = (id) => document.getElementById(id);
const msg = (e) => (e && e.message) || String(e);
function el(tag, props = {}, ...children) {
  const n = document.createElement(tag);
  for (const [k, v] of Object.entries(props)) {
    if (k === "class") n.className = v; else if (k === "text") n.textContent = v;
    else if (k.startsWith("on")) n.addEventListener(k.slice(2), v);
    else if (k === "dataset") Object.assign(n.dataset, v);
    else if (v !== false && v != null) n.setAttribute(k, v === true ? "" : v);
  }
  for (const c of children) if (c != null) n.append(c);
  return n;
}
const SVG_NS = "http://www.w3.org/2000/svg";
function icon(paths, size = 20) {
  const s = document.createElementNS(SVG_NS, "svg");
  s.setAttribute("width", size); s.setAttribute("height", size); s.setAttribute("viewBox", "0 0 24 24");
  s.setAttribute("fill", "none"); s.setAttribute("stroke", "currentColor"); s.setAttribute("stroke-width", "2");
  s.setAttribute("stroke-linecap", "round"); s.setAttribute("stroke-linejoin", "round"); s.setAttribute("aria-hidden", "true");
  for (const d of paths) { const p = document.createElementNS(SVG_NS, "path"); p.setAttribute("d", d); s.append(p); }
  return s;
}
const ICON_OK = ["M5 12l5 5L20 7"], ICON_ERR = ["M12 8v5", "M12 16h.01", "M10.3 3.9L2 18a2 2 0 0 0 1.7 3h16.6a2 2 0 0 0 1.7-3L13.7 3.9a2 2 0 0 0-3.4 0z"], ICON_INFO = ["M12 16v-4", "M12 8h.01"];

function toast(text, kind = "info", ms = 4200) {
  const t = el("div", { class: "toast", dataset: { kind } }, el("span", { class: "t-icon" }, icon(kind === "ok" ? ICON_OK : kind === "error" ? ICON_ERR : ICON_INFO, 18)), el("span", { text }));
  const box = $("toasts");
  while (box.children.length >= 2) box.firstElementChild.remove(); // keep the header readable
  box.append(t);
  setTimeout(() => t.remove(), ms);
}
async function withBusy(btn, label, fn) {
  const prev = btn ? [...btn.childNodes] : null;
  if (btn) { btn.disabled = true; btn.replaceChildren(el("span", { class: "spinner", "aria-hidden": "true" }), el("span", { text: label })); btn.setAttribute("aria-busy", "true"); }
  try { return await fn(); }
  finally { if (btn) { btn.disabled = false; btn.replaceChildren(...prev); btn.removeAttribute("aria-busy"); } }
}
function setFieldError(input, errEl, text) {
  errEl.textContent = text || "";
  if (text) input.setAttribute("aria-invalid", "true"); else input.removeAttribute("aria-invalid");
}
function avatarColor(name) {
  let h = 0; for (const c of name) h = (h * 31 + c.codePointAt(0)) >>> 0;
  return `hsl(${h % 360} 62% 64%)`;
}
function openDialog(d) { if (!d.open) d.showModal(); }
function closeDialog(d) { if (d.open) d.close(); }
document.addEventListener("click", (e) => {
  const c = e.target.closest("[data-close]"); if (c) closeDialog(c.closest("dialog"));
  const a = e.target.closest("[data-action='disconnect']"); if (a) client.disconnect();
  const pw = e.target.closest("[data-toggle-pw]");
  if (pw) { const inp = $(pw.dataset.togglePw); const show = inp.type === "password"; inp.type = show ? "text" : "password"; pw.setAttribute("aria-pressed", String(show)); pw.setAttribute("aria-label", show ? "Hide password" : "Show password"); }
});
// Tap on the backdrop closes sheets.
for (const d of document.querySelectorAll("dialog.sheet")) d.addEventListener("click", (e) => { if (e.target === d) closeDialog(d); });

function confirmDialog(title, text, okLabel) {
  const d = $("dlgConfirm");
  $("confirmTitle").textContent = title; $("confirmText").textContent = text; $("confirmYes").textContent = okLabel;
  return new Promise((resolve) => {
    const done = (v) => { cleanup(); closeDialog(d); resolve(v); };
    const yes = () => done(true), no = () => done(false), cancel = () => { cleanup(); resolve(false); };
    const cleanup = () => { $("confirmYes").removeEventListener("click", yes); $("confirmNo").removeEventListener("click", no); d.removeEventListener("cancel", cancel); };
    $("confirmYes").addEventListener("click", yes); $("confirmNo").addEventListener("click", no); d.addEventListener("cancel", cancel);
    openDialog(d); $("confirmNo").focus();
  });
}

/* =====================================================================
 * App state + views
 * ===================================================================== */
const state = { secrets: [], loadingSecrets: false, tab: "Secrets", editIndex: -1 };
const STATUS_TEXT = { disconnected: "Not connected", connecting: "Connecting…", "awaiting-key": "Paired · needs key", verifying: "Checking key…", ready: "Connected" };

function showView(name) {
  for (const v of ["Unsupported", "Connect", "Key", "Main"]) $("view" + v).hidden = v !== name;
  $("tabbar").hidden = name !== "Main";
}

client.on((ev) => {
  if (ev.type === "busy") { $("busyBar").hidden = !ev.busy; return; }
  const s = ev.state;
  $("statusPill").dataset.state = s; $("statusText").textContent = STATUS_TEXT[s];
  if (s === "disconnected") {
    for (const d of document.querySelectorAll("dialog")) closeDialog(d);
    stopScanner();
    showView("Connect");
    $("btnReconnect").hidden = !client.device;
    $("btnConnect").disabled = false;
    if (ev.reason) toast(ev.reason, ev.reason === "Disconnected." ? "info" : "error");
  } else if (s === "connecting") {
    $("btnConnect").disabled = true;
  } else if (s === "awaiting-key") {
    $("keyDeviceName").textContent = (client.device && client.device.name) || "TOTP-Token";
    if ($("viewKey").hidden) { $("keyInput").value = ""; updateKeyCount(); setFieldError($("keyInput"), $("keyErr"), ""); }
    showView("Key");
    $("keyInput").focus({ preventScroll: true });
  } else if (s === "ready") {
    $("deviceName").textContent = (client.device && client.device.name) || "TOTP-Token";
    showView("Main"); selectTab("Secrets");
    toast("Connected securely.", "ok");
    refreshSecrets();
  }
});

/* ---------- connect ---------- */
$("btnConnect").addEventListener("click", () => client.connect(false).catch(() => {}));
$("btnReconnect").addEventListener("click", () => client.connect(true).catch(() => {}));

/* ---------- key entry ---------- */
function updateKeyCount() { const n = $("keyInput").value.replace(/[\s\-:]/g, "").length; $("keyCount").textContent = `${n} / 32`; }
$("keyInput").addEventListener("input", () => { updateKeyCount(); setFieldError($("keyInput"), $("keyErr"), ""); });
async function submitKey(value) {
  const input = $("keyInput");
  if (!parseKeyHex(value)) { setFieldError(input, $("keyErr"), "The key is 32 characters: digits 0–9 and letters A–F."); input.focus(); return; }
  await withBusy($("btnSubmitKey"), "Checking…", async () => {
    try { await client.submitKey(value); }
    catch (e) { if (client.state !== "disconnected") setFieldError(input, $("keyErr"), msg(e)); }
  });
}
$("keyForm").addEventListener("submit", (e) => { e.preventDefault(); submitKey($("keyInput").value); });
$("btnScanKey").addEventListener("click", () => startScanner("key", "Scan the key on the token"));

/* ---------- tabs ---------- */
function selectTab(name) {
  state.tab = name;
  for (const b of document.querySelectorAll(".tab")) { const on = b.dataset.tab === name; b.setAttribute("aria-selected", String(on)); b.tabIndex = on ? 0 : -1; }
  for (const t of ["Secrets", "Wifi", "Device"]) $("tab" + t).hidden = t !== name;
  if (name === "Wifi") loadWifi();
  if (name === "Device") readTime();
}
$("tabbar").addEventListener("click", (e) => { const b = e.target.closest(".tab"); if (b) selectTab(b.dataset.tab); });
$("tabbar").addEventListener("keydown", (e) => {
  if (e.key !== "ArrowRight" && e.key !== "ArrowLeft") return;
  const tabs = [...document.querySelectorAll(".tab")]; const i = tabs.findIndex((t) => t.dataset.tab === state.tab);
  const n = tabs[(i + (e.key === "ArrowRight" ? 1 : tabs.length - 1)) % tabs.length]; selectTab(n.dataset.tab); n.focus();
});

/* ---------- secrets list ---------- */
function renderSecrets() {
  const ul = $("secretList"); ul.replaceChildren();
  if (state.loadingSecrets && !state.secrets.length) { for (let i = 0; i < 3; i++) ul.append(el("li", { class: "skeleton", "aria-hidden": "true" })); $("secretsCount").textContent = "Loading…"; return; }
  $("secretsCount").textContent = state.secrets.length === 1 ? "1 account on the token" : `${state.secrets.length} accounts on the token`;
  if (!state.secrets.length) {
    ul.append(el("li", { class: "empty card card-quiet" }, icon(["M4 10h16v11H4z", "M8 10V7a4 4 0 0 1 8 0v3"], 36), el("p", { text: "No accounts yet." }), el("p", { class: "faint", text: "Tap Add to scan a QR code or import from Google Authenticator." })));
    return;
  }
  state.secrets.forEach((name, i) => {
    const av = el("span", { class: "avatar", "aria-hidden": "true", text: (name.trim()[0] || "?").toUpperCase() });
    av.style.background = avatarColor(name); // CSSOM: allowed by CSP (no inline style attribute)
    ul.append(el("li", {}, el("button", { class: "item", type: "button", "aria-label": `Edit ${name}`, onclick: () => openEdit(i) },
      av, el("span", { class: "item-body" }, el("span", { class: "item-title", text: name || "(no name)" }), el("span", { class: "item-sub", text: `Slot ${i + 1}` })),
      el("span", { class: "chev" }, icon(["M9 6l6 6-6 6"])))));
  });
}
async function refreshSecrets() {
  state.loadingSecrets = true; renderSecrets();
  try {
    const r = await client.list();
    state.secrets = r.items;
    if (r.incomplete) toast("Some names couldn't be loaded.", "error");
  } catch (e) { if (client.state === "ready") toast(`Couldn't load accounts: ${msg(e)}`, "error"); }
  finally { state.loadingSecrets = false; renderSecrets(); }
}
$("btnRefresh").addEventListener("click", () => refreshSecrets());

/* ---------- add ---------- */
const addState = { mode: "scan", candidates: [], batch: null };
function setAddMode(mode) {
  addState.mode = mode;
  for (const b of document.querySelectorAll("#dlgAdd [data-mode]")) b.setAttribute("aria-selected", String(b.dataset.mode === mode));
  for (const p of document.querySelectorAll("#dlgAdd [data-pane]")) p.hidden = p.dataset.pane !== mode;
}
function resetAdd() {
  addState.candidates = []; addState.batch = null;
  $("importReview").hidden = true; $("importFoot").hidden = true;
  document.querySelector("#dlgAdd .segmented").hidden = false;
  setAddMode(addState.mode);
  for (const id of ["linkInput", "manName", "manSecret"]) $(id).value = "";
  setFieldError($("linkInput"), $("linkErr"), ""); setFieldError($("manName"), $("manNameErr"), ""); setFieldError($("manSecret"), $("manSecretErr"), "");
}
$("btnAdd").addEventListener("click", () => { resetAdd(); openDialog($("dlgAdd")); });
document.querySelector("#dlgAdd .segmented").addEventListener("click", (e) => { const b = e.target.closest("[data-mode]"); if (b) setAddMode(b.dataset.mode); });
$("btnImportBack").addEventListener("click", () => resetAdd());

function showImport(entries, meta) {
  // Google Authenticator splits large exports over several QR codes.
  if (meta && meta.batchSize > 1) {
    addState.batch = addState.batch && addState.batch.id === meta.batchId ? addState.batch : { id: meta.batchId, size: meta.batchSize, seen: new Set() };
    if (addState.batch.seen.has(meta.batchIndex)) return false;
    addState.batch.seen.add(meta.batchIndex);
  }
  for (const e of entries) {
    const reason = unsupportedReason(e);
    const chk = checkSecret(e.secret);
    addState.candidates.push({ ...e, secret: chk.ok ? chk.value : e.secret, disabledReason: reason || (chk.ok ? null : chk.error), checked: !reason && chk.ok });
  }
  renderImport();
  return true;
}
function renderImport() {
  document.querySelector("#dlgAdd .segmented").hidden = true;
  for (const p of document.querySelectorAll("#dlgAdd [data-pane]")) p.hidden = true;
  $("importReview").hidden = false; $("importFoot").hidden = false;
  const list = $("importList"); list.replaceChildren();
  const n = addState.candidates.length;
  $("importTitle").textContent = n === 1 ? "Found 1 account" : `Found ${n} accounts`;
  const b = addState.batch;
  $("importBatch").hidden = !b;
  if (b) $("importBatch").textContent = b.seen.size < b.size ? `Scanned ${b.seen.size} of ${b.size} export codes. Scan the rest to import everything.` : `All ${b.size} export codes scanned.`;
  addState.candidates.forEach((c, i) => {
    const id = `imp${i}`;
    const cb = el("input", { type: "checkbox", id: id + "c", "aria-label": `Import ${c.name}` });
    cb.checked = c.checked; cb.disabled = !!c.disabledReason;
    const name = el("input", { class: "input", id: id + "n", value: c.name, "aria-label": "Account name", autocomplete: "off" });
    name.disabled = !!c.disabledReason;
    const err = el("span", { class: "error-text", id: id + "e" });
    const body = el("div", { class: "stack" }, name, c.disabledReason ? el("span", { class: "tag tag-warn", text: "Not supported" }) : null,
      c.disabledReason ? el("span", { class: "faint", text: c.disabledReason }) : null, err);
    name.setAttribute("aria-describedby", err.id);
    cb.addEventListener("change", () => { c.checked = cb.checked; updateImportBtn(); });
    name.addEventListener("input", () => { c.name = name.value; err.textContent = ""; name.removeAttribute("aria-invalid"); });
    list.append(el("div", { class: "check-item", dataset: { disabled: String(!!c.disabledReason) } }, cb, body));
  });
  if (b && b.seen.size < b.size) list.append(el("button", { class: "btn btn-block", type: "button", text: "Scan next export code", onclick: () => startScanner("otp", "Scan the next export code") }));
  updateImportBtn();
}
function updateImportBtn() {
  const n = addState.candidates.filter((c) => c.checked && !c.disabledReason).length;
  $("btnImport").disabled = n === 0;
  $("btnImport").textContent = n === 1 ? "Add 1 account" : `Add ${n} accounts`;
}
$("btnImport").addEventListener("click", async () => {
  const chosen = addState.candidates.map((c, i) => ({ c, i })).filter(({ c }) => c.checked && !c.disabledReason);
  // Validate everything first so nothing is half-imported because of a typo.
  let bad = false;
  for (const { c, i } of chosen) {
    const e = checkName(c.name) || checkBudget(c.name.trim(), c.secret, "put");
    const err = $(`imp${i}e`), inp = $(`imp${i}n`);
    if (e) { bad = true; err.textContent = e; inp.setAttribute("aria-invalid", "true"); }
  }
  if (bad) { toast("Fix the highlighted names first.", "error"); return; }
  if (state.secrets.length + chosen.length > 255) { toast("The token can hold at most 255 accounts.", "error"); return; }
  await withBusy($("btnImport"), "Adding…", async () => {
    let ok = 0;
    for (const { c } of chosen) {
      try { await client.put(c.name.trim(), c.secret); ok++; c.checked = false; c.disabledReason = "Added"; }
      catch (e) { toast(`Couldn't add “${c.name}”: ${msg(e)}`, "error"); if (client.state !== "ready") return; }
    }
    if (ok) toast(ok === 1 ? "Account added." : `${ok} accounts added.`, "ok");
    if (ok === chosen.length) closeDialog($("dlgAdd")); else renderImport();
    refreshSecrets();
  });
});

$("linkForm").addEventListener("submit", (e) => {
  e.preventDefault();
  const tokens = $("linkInput").value.split(/\s+/).filter(Boolean);
  if (!tokens.length) { setFieldError($("linkInput"), $("linkErr"), "Paste a link first."); return; }
  try { for (const t of tokens) { const r = parsePayload(t); showImport(r.entries, r.meta); } }
  catch (err) { setFieldError($("linkInput"), $("linkErr"), msg(err)); }
});
$("manualForm").addEventListener("submit", async (e) => {
  e.preventDefault();
  const name = $("manName").value.trim();
  const nameErr = checkName(name); setFieldError($("manName"), $("manNameErr"), nameErr);
  const s = checkSecret($("manSecret").value); setFieldError($("manSecret"), $("manSecretErr"), s.ok ? "" : s.error);
  if (nameErr || !s.ok) return;
  const budget = checkBudget(name, s.value, "put"); if (budget) { setFieldError($("manName"), $("manNameErr"), budget); return; }
  if (state.secrets.length >= 255) { toast("The token can hold at most 255 accounts.", "error"); return; }
  await withBusy($("btnManualSave"), "Adding…", async () => {
    try { await client.put(name, s.value); toast("Account added.", "ok"); closeDialog($("dlgAdd")); refreshSecrets(); }
    catch (err) { toast(`Couldn't add: ${msg(err)}`, "error"); }
  });
});
$("btnScanOtp").addEventListener("click", () => startScanner("otp", "Scan an authenticator QR code"));
$("qrFile").addEventListener("change", async () => {
  const f = $("qrFile").files[0]; $("qrFile").value = "";
  if (!f) return;
  try {
    const bmp = await createImageBitmap(f);
    const text = await decodeFrame(bmp, bmp.width, bmp.height);
    bmp.close && bmp.close();
    if (!text) { toast("No QR code found in that image.", "error"); return; }
    const r = parsePayload(text); showImport(r.entries, r.meta);
  } catch (e) { toast(msg(e) || "Couldn't read that image.", "error"); }
});

/* ---------- edit ---------- */
let totpTimer = null;
function openEdit(i) {
  state.editIndex = i;
  const name = state.secrets[i];
  $("editName").value = name; $("editSecret").value = "";
  setFieldError($("editName"), $("editNameErr"), ""); setFieldError($("editSecret"), $("editSecretErr"), "");
  $("revealBox").open = false; $("replaceBox").open = false; hideReveal();
  $("dlgEditTitle").textContent = name || "Edit account";
  openDialog($("dlgEdit"));
}
function hideReveal() { $("revealed").hidden = true; $("revealedSecret").textContent = ""; $("btnReveal").hidden = false; clearInterval(totpTimer); totpTimer = null; }
$("dlgEdit").addEventListener("close", hideReveal);
$("revealBox").addEventListener("toggle", () => { if (!$("revealBox").open) hideReveal(); });
$("btnReveal").addEventListener("click", () => withBusy($("btnReveal"), "Reading…", async () => {
  try {
    const r = await client.get(state.editIndex);
    $("revealedSecret").textContent = r.secret.replace(/(.{4})/g, "$1 ").trim();
    $("revealed").hidden = false; $("btnReveal").hidden = true;
    const ring = $("totpRing"); const C = 2 * Math.PI * 13; ring.style.strokeDasharray = String(C);
    const tick = async () => {
      if ($("revealed").hidden) return;
      const now = Date.now(); const left = 30 - Math.floor(now / 1000) % 30;
      ring.style.strokeDashoffset = String(C * (1 - left / 30));
      try { const c = await totp(r.secret, now); $("totpCode").textContent = `${c.slice(0, 3)} ${c.slice(3)}`; } catch { $("totpCode").textContent = "—"; }
    };
    tick(); clearInterval(totpTimer); totpTimer = setInterval(tick, 1000);
  } catch (e) { toast(msg(e), "error"); }
}));
$("btnCopySecret").addEventListener("click", async () => {
  try { await navigator.clipboard.writeText($("revealedSecret").textContent.replace(/\s/g, "")); toast("Secret copied.", "ok", 2500); }
  catch { toast("Couldn't copy. Select the text instead.", "error"); }
});
$("editForm").addEventListener("submit", async (e) => {
  e.preventDefault();
  const name = $("editName").value.trim(); const raw = $("editSecret").value.trim();
  const nErr = checkName(name); setFieldError($("editName"), $("editNameErr"), nErr);
  let b32 = "";
  if (raw) { const s = checkSecret(raw); setFieldError($("editSecret"), $("editSecretErr"), s.ok ? "" : s.error); if (!s.ok) { $("replaceBox").open = true; return; } b32 = s.value; }
  else setFieldError($("editSecret"), $("editSecretErr"), "");
  if (nErr) return;
  const budget = checkBudget(name, b32, "update"); if (budget) { setFieldError($("editName"), $("editNameErr"), budget); return; }
  if (name === state.secrets[state.editIndex] && !b32) { closeDialog($("dlgEdit")); return; }
  await withBusy($("btnEditSave"), "Saving…", async () => {
    try { await client.update(state.editIndex, name, b32); toast(b32 ? "Account updated with the new key." : "Account renamed.", "ok"); closeDialog($("dlgEdit")); refreshSecrets(); }
    catch (err) { toast(`Couldn't save: ${msg(err)}`, "error"); }
  });
});
$("btnDelete").addEventListener("click", async () => {
  const i = state.editIndex, name = state.secrets[i];
  const yes = await confirmDialog(`Delete “${name}”?`, "This removes the account from the token. Make sure you have another way to sign in, or the original setup key.", "Delete");
  if (!yes) return;
  await withBusy($("btnDelete"), "Deleting…", async () => {
    try { await client.del(i); toast("Account deleted.", "ok"); closeDialog($("dlgEdit")); }
    catch (err) { toast(`Couldn't delete: ${msg(err)}`, "error"); }
    refreshSecrets(); // indexes shift after a delete
  });
});

/* ---------- wifi ---------- */
const AUTH = { 0: "Open", 1: "WEP", 2: "WPA", 3: "WPA2", 4: "WPA/WPA2", 5: "WPA2-Enterprise", 6: "WPA3", 7: "WPA2/WPA3", 8: "WAPI", 9: "WPA3-Enterprise" };
const bars = (rssi) => rssi >= -55 ? 4 : rssi >= -65 ? 3 : rssi >= -75 ? 2 : rssi >= -85 ? 1 : 0;
async function loadWifi() {
  $("wifiCurrent").textContent = "…";
  try { const w = await client.wifiGet(); $("wifiCurrent").textContent = w.configured ? w.ssid : "None saved"; }
  catch (e) { $("wifiCurrent").textContent = "Couldn't read"; }
}
function renderNetworks(items) {
  const ul = $("wifiList"); ul.replaceChildren();
  if (!items.length) { ul.append(el("li", { class: "faint", text: "No networks found. Hidden networks can be entered by name below." })); return; }
  // Strongest first, one row per SSID.
  const best = new Map(); for (const n of items) { const p = best.get(n.ssid); if (!p || n.rssi > p.rssi) best.set(n.ssid, n); }
  for (const n of [...best.values()].sort((a, b) => b.rssi - a.rssi)) {
    const enterprise = n.auth === 5 || n.auth === 9;
    const b = el("span", { class: "bars", dataset: { n: String(bars(n.rssi)) }, "aria-hidden": "true" }, el("i"), el("i"), el("i"), el("i"));
    const btn = el("button", { class: "item", type: "button", "aria-pressed": "false", "aria-label": `${n.ssid}, ${AUTH[n.auth] || "unknown security"}, signal ${bars(n.rssi)} of 4` },
      b, el("span", { class: "item-body" }, el("span", { class: "item-title", text: n.ssid }), el("span", { class: "item-sub", text: `${n.rssi} dBm` })),
      el("span", { class: enterprise ? "tag tag-warn" : "tag", text: enterprise ? "Not supported" : (AUTH[n.auth] || "Unknown") }));
    btn.disabled = enterprise;
    btn.addEventListener("click", () => {
      for (const x of ul.querySelectorAll(".item")) x.setAttribute("aria-pressed", "false");
      btn.setAttribute("aria-pressed", "true");
      $("wifiSsid").value = n.ssid; setFieldError($("wifiSsid"), $("wifiSsidErr"), "");
      $("wifiPass").value = ""; $("wifiPass").disabled = n.auth === 0;
      $("wifiPassHint").textContent = n.auth === 0 ? "Open network: no password needed." : "8–63 characters.";
      if (n.auth !== 0) $("wifiPass").focus();
    });
    ul.append(el("li", {}, btn));
  }
}
$("btnWifiScan").addEventListener("click", () => withBusy($("btnWifiScan"), "Scanning…", async () => {
  $("wifiList").replaceChildren(...[0, 1, 2].map(() => el("li", { class: "skeleton", "aria-hidden": "true" })));
  try { const r = await client.wifiScan(); renderNetworks(r.items); }
  catch (e) { $("wifiList").replaceChildren(); if (client.state === "ready") toast(`Scan failed: ${msg(e)}`, "error"); }
}));
$("wifiSsid").addEventListener("input", () => { $("wifiPass").disabled = false; $("wifiPassHint").textContent = "8–63 characters. Leave empty for an open network."; for (const x of $("wifiList").querySelectorAll(".item")) x.setAttribute("aria-pressed", "false"); });
$("wifiForm").addEventListener("submit", async (e) => {
  e.preventDefault();
  const ssid = $("wifiSsid").value, pass = $("wifiPass").disabled ? "" : $("wifiPass").value;
  const sLen = utf8Len(ssid), pLen = utf8Len(pass);
  const sErr = !ssid ? "Enter the network name." : sLen > LIMITS.SSID_MAX ? `Too long (${sLen} of 32 bytes).` : null;
  const pErr = pLen && pLen < LIMITS.PASS_MIN ? "Wi-Fi passwords are at least 8 characters." : pLen > LIMITS.PASS_MAX ? "Wi-Fi passwords are at most 63 characters." : null;
  setFieldError($("wifiSsid"), $("wifiSsidErr"), sErr); setFieldError($("wifiPass"), $("wifiPassErr"), pErr);
  if (sErr || pErr) return;
  await withBusy($("btnWifiSave"), "Saving…", async () => {
    try { await client.wifiSet(ssid, pass); $("wifiPass").value = ""; toast(`Saved “${ssid}”.`, "ok"); loadWifi(); }
    catch (err) { toast(`Couldn't save: ${msg(err)}`, "error"); }
  });
});

/* ---------- time ---------- */
const fmtTime = (unix) => new Date(unix * 1000).toLocaleString([], { hour: "2-digit", minute: "2-digit", second: "2-digit", day: "numeric", month: "short" });
async function readTime() {
  try {
    const t0 = Date.now(); const dev = await client.getTime(); const phone = Math.round((t0 + Date.now()) / 2000);
    $("timeDevice").textContent = fmtTime(dev); $("timePhone").textContent = fmtTime(phone);
    const drift = dev - phone; const ok = Math.abs(drift) <= 5;
    $("timeDrift").className = ok ? "drift-ok" : "drift-bad";
    const a = Math.abs(drift);
    const human = a >= 3000 ? `${+(a / 3600).toFixed(1)} h` : a >= 120 ? `${Math.round(a / 60)} min` : `${a} s`;
    $("timeDrift").textContent = ok ? "Clocks match. Codes will be correct." : `The token is ${human} ${drift > 0 ? "ahead" : "behind"}. Sync it, or codes may be rejected.`;
  } catch (e) { $("timeDevice").textContent = "—"; if (client.state === "ready") toast(`Couldn't read the clock: ${msg(e)}`, "error"); }
}
$("btnReadTime").addEventListener("click", () => withBusy($("btnReadTime"), "Checking…", readTime));
$("btnSyncTime").addEventListener("click", () => withBusy($("btnSyncTime"), "Syncing…", async () => {
  try { await client.setTime(Math.floor(Date.now() / 1000)); toast("Clock synced.", "ok"); await readTime(); }
  catch (e) { toast(`Couldn't sync: ${msg(e)}`, "error"); }
}));

/* =====================================================================
 * QR scanner: BarcodeDetector when available, jsQR otherwise
 * ===================================================================== */
const scan = { stream: null, raf: 0, mode: null, canvas: null, ctx: null, detector: null, busy: false, last: "" };
async function getDetector() {
  if (scan.detector !== null) return scan.detector;
  try {
    if ("BarcodeDetector" in window && (await BarcodeDetector.getSupportedFormats()).includes("qr_code")) scan.detector = new BarcodeDetector({ formats: ["qr_code"] });
    else scan.detector = false;
  } catch { scan.detector = false; }
  return scan.detector;
}
async function decodeFrame(source, w, h) {
  const det = await getDetector();
  if (det) { try { const r = await det.detect(source); if (r.length) return r[0].rawValue; } catch {} }
  if (typeof jsQR !== "function") return null;
  const max = 1024, s = Math.min(1, max / Math.max(w, h));
  const cw = Math.max(1, Math.round(w * s)), ch = Math.max(1, Math.round(h * s));
  if (!scan.canvas) { scan.canvas = document.createElement("canvas"); scan.ctx = scan.canvas.getContext("2d", { willReadFrequently: true }); }
  scan.canvas.width = cw; scan.canvas.height = ch;
  scan.ctx.drawImage(source, 0, 0, cw, ch);
  const img = scan.ctx.getImageData(0, 0, cw, ch);
  const r = jsQR(img.data, cw, ch, { inversionAttempts: "attemptBoth" });
  return r ? r.data : null;
}
async function startScanner(mode, title) {
  scan.mode = mode; scan.last = "";
  $("scanTitle").textContent = title; $("scanStatus").textContent = "Starting camera…";
  openDialog($("dlgScan"));
  if (!navigator.mediaDevices || !navigator.mediaDevices.getUserMedia) { $("scanStatus").textContent = "Camera isn't available in this browser."; return; }
  try {
    const sel = $("scanCamera");
    const constraints = sel.value ? { deviceId: { exact: sel.value } } : { facingMode: { ideal: "environment" } };
    scan.stream = await navigator.mediaDevices.getUserMedia({ video: { ...constraints, width: { ideal: 1280 }, height: { ideal: 720 } }, audio: false });
    if (!$("dlgScan").open) { stopScanner(); return; }
    const v = $("scanVideo"); v.srcObject = scan.stream; await v.play();
    $("scanStatus").textContent = mode === "key" ? "Point the camera at the QR code on the token." : "Point the camera at the QR code.";
    listCameras();
    loop();
  } catch (e) {
    $("scanStatus").textContent = e && e.name === "NotAllowedError" ? "Camera permission was denied. Allow it in your browser's site settings." : "Couldn't start the camera.";
  }
}
async function listCameras() {
  try {
    const cams = (await navigator.mediaDevices.enumerateDevices()).filter((d) => d.kind === "videoinput");
    const sel = $("scanCamera"); if (cams.length < 2) { sel.hidden = true; return; }
    const cur = scan.stream && scan.stream.getVideoTracks()[0].getSettings().deviceId;
    sel.replaceChildren(...cams.map((c, i) => { const o = el("option", { value: c.deviceId, text: c.label || `Camera ${i + 1}` }); o.selected = c.deviceId === cur; return o; }));
    sel.hidden = false;
  } catch {}
}
$("scanCamera").addEventListener("change", () => { const m = scan.mode, t = $("scanTitle").textContent; stopScanner(); startScanner(m, t); });
function loop() {
  scan.raf = requestAnimationFrame(async () => {
    const v = $("scanVideo");
    if (!scan.stream) return;
    if (!scan.busy && v.readyState >= 2 && v.videoWidth) {
      scan.busy = true;
      try { const text = await decodeFrame(v, v.videoWidth, v.videoHeight); if (text && text !== scan.last) { scan.last = text; onScanned(text); } }
      finally { scan.busy = false; }
    }
    if (scan.stream) loop();
  });
}
function stopScanner() {
  cancelAnimationFrame(scan.raf);
  if (scan.stream) { for (const t of scan.stream.getTracks()) t.stop(); scan.stream = null; }
  const v = $("scanVideo"); v.pause(); v.srcObject = null;
}
$("dlgScan").addEventListener("close", stopScanner);
function onScanned(text) {
  if (scan.mode === "key") {
    if (parseKeyHex(text)) { closeDialog($("dlgScan")); $("keyInput").value = text.replace(/[\s\-:]/g, "").toUpperCase(); updateKeyCount(); submitKey($("keyInput").value); }
    else $("scanStatus").textContent = "That's not the token's key QR. Scan the code on the token's screen.";
    return;
  }
  try {
    const r = parsePayload(text);
    const added = showImport(r.entries, r.meta);
    const b = addState.batch;
    if (b && b.seen.size < b.size) { $("scanStatus").textContent = added ? `Got ${b.seen.size} of ${b.size}. Show the next export code.` : `Already scanned. Show code ${b.seen.size + 1} of ${b.size}.`; return; }
    closeDialog($("dlgScan"));
  } catch (e) { $("scanStatus").textContent = msg(e); }
}

/* =====================================================================
 * Boot
 * ===================================================================== */
// Refuse to run inside a frame. frame-ancestors can't be set from a <meta>
// CSP, and hosts like GitHub Pages can't send headers, so this stops
// clickjacking there.
const framed = window.top !== window.self;
if (framed) document.body.replaceChildren();
if (!framed) boot();
function boot() {
// Hide passwords if the page is backgrounded (shoulder-surfing on phones).
document.addEventListener("visibilitychange", () => { if (document.hidden) { $("wifiPass").type = "password"; hideReveal(); stopScanner(); closeDialog($("dlgScan")); } });
if (!window.isSecureContext) { $("unsupportedReason").textContent = "This page must be opened over HTTPS for Bluetooth to work."; showView("Unsupported"); }
else if (!navigator.bluetooth) { showView("Unsupported"); }
else {
  showView("Connect");
  if (navigator.bluetooth.getAvailability) navigator.bluetooth.getAvailability().then((ok) => { if (!ok) toast("Bluetooth seems to be off or unavailable on this device.", "error", 6000); }).catch(() => {});
}
}

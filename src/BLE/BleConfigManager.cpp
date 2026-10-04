#include "BleConfigManager.h"

#include <M5StickC.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_ble_api.h>

#include "../Secret/SecretManager.h"
#include "../Time/TimeManager.h"
#include "../Util/Base32.h"
#include "../Wifi/WifiConfigManager.h"
#include "../Wifi/WifiManager.h"

// ── BLE Security Callbacks ──────────────────────────────────────────
// Required for the Bluedroid stack to drive the SMP pairing state machine.
uint32_t BleSecurityCallbacks::onPassKeyRequest() {
  // We are a display-only device; the passkey is entered on the client.
  return owner->getPin();
}

void BleSecurityCallbacks::onPassKeyNotify(uint32_t pass_key) {
  // Never log the passkey: serial logs are readable by anyone with a cable.
  Serial.println("BLE: Passkey displayed on screen");
}

bool BleSecurityCallbacks::onSecurityRequest() { return true; }

void BleSecurityCallbacks::onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
  if (cmpl.success) {
    Serial.println("BLE: Authentication complete");
  } else {
    Serial.printf("BLE: Authentication failed, reason=0x%x\n",
                  cmpl.fail_reason);
  }
  owner->onPairingResult(cmpl.success);
}

// Numeric comparison is not used with a DisplayOnly IO capability. Reject it
// rather than blindly accepting, so it can never silently downgrade.
bool BleSecurityCallbacks::onConfirmPIN(uint32_t pin) { return false; }

// ── BleConfigManager ────────────────────────────────────────────────
BleConfigManager::BleConfigManager()
    : pServer(nullptr),
      pService(nullptr),
      pTxCharacteristic(nullptr),
      pRxCharacteristic(nullptr),
      securityCallbacks(this),
      bleState(BLE_STATE_IDLE),
      pairingPin(0),
      hasNewCommand(false),
      keyRotatePending(false),
      disconnectPending(false),
      cmdPlaintextLen(0),
      lastKeepaliveTime(0),
      scanCount(0) {}

void BleConfigManager::start() {
  bleState = BLE_STATE_IDLE;
  hasNewCommand = false;
  keyRotatePending = false;
  disconnectPending = false;
  cmdPlaintextLen = 0;
  lastKeepaliveTime = 0;

  // Initialize BLE first: esp_fill_random() is only a true RNG while the
  // radio is on. Generating the key/PIN before this gave weak randomness.
  BLEDevice::init("TOTP-Token");
  BLEDevice::setMTU(256);

  crypto.generateRandomKey();
  // Fresh random 6-digit passkey per session (was a hard-coded constant,
  // which gives zero MITM protection since it's the same on every device).
  pairingPin = esp_random() % 1000000;

  // Require LE Secure Connections + MITM (passkey) + bonding, and refuse any
  // peer that can't do that. With the "only accept" option disabled, a peer
  // claiming NoInputNoOutput could pair via Just Works and skip the PIN.
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
  BLEDevice::setSecurityCallbacks(&securityCallbacks);

  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
  esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;
  uint8_t key_size = 16;
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;
  uint32_t passkey = pairingPin;

  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey,
                                 sizeof(uint32_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req,
                                 sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap,
                                 sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size,
                                 sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH,
                                 &auth_option, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key,
                                 sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key,
                                 sizeof(uint8_t));

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  pService = pServer->createService(BLE_SERVICE_UUID);

  // TX characteristic: client writes encrypted commands here.
  // Attribute permissions require an encrypted, MITM-authenticated link;
  // previously the attributes were open, so an unpaired peer could use them.
  pTxCharacteristic = pService->createCharacteristic(
      BLE_CHAR_TX_UUID, BLECharacteristic::PROPERTY_WRITE);
  pTxCharacteristic->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM);
  pTxCharacteristic->setCallbacks(this);

  // RX characteristic: device sends encrypted responses here via notify
  pRxCharacteristic = pService->createCharacteristic(
      BLE_CHAR_RX_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pRxCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM);
  BLE2902* cccd = new BLE2902();
  cccd->setAccessPermissions(ESP_GATT_PERM_READ_ENC_MITM |
                             ESP_GATT_PERM_WRITE_ENC_MITM);
  pRxCharacteristic->addDescriptor(cccd);

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();

  bleState = BLE_STATE_ADVERTISING;
}

void BleConfigManager::stop() {
  if (pServer != nullptr && pServer->getConnectedCount() > 0) {
    pServer->disconnect(pServer->getConnId());
  }

  BLEDevice::stopAdvertising();

  if (pService != nullptr) {
    pService->stop();
  }

  // deinit(true) releases the controller memory permanently AND leaves
  // BLEDevice marked as initialized, so the next BLEDevice::init() became a
  // no-op on a dead stack: re-entering BT config crashed/hung until reboot.
  BLEDevice::deinit(false);
  pServer = nullptr;
  pService = nullptr;
  pTxCharacteristic = nullptr;
  pRxCharacteristic = nullptr;

  crypto.reset();
  pairingPin = 0;
  bleState = BLE_STATE_IDLE;
  hasNewCommand = false;
  keyRotatePending = false;
  disconnectPending = false;
  memset(cmdPlaintext, 0, sizeof(cmdPlaintext));
}

void BleConfigManager::loop() {
  if (pServer == nullptr) return;

  if (disconnectPending) {
    disconnectPending = false;
    if (pServer->getConnectedCount() > 0) {
      pServer->disconnect(pServer->getConnId());
    }
  }

  // New AES key (and counters) for every connection, done in the main loop
  // so it can't race with an in-flight encrypt/decrypt.
  if (keyRotatePending) {
    keyRotatePending = false;
    crypto.generateRandomKey();
  }

  // Process commands in the main loop context (not in BLE callback)
  if (hasNewCommand) {
    if (cmdPlaintextLen > 0) {
      processCommand(cmdPlaintext, cmdPlaintextLen);
    }
    memset(cmdPlaintext, 0, sizeof(cmdPlaintext));
    cmdPlaintextLen = 0;
    hasNewCommand = false;
  }

  // Keepalive while connected but before the encrypted channel is up.
  if ((bleState == BLE_STATE_CONNECTED ||
       bleState == BLE_STATE_AUTHENTICATED) &&
      millis() - lastKeepaliveTime > 2000) {
    lastKeepaliveTime = millis();
    uint8_t heartbeat = 0x00;
    pRxCharacteristic->setValue(&heartbeat, 1);
    pRxCharacteristic->notify();
  }
}

uint8_t BleConfigManager::getState() { return bleState; }

uint32_t BleConfigManager::getPin() { return pairingPin; }

AesCrypto* BleConfigManager::getCrypto() { return &crypto; }

bool BleConfigManager::isConnected() { return bleState >= BLE_STATE_CONNECTED; }

bool BleConfigManager::isEncrypted() { return bleState == BLE_STATE_ENCRYPTED; }

// BLE Server Callbacks
void BleConfigManager::onConnect(BLEServer* pServer) {
  bleState = BLE_STATE_CONNECTED;
  lastKeepaliveTime = millis();
  Serial.printf("BLE: Client connected at %lu ms\n", millis());
}

void BleConfigManager::onDisconnect(BLEServer* pServer) {
  bleState = BLE_STATE_ADVERTISING;
  hasNewCommand = false;
  cmdPlaintextLen = 0;
  keyRotatePending = true;
  Serial.printf("BLE: Client disconnected at %lu ms\n", millis());

  // Don't delay() here: this runs on the Bluetooth stack's task.
  BLEDevice::startAdvertising();
}

void BleConfigManager::onPairingResult(bool success) {
  if (success) {
    if (bleState == BLE_STATE_CONNECTED) {
      bleState = BLE_STATE_AUTHENTICATED;
    }
  } else {
    disconnectPending = true;
  }
}

// Characteristic write callback - receives encrypted commands
void BleConfigManager::onWrite(BLECharacteristic* pCharacteristic) {
  // Defence in depth: attribute permissions should already enforce this.
  if (bleState < BLE_STATE_AUTHENTICATED) {
    return;
  }

  // Previous command still being processed by the main loop; drop this one
  // instead of overwriting the buffer underneath it.
  if (hasNewCommand) {
    Serial.println("BLE: Busy, dropping command");
    return;
  }

  std::string value = pCharacteristic->getValue();
  if (value.length() < AES_IV_LEN + AES_TAG_LEN + 1 ||
      value.length() > BLE_MAX_PACKET) {
    return;
  }

  uint8_t decrypted[BLE_MAX_PLAINTEXT];
  int decLen = crypto.decrypt((const uint8_t*)value.data(), value.length(),
                              decrypted, sizeof(decrypted));

  if (decLen < 1) {
    Serial.println("BLE: Decryption/authentication failed");
    return;
  }

  if (bleState == BLE_STATE_AUTHENTICATED) {
    bleState = BLE_STATE_ENCRYPTED;
  }

  memcpy(cmdPlaintext, decrypted, decLen);
  memset(decrypted, 0, sizeof(decrypted));
  cmdPlaintextLen = decLen;
  hasNewCommand = true;  // publish last
}

// Largest response plaintext that fits in one notification. Notifications
// are capped at (ATT MTU - 3); anything longer was silently truncated by the
// stack, which broke the GCM tag and made the client reject the response.
size_t BleConfigManager::maxResponsePlaintext() {
  size_t mtu = 23;
  if (pServer != nullptr && pServer->getConnectedCount() > 0) {
    mtu = pServer->getPeerMTU(pServer->getConnId());
  }
  size_t maxPacket = mtu > 3 ? mtu - 3 : 0;
  if (maxPacket > BLE_MAX_PACKET) maxPacket = BLE_MAX_PACKET;
  if (maxPacket <= AES_IV_LEN + AES_TAG_LEN) return 0;
  return maxPacket - AES_IV_LEN - AES_TAG_LEN;
}

void BleConfigManager::sendStatus(uint8_t cmd, uint8_t status) {
  uint8_t resp[] = {cmd, status};
  sendEncryptedResponse(resp, sizeof(resp));
}

void BleConfigManager::sendEncryptedResponse(const uint8_t* data, size_t len) {
  if (pRxCharacteristic == nullptr || len == 0) return;

  if (len > maxResponsePlaintext()) {
    Serial.println("BLE: Response exceeds MTU");
    if (len > 2) {
      sendStatus(data[0], BLE_RESP_FAIL);
    }
    return;
  }

  uint8_t encrypted[BLE_MAX_PACKET];
  int encLen = crypto.encrypt(data, len, encrypted, sizeof(encrypted));

  if (encLen < 0) {
    Serial.println("BLE: Encryption failed");
    return;
  }

  pRxCharacteristic->setValue(encrypted, encLen);
  pRxCharacteristic->notify();
}

void BleConfigManager::processCommand(const uint8_t* plaintext, int len) {
  uint8_t cmd = plaintext[0];

  switch (cmd) {
    case BLE_CMD_LIST_SECRETS:
      handleListSecrets(len >= 2 ? plaintext[1] : 0);
      break;
    case BLE_CMD_GET_SECRET:
      if (len >= 2) handleGetSecret(plaintext[1]);
      break;
    case BLE_CMD_DELETE_SECRET:
      if (len >= 2) handleDeleteSecret(plaintext[1]);
      break;
    case BLE_CMD_PUT_SECRET:
      handlePutSecret(plaintext + 1, len - 1);
      break;
    case BLE_CMD_SET_TIME:
      if (len >= 5) handleSetTime(plaintext + 1, len - 1);
      break;
    case BLE_CMD_GET_TIME:
      handleGetTime();
      break;
    case BLE_CMD_PING:
      handlePing();
      break;
    case BLE_CMD_UPDATE_SECRET:
      handleUpdateSecret(plaintext + 1, len - 1);
      break;
    case BLE_CMD_WIFI_GET:
      handleWifiGet();
      break;
    case BLE_CMD_WIFI_SET:
      handleWifiSet(plaintext + 1, len - 1);
      break;
    case BLE_CMD_WIFI_SCAN:
      handleWifiScan();
      break;
    case BLE_CMD_WIFI_SCAN_RESULTS:
      handleWifiScanResults(len >= 2 ? plaintext[1] : 0);
      break;
    default:
      Serial.printf("BLE: Unknown command 0x%02X\n", cmd);
      sendStatus(cmd, BLE_RESP_FAIL);
      break;
  }
}

void BleConfigManager::handleListSecrets(uint8_t start) {
  secretManager.start();
  int count = secretManager.getSecretCount();
  const size_t limit = min((size_t)BLE_MAX_PLAINTEXT, maxResponsePlaintext());

  // Response: [CMD] [total] [start] [n] ([name_len] [name])*n
  // Names that don't fit in one notification are fetched with a later
  // request starting at (start + n).
  uint8_t resp[BLE_MAX_PLAINTEXT];
  size_t pos = 4;
  uint8_t included = 0;
  resp[0] = BLE_CMD_LIST_SECRETS;
  resp[1] = (uint8_t)count;
  resp[2] = start;

  for (int i = start; i < count; i++) {
    Secret sec = secretManager.readRecord(i);
    String name = sec.isValid() ? sec.getName() : String();
    size_t nameLen = min((size_t)name.length(), (size_t)255);
    if (pos + 1 + nameLen > limit) {
      // Always make progress: truncate a single over-long name.
      if (included == 0 && pos + 1 < limit) {
        nameLen = limit - pos - 1;
      } else {
        break;
      }
    }
    resp[pos++] = (uint8_t)nameLen;
    memcpy(resp + pos, name.c_str(), nameLen);
    pos += nameLen;
    included++;
  }
  resp[3] = included;

  sendEncryptedResponse(resp, pos);
}

void BleConfigManager::handleGetSecret(uint8_t index) {
  secretManager.start();

  if (!secretManager.isIndexValid(index)) {
    sendStatus(BLE_CMD_GET_SECRET, BLE_RESP_FAIL);
    return;
  }

  Secret sec = secretManager.readRecord(index);
  if (!sec.isValid()) {
    sendStatus(BLE_CMD_GET_SECRET, BLE_RESP_FAIL);
    return;
  }

  String name = sec.getName();
  byte* secretBase32 = nullptr;
  int b32Len = Base32::toBase32(sec.get(), sec.bitLen(), secretBase32);

  // [CMD] [OK] [name_len] [name] [b32_len] [b32]
  // Bounds-checked: a long name + secret previously overflowed the 256-byte
  // stack buffer.
  size_t needed = 4 + name.length() + (size_t)b32Len;
  if (secretBase32 == nullptr || name.length() > 255 || b32Len > 255 ||
      needed > BLE_MAX_PLAINTEXT) {
    if (secretBase32 != nullptr) {
      memset(secretBase32, 0, b32Len);
      free(secretBase32);
    }
    sendStatus(BLE_CMD_GET_SECRET, BLE_RESP_FAIL);
    return;
  }

  uint8_t resp[BLE_MAX_PLAINTEXT];
  size_t pos = 0;
  resp[pos++] = BLE_CMD_GET_SECRET;
  resp[pos++] = BLE_RESP_OK;
  resp[pos++] = (uint8_t)name.length();
  memcpy(resp + pos, name.c_str(), name.length());
  pos += name.length();
  resp[pos++] = (uint8_t)b32Len;
  memcpy(resp + pos, secretBase32, b32Len);
  pos += b32Len;

  memset(secretBase32, 0, b32Len);
  free(secretBase32);

  sendEncryptedResponse(resp, pos);
  memset(resp, 0, sizeof(resp));
}

void BleConfigManager::handleDeleteSecret(uint8_t index) {
  secretManager.start();
  sendStatus(BLE_CMD_DELETE_SECRET, secretManager.deleteRecord(index)
                                        ? BLE_RESP_OK
                                        : BLE_RESP_FAIL);
}

void BleConfigManager::handlePutSecret(const uint8_t* data, int len) {
  // data: [name_len] [secret_len] [name bytes] [secret base32 bytes]
  if (len < 2) {
    sendStatus(BLE_CMD_PUT_SECRET, BLE_RESP_FAIL);
    return;
  }

  uint8_t nameLen = data[0];
  uint8_t secretB32Len = data[1];

  if (nameLen == 0 || secretB32Len == 0 ||
      len < 2 + (int)nameLen + (int)secretB32Len) {
    sendStatus(BLE_CMD_PUT_SECRET, BLE_RESP_FAIL);
    return;
  }

  // Length now comes from the bytes actually decoded. Previously it was
  // (base32 chars * 5) bits, which over-read the decode buffer and appended
  // heap garbage to any secret whose length isn't a multiple of 8 chars or
  // that contained '=' padding -> wrong OTP codes. Invalid base32 also freed
  // an uninitialised pointer.
  Secret secret = Secret::fromBase32(data + 2 + nameLen, secretB32Len);
  if (!secret.isValid()) {
    sendStatus(BLE_CMD_PUT_SECRET, BLE_RESP_FAIL);
    return;
  }

  char name[256];
  memcpy(name, data + 2, nameLen);
  name[nameLen] = '\0';
  secret.setName(String(name));

  secretManager.start();
  bool ok = secretManager.putRecord(&secret);
  sendStatus(BLE_CMD_PUT_SECRET, ok ? BLE_RESP_OK : BLE_RESP_FAIL);
}

void BleConfigManager::handleSetTime(const uint8_t* data, int len) {
  if (len < 4) return;

  // Big-endian 4-byte unix timestamp
  uint32_t timestamp = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
                       ((uint32_t)data[2] << 8) | (uint32_t)data[3];

  // The RTC holds *local* wall time (TimeManager sets TZ to UTC+8 and
  // converts RTC -> unix with mktime()). Writing gmtime() here, as before,
  // put UTC wall time in the RTC, so the device's unix time (and every TOTP
  // code) ended up 8 hours off after a Bluetooth time sync.
  TimeManager::getInstance();  // ensures TZ is set
  time_t rawtime = (time_t)timestamp;
  struct tm tmLocal;
  struct tm* timeinfo = localtime_r(&rawtime, &tmLocal);

  if (timeinfo == nullptr) {
    sendStatus(BLE_CMD_SET_TIME, BLE_RESP_FAIL);
    return;
  }

  RTC_DateTypeDef date;
  date.Date = timeinfo->tm_mday;
  date.Month = timeinfo->tm_mon + 1;
  date.Year = timeinfo->tm_year + 1900;
  date.WeekDay = timeinfo->tm_wday;

  RTC_TimeTypeDef time;
  time.Hours = timeinfo->tm_hour;
  time.Minutes = timeinfo->tm_min;
  time.Seconds = timeinfo->tm_sec;

  M5.Rtc.SetTime(&time);
  M5.Rtc.SetData(&date);

  sendStatus(BLE_CMD_SET_TIME, BLE_RESP_OK);
}

void BleConfigManager::handleGetTime() {
  TimeManager* tm = TimeManager::getInstance();
  TimeData td = tm->getTime();
  long unixTime = td.unix();

  uint8_t resp[6];
  resp[0] = BLE_CMD_GET_TIME;
  resp[1] = (unixTime >> 24) & 0xFF;
  resp[2] = (unixTime >> 16) & 0xFF;
  resp[3] = (unixTime >> 8) & 0xFF;
  resp[4] = unixTime & 0xFF;

  sendEncryptedResponse(resp, 5);
}

void BleConfigManager::handlePing() {
  sendStatus(BLE_CMD_PING, BLE_RESP_OK);
}

void BleConfigManager::handleUpdateSecret(const uint8_t* data, int len) {
  // data: [index] [name_len] [secret_len] [name] [secret base32]
  // secret_len == 0 keeps the existing secret (rename only).
  if (len < 3) {
    sendStatus(BLE_CMD_UPDATE_SECRET, BLE_RESP_FAIL);
    return;
  }
  uint8_t index = data[0];
  uint8_t nameLen = data[1];
  uint8_t secretB32Len = data[2];
  if (nameLen == 0 || len < 3 + (int)nameLen + (int)secretB32Len) {
    sendStatus(BLE_CMD_UPDATE_SECRET, BLE_RESP_FAIL);
    return;
  }

  char name[256];
  memcpy(name, data + 3, nameLen);
  name[nameLen] = '\0';

  secretManager.start();
  bool ok;
  if (secretB32Len == 0) {
    ok = secretManager.updateRecord(index, String(name), nullptr);
  } else {
    Secret secret = Secret::fromBase32(data + 3 + nameLen, secretB32Len);
    ok = secret.isValid() &&
         secretManager.updateRecord(index, String(name), &secret);
  }
  sendStatus(BLE_CMD_UPDATE_SECRET, ok ? BLE_RESP_OK : BLE_RESP_FAIL);
}

void BleConfigManager::handleWifiGet() {
  // [CMD] [OK] [configured 0/1] [ssid_len] [ssid]. Never returns the
  // passphrase.
  WifiConfig config = wifiConfigManager.getConfig();
  uint8_t resp[4 + 33];
  size_t pos = 0;
  resp[pos++] = BLE_CMD_WIFI_GET;
  resp[pos++] = BLE_RESP_OK;
  resp[pos++] = config.isConfigured ? 1 : 0;
  size_t ssidLen = config.isConfigured ? min((size_t)config.ssid.length(),
                                             (size_t)32)
                                       : 0;
  resp[pos++] = (uint8_t)ssidLen;
  memcpy(resp + pos, config.ssid.c_str(), ssidLen);
  pos += ssidLen;
  sendEncryptedResponse(resp, pos);
}

void BleConfigManager::handleWifiSet(const uint8_t* data, int len) {
  // data: [ssid_len] [pass_len] [ssid] [passphrase]
  // ssid 1..32 bytes; passphrase empty (open network) or 8..63 bytes.
  if (len < 2) {
    sendStatus(BLE_CMD_WIFI_SET, BLE_RESP_FAIL);
    return;
  }
  uint8_t ssidLen = data[0];
  uint8_t passLen = data[1];
  if (ssidLen == 0 || ssidLen > 32 || (passLen != 0 && passLen < 8) ||
      passLen > 63 || len < 2 + (int)ssidLen + (int)passLen) {
    sendStatus(BLE_CMD_WIFI_SET, BLE_RESP_FAIL);
    return;
  }
  char ssid[33];
  char pass[64];
  memcpy(ssid, data + 2, ssidLen);
  ssid[ssidLen] = '\0';
  memcpy(pass, data + 2 + ssidLen, passLen);
  pass[passLen] = '\0';

  wifiConfigManager.putConfig(String(ssid), String(pass));
  memset(pass, 0, sizeof(pass));
  sendStatus(BLE_CMD_WIFI_SET, BLE_RESP_OK);
}

void BleConfigManager::handleWifiScan() {
  // Blocking scan (~2-4 s) with BLE still connected (Wi-Fi/BT coexistence),
  // then the Wi-Fi radio is switched off again. Results are cached for
  // BLE_CMD_WIFI_SCAN_RESULTS.
  // Response: [CMD] [status] [count]
  scanCount = 0;
  WiFi.mode(WIFI_STA);
  int16_t found = WiFi.scanNetworks();
  if (found < 0) {
    wifiManager.disconnectWifi();
    uint8_t resp[] = {BLE_CMD_WIFI_SCAN, BLE_RESP_FAIL, 0};
    sendEncryptedResponse(resp, sizeof(resp));
    return;
  }
  for (int i = 0; i < found && scanCount < BLE_WIFI_SCAN_MAX; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) continue;  // hidden network
    WifiScanEntry& e = scanResults[scanCount++];
    size_t n = min((size_t)ssid.length(), (size_t)32);
    memcpy(e.ssid, ssid.c_str(), n);
    e.ssid[n] = '\0';
    e.rssi = (int8_t)WiFi.RSSI(i);
    e.auth = (uint8_t)WiFi.encryptionType(i);
  }
  WiFi.scanDelete();
  wifiManager.disconnectWifi();

  uint8_t resp[] = {BLE_CMD_WIFI_SCAN, BLE_RESP_OK, scanCount};
  sendEncryptedResponse(resp, sizeof(resp));
}

void BleConfigManager::handleWifiScanResults(uint8_t start) {
  // Response: [CMD] [total] [start] [n] ([rssi int8] [auth] [len] [ssid])*n
  const size_t limit = min((size_t)BLE_MAX_PLAINTEXT, maxResponsePlaintext());
  uint8_t resp[BLE_MAX_PLAINTEXT];
  size_t pos = 4;
  uint8_t included = 0;
  resp[0] = BLE_CMD_WIFI_SCAN_RESULTS;
  resp[1] = scanCount;
  resp[2] = start;
  for (int i = start; i < scanCount; i++) {
    const WifiScanEntry& e = scanResults[i];
    size_t n = strlen(e.ssid);
    if (pos + 3 + n > limit) break;
    resp[pos++] = (uint8_t)e.rssi;
    resp[pos++] = e.auth;
    resp[pos++] = (uint8_t)n;
    memcpy(resp + pos, e.ssid, n);
    pos += n;
    included++;
  }
  resp[3] = included;
  sendEncryptedResponse(resp, pos);
}

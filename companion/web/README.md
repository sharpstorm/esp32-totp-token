# Token Companion (Web Bluetooth)

A single-file web app for configuring the token over encrypted Bluetooth: manage accounts (add,
rename, replace key, reveal, delete), import Google Authenticator exports, set Wi-Fi, and sync the
clock. Protocol: [`docs/BLE_PROTOCOL.md`](../../docs/BLE_PROTOCOL.md).

`npm run build` produces the deployable app: **`public/index.html`**, one self-contained file
with no external resources, plus `public/_headers` with the security headers for Cloudflare Pages.
`public/` is a build output and is not committed.

## Deploy to Cloudflare Pages

1. Create a Pages project from the repo.
2. Set **Root directory** to `companion/web`.
3. Set **Build command** to `npm run build`. Cloudflare installs the dependencies from
   `package-lock.json` first.
4. Set **Build output directory** to `public`.

You can also run `npm run build` locally and drag `public/` into a direct-upload Pages project.

In the Cloudflare dashboard, leave **Web Analytics** off for the project. The headers block it
anyway, but there's no reason to have Cloudflare inject it.

## Deploy to GitHub Pages

`.github/workflows/pages.yml` publishes `public/index.html` whenever `companion/web/` changes on
`main`/`master`, or when you run it by hand from the Actions tab. It builds the page and deploys
it.

One-time setup: in the repo, go to **Settings → Pages → Build and deployment** and set
**Source** to **GitHub Actions**.

GitHub Pages can't send custom headers, so `_headers` doesn't apply there. The CSP `<meta>` tag
in the page still blocks every script, style and network request that isn't the page's own. The
app also refuses to run inside a frame, since a `<meta>` CSP can't set `frame-ancestors`. You lose
`Permissions-Policy` and the other response headers, so Cloudflare Pages is the stricter of the
two hosts.

## How tracking is blocked

`public/_headers` sends a Content-Security-Policy where:

* **Scripts and styles are allowed only by SHA-256 hash** (`script-src 'sha256-…'`). There is no
  `'self'` and no `'unsafe-inline'`, so only the exact code in `index.html` can run. Anything
  Cloudflare or a proxy injects is blocked, including the Web Analytics beacon from
  `static.cloudflareinsights.com` and same-origin `/cdn-cgi/` scripts.
* **`connect-src 'none'`** means the page can't make any network request at all: no `fetch`,
  beacon or WebSocket. Images are limited to `data:` (the favicon).
* **`Cache-Control: no-transform`** tells Cloudflare not to rewrite the HTML: no beacon
  injection, Rocket Loader or email obfuscation.
* **No `report-uri` / `report-to`.** A reporting endpoint would itself send data out.
* **Other headers:** `Permissions-Policy` allows only Bluetooth and camera for this origin, plus
  `Referrer-Policy: no-referrer`, `X-Frame-Options`/`frame-ancestors`, COOP/COEP/CORP and HSTS.

The same CSP, minus `frame-ancestors`, is also in a `<meta>` tag, so it still applies if the file
is hosted somewhere else.

## Editing

The source is in `src/`:

* `src/index.html` is the markup.
* `src/app.css` is the styles.
* `src/main.js` is the app. It imports jsQR from npm.

```sh
npm ci
npm run build   # writes public/index.html + public/_headers
```

The build uses esbuild to bundle `main.js` with its dependencies and to minify the CSS. It inlines
both into `index.html` and computes the CSP hashes for exactly that code. The hashes are
recomputed on every build, so there's nothing to keep in sync by hand. To update jsQR, bump it in
`package.json` and rebuild.

`build` also fails if the output contains:

* an external URL,
* an inline `style=""` or `on*=` attribute, or
* a resource reference that isn't `data:`.

## Browser support

Web Bluetooth works in **Chrome or Edge** on Android, Windows, macOS, Linux and ChromeOS. On
**iPhone/iPad**, Safari doesn't support it; use the **Bluefy** browser. Firefox doesn't support
it.

QR scanning uses the built-in `BarcodeDetector` where available (Android, macOS), with jsQR (bundled
into the page) as a fallback.

The token's time-based codes must be **SHA-1, 6 digits, 30 s**. The app flags anything else on
import.

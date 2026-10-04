# esp32-totp-token

Firmware for an M5StickC TOTP token, with a Wi-Fi (HTTPS) web configurator and a Bluetooth configuration mode.

- Build: see `.github/workflows/build.yml` (web app -> firmware -> merged flash image).
- Bluetooth protocol and security model: [docs/BLE_PROTOCOL.md](docs/BLE_PROTOCOL.md)
- HTTPS cert: run `scripts/generate_ec_key.sh` and copy `SslCertStore.cpp` to `src/AppServer/ssl/` (gitignored).

#pragma once

#ifndef BleConfigManager_h
#define BleConfigManager_h

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <stdint.h>

#include "AesCrypto.h"

// BLE Service and Characteristic UUIDs
#define BLE_SERVICE_UUID "4f545000-7837-4c91-82c3-4f1f5cb0f982"
#define BLE_CHAR_TX_UUID "4f545001-7837-4c91-82c3-4f1f5cb0f982"
#define BLE_CHAR_RX_UUID "4f545002-7837-4c91-82c3-4f1f5cb0f982"

// Command codes
#define BLE_CMD_LIST_SECRETS 0x01
#define BLE_CMD_GET_SECRET 0x02
#define BLE_CMD_DELETE_SECRET 0x03
#define BLE_CMD_PUT_SECRET 0x04
#define BLE_CMD_SET_TIME 0x05
#define BLE_CMD_GET_TIME 0x06
#define BLE_CMD_PING 0x07
#define BLE_CMD_UPDATE_SECRET 0x08
#define BLE_CMD_WIFI_GET 0x09
#define BLE_CMD_WIFI_SET 0x0A
#define BLE_CMD_WIFI_SCAN 0x0B
#define BLE_CMD_WIFI_SCAN_RESULTS 0x0C

#define BLE_WIFI_SCAN_MAX 20

// Response status
#define BLE_RESP_OK 0x00
#define BLE_RESP_FAIL 0x01

// Max buffer size
#define BLE_MAX_PLAINTEXT 256
#define BLE_MAX_PACKET (BLE_MAX_PLAINTEXT + AES_IV_LEN + AES_TAG_LEN)

// BLE state
#define BLE_STATE_IDLE 0
#define BLE_STATE_ADVERTISING 1
#define BLE_STATE_CONNECTED 2      // link up, pairing not yet complete
#define BLE_STATE_AUTHENTICATED 3  // LE Secure Connections + passkey done
#define BLE_STATE_ENCRYPTED 4      // first valid AES-GCM command received

class BleConfigManager;

class BleSecurityCallbacks : public BLESecurityCallbacks {
 private:
  BleConfigManager* owner;

 public:
  explicit BleSecurityCallbacks(BleConfigManager* owner) : owner(owner) {}

  uint32_t onPassKeyRequest() override;
  void onPassKeyNotify(uint32_t pass_key) override;
  bool onSecurityRequest() override;
  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) override;
  bool onConfirmPIN(uint32_t pin) override;
};

class BleConfigManager : public BLEServerCallbacks,
                         public BLECharacteristicCallbacks {
 private:
  BLEServer* pServer;
  BLEService* pService;
  BLECharacteristic* pTxCharacteristic;
  BLECharacteristic* pRxCharacteristic;

  BleSecurityCallbacks securityCallbacks;
  AesCrypto crypto;
  volatile uint8_t bleState;
  uint32_t pairingPin;
  // Set by the BT task, cleared by the main loop once processed. While set,
  // new writes are dropped so cmdPlaintext is never overwritten mid-command.
  volatile bool hasNewCommand;
  volatile bool keyRotatePending;
  volatile bool disconnectPending;

  uint8_t cmdPlaintext[BLE_MAX_PLAINTEXT];
  volatile int cmdPlaintextLen;
  uint32_t lastKeepaliveTime;

  void processCommand(const uint8_t* plaintext, int len);
  void sendEncryptedResponse(const uint8_t* data, size_t len);
  void sendStatus(uint8_t cmd, uint8_t status);
  size_t maxResponsePlaintext();

  struct WifiScanEntry {
    char ssid[33];
    int8_t rssi;
    uint8_t auth;
  };
  WifiScanEntry scanResults[BLE_WIFI_SCAN_MAX];
  uint8_t scanCount;

  void handleListSecrets(uint8_t start);
  void handleUpdateSecret(const uint8_t* data, int len);
  void handleWifiGet();
  void handleWifiSet(const uint8_t* data, int len);
  void handleWifiScan();
  void handleWifiScanResults(uint8_t start);
  void handleGetSecret(uint8_t index);
  void handleDeleteSecret(uint8_t index);
  void handlePutSecret(const uint8_t* data, int len);
  void handleSetTime(const uint8_t* data, int len);
  void handleGetTime();
  void handlePing();

 public:
  BleConfigManager();

  void start();
  void stop();
  void loop();

  uint8_t getState();
  uint32_t getPin();
  AesCrypto* getCrypto();
  bool isConnected();
  bool isEncrypted();

  // BLEServerCallbacks
  void onConnect(BLEServer* pServer) override;
  void onDisconnect(BLEServer* pServer) override;

  // BLECharacteristicCallbacks
  void onWrite(BLECharacteristic* pCharacteristic) override;

  // Called from BleSecurityCallbacks
  void onPairingResult(bool success);
};

#endif
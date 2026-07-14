#include "FindMyAdv.h"

#include <string.h>
#include "sdkconfig.h"

#ifndef FINDMYADV_DISABLE_BUILTIN_ACCELEROMETER
#define FINDMYADV_DISABLE_BUILTIN_ACCELEROMETER 0
#endif

#if !FINDMYADV_DISABLE_BUILTIN_ACCELEROMETER
#include <Wire.h>
#endif

#ifndef FINDMYADV_USE_BLUEDROID
#define FINDMYADV_USE_BLUEDROID 0
#endif

#if FINDMYADV_USE_BLUEDROID
#if !defined(CONFIG_BT_BLUEDROID_ENABLED) || !CONFIG_BT_BLUEDROID_ENABLED
#error "FINDMYADV_USE_BLUEDROID requires a core built with Bluedroid"
#endif
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#else
#include <NimBLEDevice.h>
#endif

namespace {

FindMyAdvConfig cfg;
char appleText[41];
char googleText[41];
uint8_t appleKey[28];
bool appleEnabled = false;
bool googleEnabled = false;
volatile bool running = false;
volatile bool stopRequested = false;
FindMyAdvStatus currentStatus = FindMyAdvStatus::Stopped;
const char *errorText = "";
TaskHandle_t schedulerTask = nullptr;

uint8_t appleAdv[31] = {
    0x1e, 0xff, 0x4c, 0x00, 0x12, 0x19, 0x00,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0x00, 0x00,
};
uint8_t googleAdv[29] = {
    0x02, 0x01, 0x06,
    0x19, 0x16, 0xaa, 0xfe, 0x41,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0x00,
};
uint8_t appleAddr[6];
uint8_t googleAddr[6];

bool accelOk = false;
int32_t accelPrevious = -1;
uint32_t motionUntil = 0;
uint8_t accelAddress = 0x18;
uint32_t activeInterval = 0;
uint32_t lastSwitch = 0;
uint8_t selectedProvider = 0;

#if FINDMYADV_USE_BLUEDROID
esp_ble_adv_params_t advParams = {
    .adv_int_min = 0,
    .adv_int_max = 0,
    .adv_type = ADV_TYPE_NONCONN_IND,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
uint8_t pendingAddress[6];
uint8_t pendingData[31];
uint8_t pendingDataLength = 0;
volatile bool pendingRestart = false;
volatile bool bluedroidAdvertising = false;
#endif

void copyKey(char *destination, const char *source) {
    memset(destination, 0, 41);
    if (source) strncpy(destination, source, 40);
}

bool keyFitsInternalBuffer(const char *source) {
    return !source || strnlen(source, 42) <= 40;
}

int base64Value(char value) {
    if (value >= 'A' && value <= 'Z') return value - 'A';
    if (value >= 'a' && value <= 'z') return value - 'a' + 26;
    if (value >= '0' && value <= '9') return value - '0' + 52;
    if (value == '+') return 62;
    if (value == '/') return 63;
    return -1;
}

bool decodeApple(const char *input) {
    if (!input || strlen(input) != 40 || input[38] != '=' || input[39] != '=')
        return false;
    uint32_t accumulator = 0;
    int bits = 0;
    size_t output = 0;
    for (size_t i = 0; input[i] && input[i] != '='; ++i) {
        const int value = base64Value(input[i]);
        if (value < 0) return false;
        accumulator = (accumulator << 6) | static_cast<uint32_t>(value);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (output >= sizeof(appleKey)) return false;
            appleKey[output++] = static_cast<uint8_t>(accumulator >> bits);
        }
    }
    return output == sizeof(appleKey);
}

int hexValue(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

bool decodeGoogle(const char *input) {
    if (!input || strlen(input) != 40) return false;
    for (int i = 0; i < 20; ++i) {
        const int high = hexValue(input[i * 2]);
        const int low = hexValue(input[i * 2 + 1]);
        if (high < 0 || low < 0) return false;
        googleAdv[8 + i] = static_cast<uint8_t>((high << 4) | low);
    }
    return true;
}

void buildAppleFrame() {
    memcpy(&appleAdv[7], &appleKey[6], 22);
    appleAdv[29] = appleKey[0] >> 6;
    appleAddr[0] = appleKey[0] | 0xc0;
    memcpy(&appleAddr[1], &appleKey[1], 5);
}

#if !FINDMYADV_DISABLE_BUILTIN_ACCELEROMETER
uint8_t readAccelRegister(uint8_t address, uint8_t reg) {
    TwoWire *wire = cfg.accelerometerWire ? cfg.accelerometerWire : &Wire;
    wire->beginTransmission(address);
    wire->write(reg);
    if (wire->endTransmission(false) != 0) return 0;
    if (wire->requestFrom(static_cast<int>(address), 1) != 1) return 0;
    return wire->read();
}

bool probeAccelerometer(uint8_t address) {
    if (readAccelRegister(address, 0x0f) != 0x33) return false;
    TwoWire *wire = cfg.accelerometerWire ? cfg.accelerometerWire : &Wire;
    wire->beginTransmission(address);
    wire->write(0x20);
    wire->write(0x57);
    if (wire->endTransmission() != 0) return false;
    accelAddress = address;
    return true;
}
#endif

void initializeAccelerometer() {
    accelOk = false;
#if FINDMYADV_DISABLE_BUILTIN_ACCELEROMETER
    return;
#else
    if (!cfg.accelerometerEnabled) return;
    if (!cfg.accelerometerWire) cfg.accelerometerWire = &Wire;
    if (cfg.initializeAccelerometerBus)
        cfg.accelerometerWire->begin(cfg.accelerometerSda,
                                     cfg.accelerometerScl);
    if (cfg.accelerometerAddress) {
        accelOk = probeAccelerometer(cfg.accelerometerAddress);
    } else {
        accelOk = probeAccelerometer(0x18) || probeAccelerometer(0x19);
    }
#endif
}

#if !FINDMYADV_DISABLE_BUILTIN_ACCELEROMETER
int32_t accelerationMagnitude() {
    TwoWire *wire = cfg.accelerometerWire ? cfg.accelerometerWire : &Wire;
    wire->beginTransmission(accelAddress);
    wire->write(0x28 | 0x80);
    if (wire->endTransmission(false) != 0) return -1;
    if (wire->requestFrom(static_cast<int>(accelAddress), 6) != 6) return -1;
    int16_t x = static_cast<int16_t>(wire->read() | (wire->read() << 8));
    int16_t y = static_cast<int16_t>(wire->read() | (wire->read() << 8));
    int16_t z = static_cast<int16_t>(wire->read() | (wire->read() << 8));
    x >>= 8;
    y >>= 8;
    z >>= 8;
    return static_cast<int32_t>(abs(x) + abs(y) + abs(z));
}
#endif

void updateAccelerometer() {
    bool movementDetected = cfg.motionDetectedCallback &&
                            cfg.motionDetectedCallback();
#if !FINDMYADV_DISABLE_BUILTIN_ACCELEROMETER
    if (accelOk) {
        const int32_t magnitude = accelerationMagnitude();
        if (magnitude >= 0) {
            uint16_t threshold = cfg.motionThreshold / 16;
            if (threshold == 0) threshold = 1;
            if (accelPrevious >= 0 &&
                abs(magnitude - accelPrevious) > threshold) {
                movementDetected = true;
            }
            accelPrevious = magnitude;
        }
    }
#endif
    if (movementDetected) {
        motionUntil = millis() + cfg.motionHoldMs;
    }
}

bool isMoving() {
    return (accelOk || cfg.motionDetectedCallback) &&
           static_cast<int32_t>(motionUntil - millis()) > 0;
}

uint32_t currentIntervalMs() {
    return isMoving() ? cfg.motionIntervalMs : cfg.advertisingIntervalMs;
}

#if FINDMYADV_USE_BLUEDROID
esp_power_level_t powerLevelFor(int8_t dbm) {
    if (dbm <= -12) return ESP_PWR_LVL_N12;
    if (dbm <= -9) return ESP_PWR_LVL_N9;
    if (dbm <= -6) return ESP_PWR_LVL_N6;
    if (dbm <= -3) return ESP_PWR_LVL_N3;
    if (dbm <= 0) return ESP_PWR_LVL_N0;
    if (dbm <= 3) return ESP_PWR_LVL_P3;
    if (dbm <= 6) return ESP_PWR_LVL_P6;
    return ESP_PWR_LVL_P9;
}
#endif

#if !FINDMYADV_USE_BLUEDROID
bool initializeBleStack() {
    if (!NimBLEDevice::isInitialized() && !NimBLEDevice::init("")) {
        return false;
    }
    NimBLEDevice::setPower(cfg.txPowerDbm, NimBLETxPowerType::Advertise);
    return true;
}

void startProvider(const uint8_t *address, uint8_t *data, uint8_t length) {
    activeInterval = currentIntervalMs();
    const uint16_t units = static_cast<uint16_t>((activeInterval * 8) / 5);
    ble_gap_adv_stop();

    uint8_t reversedAddress[6];
    for (uint8_t i = 0; i < sizeof(reversedAddress); ++i) {
        reversedAddress[i] = address[sizeof(reversedAddress) - 1 - i];
    }
    if (!NimBLEDevice::setOwnAddr(reversedAddress) ||
        !NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM) ||
        ble_gap_adv_set_data(data, length) != 0) {
        activeInterval = 0;
        return;
    }

    ble_gap_adv_params parameters{};
    parameters.conn_mode = BLE_GAP_CONN_MODE_NON;
    parameters.disc_mode = BLE_GAP_DISC_MODE_NON;
    parameters.itvl_min = units;
    parameters.itvl_max = units;
    if (ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, nullptr, BLE_HS_FOREVER,
                          &parameters, nullptr, nullptr) != 0) {
        activeInterval = 0;
    }
}

void stopAdvertising() {
    ble_gap_adv_stop();
}
#else
void configurePendingBluedroidAdvertisement() {
    if (!pendingRestart) return;
    if (esp_ble_gap_set_rand_addr(pendingAddress) != ESP_OK ||
        esp_ble_gap_config_adv_data_raw(pendingData,
                                        pendingDataLength) != ESP_OK) {
        pendingRestart = false;
        activeInterval = 0;
    }
}

void gapCallback(esp_gap_ble_cb_event_t event,
                 esp_ble_gap_cb_param_t *parameters) {
    switch (event) {
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            bluedroidAdvertising = false;
            configurePendingBluedroidAdvertisement();
            break;
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            if (parameters->adv_data_raw_cmpl.status == ESP_BT_STATUS_SUCCESS) {
                if (esp_ble_gap_start_advertising(&advParams) != ESP_OK) {
                    pendingRestart = false;
                    activeInterval = 0;
                }
            } else {
                pendingRestart = false;
                activeInterval = 0;
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            bluedroidAdvertising =
                parameters->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS;
            pendingRestart = false;
            if (!bluedroidAdvertising) activeInterval = 0;
            break;
        default:
            break;
    }
}

bool initializeBleStack() {
    esp_bt_controller_status_t controller = esp_bt_controller_get_status();
    if (controller == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_bt_controller_config_t controllerConfig = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        if (esp_bt_controller_init(&controllerConfig) != ESP_OK) return false;
        controller = esp_bt_controller_get_status();
    }
    if (controller == ESP_BT_CONTROLLER_STATUS_INITED &&
        esp_bt_controller_enable(ESP_BT_MODE_BLE) != ESP_OK) return false;

    esp_bluedroid_status_t bluedroid = esp_bluedroid_get_status();
    if (bluedroid == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        if (esp_bluedroid_init() != ESP_OK) return false;
        bluedroid = esp_bluedroid_get_status();
    }
    if (bluedroid == ESP_BLUEDROID_STATUS_INITIALIZED &&
        esp_bluedroid_enable() != ESP_OK) return false;

    if (esp_ble_gap_register_callback(gapCallback) != ESP_OK) return false;
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, powerLevelFor(cfg.txPowerDbm));
    return true;
}

void startProvider(const uint8_t *address, uint8_t *data, uint8_t length) {
    activeInterval = currentIntervalMs();
    const uint16_t units = static_cast<uint16_t>((activeInterval * 8) / 5);
    advParams.adv_int_min = units;
    advParams.adv_int_max = units;
    memcpy(pendingAddress, address, sizeof(pendingAddress));
    pendingDataLength = length < sizeof(pendingData)
        ? length : static_cast<uint8_t>(sizeof(pendingData));
    memcpy(pendingData, data, pendingDataLength);
    pendingRestart = true;
    if (!bluedroidAdvertising ||
        esp_ble_gap_stop_advertising() != ESP_OK) {
        bluedroidAdvertising = false;
        configurePendingBluedroidAdvertisement();
    }
}

void stopAdvertising() {
    pendingRestart = false;
    esp_ble_gap_stop_advertising();
}
#endif

void schedulerPoll() {
    if (!running) return;
    updateAccelerometer();

    const bool dualProvider = appleEnabled && googleEnabled;
    const bool switchWindow = millis() - lastSwitch >= cfg.providerWindowMs;
    if (!switchWindow && activeInterval == currentIntervalMs()) return;

    if (dualProvider) {
        if (switchWindow) selectedProvider ^= 1;
        if (selectedProvider == 0) {
            startProvider(appleAddr, appleAdv, sizeof(appleAdv));
        } else {
            startProvider(googleAddr, googleAdv, sizeof(googleAdv));
        }
    } else if (appleEnabled) {
        startProvider(appleAddr, appleAdv, sizeof(appleAdv));
    } else if (googleEnabled) {
        startProvider(googleAddr, googleAdv, sizeof(googleAdv));
    }
    if (switchWindow) lastSwitch = millis();
}

void schedulerTaskEntry(void *) {
    while (!stopRequested) {
        schedulerPoll();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    schedulerTask = nullptr;
    vTaskDelete(nullptr);
}

void resetRuntimeState() {
    activeInterval = 0;
    lastSwitch = millis();
    selectedProvider = 0;
    accelPrevious = -1;
    motionUntil = 0;
}

} // namespace

FindMyAdvClass FindMyAdv;

bool FindMyAdvClass::begin(const FindMyAdvConfig &configuration) {
    if (running) return true;

    if (!keyFitsInternalBuffer(configuration.appleAdvertisementKeyBase64) ||
        !keyFitsInternalBuffer(configuration.googleAdvertisementEidHex)) {
        currentStatus = FindMyAdvStatus::NoValidKey;
        errorText = "Advertising keys must not exceed 40 characters";
        return false;
    }

    cfg = configuration;
    copyKey(appleText, cfg.appleAdvertisementKeyBase64);
    copyKey(googleText, cfg.googleAdvertisementEidHex);
    cfg.appleAdvertisementKeyBase64 = appleText;
    cfg.googleAdvertisementEidHex = googleText;
    if (cfg.advertisingIntervalMs < 20 || cfg.advertisingIntervalMs > 10240)
        cfg.advertisingIntervalMs = 2000;
    if (cfg.motionIntervalMs < 20 || cfg.motionIntervalMs > 10240)
        cfg.motionIntervalMs = 200;
    if (cfg.providerWindowMs < 100) cfg.providerWindowMs = 5000;
    if (cfg.schedulerTaskStackBytes < 2048)
        cfg.schedulerTaskStackBytes = 2048;

    appleEnabled = decodeApple(appleText);
    googleEnabled = decodeGoogle(googleText);
    if (!appleEnabled && !googleEnabled) {
        currentStatus = FindMyAdvStatus::NoValidKey;
        errorText = "No valid Apple or Google advertising key";
        return false;
    }

    if (appleEnabled) buildAppleFrame();
    memcpy(googleAddr, &googleAdv[8], sizeof(googleAddr));
    googleAddr[0] |= 0xc0;
    initializeAccelerometer();
    if (!initializeBleStack()) {
        currentStatus = FindMyAdvStatus::BleInitializationFailed;
        errorText = "BLE stack initialization failed";
        return false;
    }

    resetRuntimeState();
    stopRequested = false;
    running = true;
    schedulerPoll();
#if !FINDMYADV_USE_BLUEDROID
    if (activeInterval == 0) {
        running = false;
        stopAdvertising();
        currentStatus = FindMyAdvStatus::BleInitializationFailed;
        errorText = "BLE advertising start failed";
        return false;
    }
#endif
    if (cfg.backgroundTask &&
        xTaskCreate(schedulerTaskEntry, "FindMyAdv",
                    cfg.schedulerTaskStackBytes, nullptr, 1,
                    &schedulerTask) != pdPASS) {
        running = false;
        stopAdvertising();
        currentStatus = FindMyAdvStatus::TaskCreationFailed;
        errorText = "Unable to create the FindMyAdv background task";
        return false;
    }

    currentStatus = FindMyAdvStatus::Running;
    errorText = "";
    return true;
}

bool FindMyAdvClass::reconfigure(const FindMyAdvConfig &configuration) {
    // The caller is allowed to pass pointers returned by its own settings
    // store, or even the values currently used by this library. Preserve both
    // strings before end()/begin() changes the internal buffers.
    if (!keyFitsInternalBuffer(configuration.appleAdvertisementKeyBase64) ||
        !keyFitsInternalBuffer(configuration.googleAdvertisementEidHex)) {
        currentStatus = FindMyAdvStatus::NoValidKey;
        errorText = "Advertising keys must not exceed 40 characters";
        return false;
    }
    char nextApple[41];
    char nextGoogle[41];
    copyKey(nextApple, configuration.appleAdvertisementKeyBase64);
    copyKey(nextGoogle, configuration.googleAdvertisementEidHex);
    FindMyAdvConfig next = configuration;
    next.appleAdvertisementKeyBase64 = nextApple;
    next.googleAdvertisementEidHex = nextGoogle;

    end();
    const uint32_t waitStarted = millis();
    while (schedulerTask && millis() - waitStarted < 1000) delay(10);
    if (schedulerTask) {
        vTaskDelete(schedulerTask);
        schedulerTask = nullptr;
    }
    return begin(next);
}

bool FindMyAdvClass::setKeys(const char *appleAdvertisementKeyBase64,
                             const char *googleAdvertisementEidHex) {
    FindMyAdvConfig next = cfg;
    next.appleAdvertisementKeyBase64 = appleAdvertisementKeyBase64;
    next.googleAdvertisementEidHex = googleAdvertisementEidHex;
    return reconfigure(next);
}

void FindMyAdvClass::end() {
    if (!running) return;
    stopRequested = true;
    running = false;
    stopAdvertising();
    currentStatus = FindMyAdvStatus::Stopped;
    errorText = "";
}

void FindMyAdvClass::poll() {
    schedulerPoll();
}

bool FindMyAdvClass::isRunning() const {
    return running;
}

bool FindMyAdvClass::isAppleEnabled() const {
    return appleEnabled;
}

bool FindMyAdvClass::isGoogleEnabled() const {
    return googleEnabled;
}

bool FindMyAdvClass::isAccelerometerDetected() const {
    return accelOk;
}

bool FindMyAdvClass::isMotionModeActive() const {
    return isMoving();
}

uint32_t FindMyAdvClass::activeAdvertisingIntervalMs() const {
    return activeInterval;
}

FindMyAdvStatus FindMyAdvClass::status() const {
    return currentStatus;
}

const char *FindMyAdvClass::lastError() const {
    return errorText;
}

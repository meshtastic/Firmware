/**
 * @file Power.cpp
 * @brief This file contains the implementation of the Power class, which is responsible for managing power-related functionality
 * of the device. It includes battery level sensing, power management unit (PMU) control, and power state machine management. The
 * Power class is used by the main device class to manage power-related functionality.
 *
 * The file also includes implementations of various battery level sensors, such as the AnalogBatteryLevel class, which assumes
 * the battery voltage is attached via a voltage-divider to an analog input.
 *
 * This file is part of the Meshtastic project.
 * For more information, see: https://meshtastic.org/
 */
#include "power.h"
#include "NodeDB.h"
#include "PowerFSM.h"
#include "buzz/buzz.h"
#include "configuration.h"
#include "main.h"
#include "meshUtils.h"
#include "sleep.h"

// Working USB detection for powered/charging states on the RAK platform
#ifdef NRF_APM
#include "nrfx_power.h"
#endif

#if defined(DEBUG_HEAP_MQTT) && !MESHTASTIC_EXCLUDE_MQTT
#include "mqtt/MQTT.h"
#include "target_specific.h"
#if HAS_WIFI
#include <WiFi.h>
#endif
#endif

#ifndef DELAY_FOREVER
#define DELAY_FOREVER portMAX_DELAY
#endif

#if defined(BATTERY_PIN) && defined(ARCH_ESP32)

#ifndef BAT_MEASURE_ADC_UNIT // ADC1 is default
static const adc1_channel_t adc_channel = ADC_CHANNEL;
static const adc_unit_t unit = ADC_UNIT_1;
#else // ADC2
static const adc2_channel_t adc_channel = ADC_CHANNEL;
static const adc_unit_t unit = ADC_UNIT_2;
RTC_NOINIT_ATTR uint64_t RTC_reg_b;

#endif // BAT_MEASURE_ADC_UNIT

esp_adc_cal_characteristics_t *adc_characs = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
#ifndef ADC_ATTENUATION
static const adc_atten_t atten = ADC_ATTEN_DB_12;
#else
static const adc_atten_t atten = ADC_ATTENUATION;
#endif
#endif // BATTERY_PIN && ARCH_ESP32

#ifdef EXT_CHRG_DETECT
#ifndef EXT_CHRG_DETECT_MODE
static const uint8_t ext_chrg_detect_mode = INPUT;
#else
static const uint8_t ext_chrg_detect_mode = EXT_CHRG_DETECT_MODE;
#endif
#ifndef EXT_CHRG_DETECT_VALUE
static const uint8_t ext_chrg_detect_value = HIGH;
#else
static const uint8_t ext_chrg_detect_value = EXT_CHRG_DETECT_VALUE;
#endif
#endif

#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && !defined(ARCH_PORTDUINO)
INA260Sensor ina260Sensor;
INA219Sensor ina219Sensor;
INA3221Sensor ina3221Sensor;
#endif

#if HAS_RAKPROT && !defined(ARCH_PORTDUINO)
RAK9154Sensor rak9154Sensor;
#endif

#ifdef HAS_PMU
XPowersLibInterface *PMU = NULL;
#else

// Copy of the base class defined in axp20x.h.
// I'd rather not include axp20x.h as it brings Wire dependency.
class HasBatteryLevel
{
  public:
    /**
     * Battery state of charge, from 0 to 100 or -1 for unknown
     */
    virtual int getBatteryPercent() { return -1; }

    /**
     * The raw voltage of the battery or NAN if unknown
     */
    virtual uint16_t getBattVoltage() { return 0; }

    /**
     * return true if there is a battery installed in this unit
     */
    virtual bool isBatteryConnect() { return false; }

    virtual bool isVbusIn() { return false; }
    virtual bool isCharging() { return false; }
};
#endif

bool pmu_irq = false;

Power *power;

using namespace meshtastic;

#ifndef AREF_VOLTAGE
#if defined(ARCH_NRF52)
/*
 * Internal Reference is +/-0.6V, with an adjustable gain of 1/6, 1/5, 1/4,
 * 1/3, 1/2 or 1, meaning 3.6, 3.0, 2.4, 1.8, 1.2 or 0.6V for the ADC levels.
 *
 * External Reference is VDD/4, with an adjustable gain of 1, 2 or 4, meaning
 * VDD/4, VDD/2 or VDD for the ADC levels.
 *
 * Default settings are internal reference with 1/6 gain (GND..3.6V ADC range)
 */
#define AREF_VOLTAGE 3.6
#else
#define AREF_VOLTAGE 3.3
#endif
#endif

/**
 * If this board has a battery level sensor, set this to a valid implementation
 */
static HasBatteryLevel *batteryLevel; // Default to NULL for no battery level sensor

/**
 * A simple battery level sensor that assumes the battery voltage is attached via a voltage-divider to an analog input
 */
class AnalogBatteryLevel : public HasBatteryLevel
{
    /**
     * Battery state of charge, from 0 to 100 or -1 for unknown
     */
    virtual int getBatteryPercent() override
    {
#if defined(HAS_RAKPROT) && !defined(ARCH_PORTDUINO) && !defined(HAS_PMU)
        if (hasRAK()) {
            return rak9154Sensor.getBusBatteryPercent();
        }
#endif

        float v = getBattVoltage();

        if (v < noBatVolt)
            return -1; // If voltage is super low assume no battery installed

#ifdef NO_BATTERY_LEVEL_ON_CHARGE
        // This does not work on a RAK4631 with battery connected
        if (v > chargingVolt)
            return 0; // While charging we can't report % full on the battery
#endif
        /**
         * @brief   Battery voltage lookup table interpolation to obtain a more
         * precise percentage rather than the old proportional one.
         * @author  Gabriele Russo
         * @date    06/02/2024
         */
        float battery_SOC = 0.0;
        uint16_t voltage = v / NUM_CELLS; // single cell voltage (average)
        for (int i = 0; i < NUM_OCV_POINTS; i++) {
            if (OCV[i] <= voltage) {
                if (i == 0) {
                    battery_SOC = 100.0; // 100% full
                } else {
                    // interpolate between OCV[i] and OCV[i-1]
                    battery_SOC = (float)100.0 / (NUM_OCV_POINTS - 1.0) *
                                  (NUM_OCV_POINTS - 1.0 - i + ((float)voltage - OCV[i]) / (OCV[i - 1] - OCV[i]));
                }
                break;
            }
        }
        return clamp((int)(battery_SOC), 0, 100);
    }

    /**
     * The raw voltage of the batteryin millivolts or NAN if unknown
     */
    virtual uint16_t getBattVoltage() override
    {

#if defined(HAS_RAKPROT) && !defined(ARCH_PORTDUINO) && !defined(HAS_PMU)
        if (hasRAK()) {
            return getRAKVoltage();
        }
#endif

#if HAS_TELEMETRY && !defined(ARCH_PORTDUINO) && !defined(ARCH_STM32WL) && !defined(HAS_PMU) &&                                  \
    !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR
        if (hasINA()) {
            LOG_DEBUG("Using INA on I2C addr 0x%x for device battery voltage\n", config.power.device_battery_ina_address);
            return getINAVoltage();
        }
#endif

#ifndef ADC_MULTIPLIER
#define ADC_MULTIPLIER 2.0
#endif

#ifndef BATTERY_SENSE_SAMPLES
#define BATTERY_SENSE_SAMPLES                                                                                                    \
    15 // Set the number of samples, it has an effect of increasing sensitivity in complex electromagnetic environment.
#endif

#ifdef BATTERY_PIN
        // Override variant or default ADC_MULTIPLIER if we have the override pref
        float operativeAdcMultiplier =
            config.power.adc_multiplier_override > 0 ? config.power.adc_multiplier_override : ADC_MULTIPLIER;
        // Do not call analogRead() often.
        const uint32_t min_read_interval = 5000;
        if (millis() - last_read_time_ms > min_read_interval) {
            last_read_time_ms = millis();

            uint32_t raw = 0;
            float scaled = 0;

#ifdef ARCH_ESP32 // ADC block for espressif platforms
            raw = espAdcRead();
            scaled = esp_adc_cal_raw_to_voltage(raw, adc_characs);
            scaled *= operativeAdcMultiplier;
#else           // block for all other platforms
#ifdef ADC_CTRL // enable adc voltage divider when we need to read
            pinMode(ADC_CTRL, OUTPUT);
            digitalWrite(ADC_CTRL, ADC_CTRL_ENABLED);
            delay(10);
#endif
            for (uint32_t i = 0; i < BATTERY_SENSE_SAMPLES; i++) {
                raw += analogRead(BATTERY_PIN);
            }
            raw = raw / BATTERY_SENSE_SAMPLES;
            scaled = operativeAdcMultiplier * ((1000 * AREF_VOLTAGE) / pow(2, BATTERY_SENSE_RESOLUTION_BITS)) * raw;
#ifdef ADC_CTRL // disable adc voltage divider when we need to read
            digitalWrite(ADC_CTRL, !ADC_CTRL_ENABLED);
#endif
#endif

            if (!initial_read_done) {
                // Flush the smoothing filter with an ADC reading, if the reading is plausibly correct
                if (scaled > last_read_value)
                    last_read_value = scaled;
                initial_read_done = true;
            } else {
                // Already initialized - filter this reading
                last_read_value += (scaled - last_read_value) * 0.5; // Virtual LPF
            }

            // LOG_DEBUG("battery gpio %d raw val=%u scaled=%u filtered=%u\n", BATTERY_PIN, raw, (uint32_t)(scaled), (uint32_t)
            // (last_read_value));
        }
        return last_read_value;
#endif // BATTERY_PIN
        return 0;
    }

#if defined(ARCH_ESP32) && !defined(HAS_PMU) && defined(BATTERY_PIN)
    /**
     * ESP32 specific function for getting calibrated ADC reads
     */
    uint32_t espAdcRead()
    {

        uint32_t raw = 0;
        uint8_t raw_c = 0; // raw reading counter

#ifndef BAT_MEASURE_ADC_UNIT // ADC1
#ifdef ADC_CTRL              // enable adc voltage divider when we need to read
        pinMode(ADC_CTRL, OUTPUT);
        digitalWrite(ADC_CTRL, ADC_CTRL_ENABLED);
        delay(10);
#endif
        for (int i = 0; i < BATTERY_SENSE_SAMPLES; i++) {
            int val_ = adc1_get_raw(adc_channel);
            if (val_ >= 0) { // save only valid readings
                raw += val_;
                raw_c++;
            }
            // delayMicroseconds(100);
        }
#ifdef ADC_CTRL // disable adc voltage divider when we need to read
        digitalWrite(ADC_CTRL, !ADC_CTRL_ENABLED);
#endif
#else // ADC2
#ifdef ADC_CTRL
#if defined(HELTEC_WIRELESS_PAPER) || defined(HELTEC_WIRELESS_PAPER_V1_0)
        pinMode(ADC_CTRL, OUTPUT);
        digitalWrite(ADC_CTRL, LOW); // ACTIVE LOW
        delay(10);
#endif
#endif // End ADC_CTRL

#ifdef CONFIG_IDF_TARGET_ESP32S3 // ESP32S3
        // ADC2 wifi bug workaround not required, breaks compile
        // On ESP32S3, ADC2 can take turns with Wifi (?)

        int32_t adc_buf;
        esp_err_t read_result;

        // Multiple samples
        for (int i = 0; i < BATTERY_SENSE_SAMPLES; i++) {
            adc_buf = 0;
            read_result = -1;

            read_result = adc2_get_raw(adc_channel, ADC_WIDTH_BIT_12, &adc_buf);
            if (read_result == ESP_OK) {
                raw += adc_buf;
                raw_c++; // Count valid samples
            } else {
                LOG_DEBUG("An attempt to sample ADC2 failed\n");
            }
        }

#else  // Other ESP32
        int32_t adc_buf = 0;
        for (int i = 0; i < BATTERY_SENSE_SAMPLES; i++) {
            // ADC2 wifi bug workaround, see
            // https://github.com/espressif/arduino-esp32/issues/102
            WRITE_PERI_REG(SENS_SAR_READ_CTRL2_REG, RTC_reg_b);
            SET_PERI_REG_MASK(SENS_SAR_READ_CTRL2_REG, SENS_SAR2_DATA_INV);
            adc2_get_raw(adc_channel, ADC_WIDTH_BIT_12, &adc_buf);
            raw += adc_buf;
            raw_c++;
        }
#endif // BAT_MEASURE_ADC_UNIT

#ifdef ADC_CTRL
#if defined(HELTEC_WIRELESS_PAPER) || defined(HELTEC_WIRELESS_PAPER_V1_0)
        digitalWrite(ADC_CTRL, HIGH);
#endif
#endif // End ADC_CTRL

#endif // End BAT_MEASURE_ADC_UNIT
        return (raw / (raw_c < 1 ? 1 : raw_c));
    }
#endif

    /**
     * return true if there is a battery installed in this unit
     */
    virtual bool isBatteryConnect() override { return getBatteryPercent() != -1; }

    /// If we see a battery voltage higher than physics allows - assume charger is pumping
    /// in power
    /// On some boards we don't have the power management chip (like AXPxxxx)
    /// so we use EXT_PWR_DETECT GPIO pin to detect external power source
    virtual bool isVbusIn() override
    {
#ifdef EXT_PWR_DETECT
#ifdef HELTEC_CAPSULE_SENSOR_V3
        // if external powered that pin will be pulled down
        if (digitalRead(EXT_PWR_DETECT) == LOW) {
            return true;
        }
        // if it's not LOW - check the battery
#else
        // if external powered that pin will be pulled up
        if (digitalRead(EXT_PWR_DETECT) == HIGH) {
            return true;
        }
        // if it's not HIGH - check the battery
#endif
#endif
        return getBattVoltage() > chargingVolt;
    }

    /// Assume charging if we have a battery and external power is connected.
    /// we can't be smart enough to say 'full'?
    virtual bool isCharging() override
    {
#if defined(HAS_RAKPROT) && !defined(ARCH_PORTDUINO) && !defined(HAS_PMU)
        if (hasRAK()) {
            return (rak9154Sensor.isCharging()) ? OptTrue : OptFalse;
        }
#endif
#ifdef EXT_CHRG_DETECT
        return digitalRead(EXT_CHRG_DETECT) == ext_chrg_detect_value;
#else
        return isBatteryConnect() && isVbusIn();
#endif
    }

  private:
    /// If we see a battery voltage higher than physics allows - assume charger is pumping
    /// in power

    /// For heltecs with no battery connected, the measured voltage is 2204, so
    // need to be higher than that, in this case is 2500mV (3000-500)
    const uint16_t OCV[NUM_OCV_POINTS] = {OCV_ARRAY};
    const float chargingVolt = (OCV[0] + 10) * NUM_CELLS;
    const float noBatVolt = (OCV[NUM_OCV_POINTS - 1] - 500) * NUM_CELLS;
    // Start value from minimum voltage for the filter to not start from 0
    // that could trigger some events.
    // This value is over-written by the first ADC reading, it the voltage seems reasonable.
    bool initial_read_done = false;
    float last_read_value = (OCV[NUM_OCV_POINTS - 1] * NUM_CELLS);
    uint32_t last_read_time_ms = 0;

#if defined(HAS_RAKPROT)

    uint16_t getRAKVoltage() { return rak9154Sensor.getBusVoltageMv(); }

    bool hasRAK()
    {
        if (!rak9154Sensor.isInitialized())
            return rak9154Sensor.runOnce() > 0;
        return rak9154Sensor.isRunning();
    }
#endif

#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && !defined(ARCH_PORTDUINO) && !defined(ARCH_STM32WL)
    uint16_t getINAVoltage()
    {
        if (nodeTelemetrySensorsMap[meshtastic_TelemetrySensorType_INA219].first == config.power.device_battery_ina_address) {
            return ina219Sensor.getBusVoltageMv();
        } else if (nodeTelemetrySensorsMap[meshtastic_TelemetrySensorType_INA260].first ==
                   config.power.device_battery_ina_address) {
            return ina260Sensor.getBusVoltageMv();
        } else if (nodeTelemetrySensorsMap[meshtastic_TelemetrySensorType_INA3221].first ==
                   config.power.device_battery_ina_address) {
            return ina3221Sensor.getBusVoltageMv();
        }
        return 0;
    }

    bool hasINA()
    {
        if (!config.power.device_battery_ina_address) {
            return false;
        }
        if (nodeTelemetrySensorsMap[meshtastic_TelemetrySensorType_INA219].first == config.power.device_battery_ina_address) {
            if (!ina219Sensor.isInitialized())
                return ina219Sensor.runOnce() > 0;
            return ina219Sensor.isRunning();
        } else if (nodeTelemetrySensorsMap[meshtastic_TelemetrySensorType_INA260].first ==
                   config.power.device_battery_ina_address) {
            if (!ina260Sensor.isInitialized())
                return ina260Sensor.runOnce() > 0;
            return ina260Sensor.isRunning();
        } else if (nodeTelemetrySensorsMap[meshtastic_TelemetrySensorType_INA3221].first ==
                   config.power.device_battery_ina_address) {
            if (!ina3221Sensor.isInitialized())
                return ina3221Sensor.runOnce() > 0;
            return ina3221Sensor.isRunning();
        }
        return false;
    }
#endif
};

AnalogBatteryLevel analogLevel;

Power::Power() : OSThread("Power")
{
    statusHandler = {};
    low_voltage_counter = 0;
#ifdef DEBUG_HEAP
    lastheap = memGet.getFreeHeap();
#endif
}

bool Power::analogInit()
{
#ifdef EXT_PWR_DETECT
#ifdef HELTEC_CAPSULE_SENSOR_V3
    pinMode(EXT_PWR_DETECT, INPUT_PULLUP);
#else
    pinMode(EXT_PWR_DETECT, INPUT);
#endif
#endif
#ifdef EXT_CHRG_DETECT
    pinMode(EXT_CHRG_DETECT, ext_chrg_detect_mode);
#endif

#ifdef BATTERY_PIN
    LOG_DEBUG("Using analog input %d for battery level\n", BATTERY_PIN);

    // disable any internal pullups
    pinMode(BATTERY_PIN, INPUT);

#ifndef BATTERY_SENSE_RESOLUTION_BITS
#define BATTERY_SENSE_RESOLUTION_BITS 10
#endif

#ifdef ARCH_ESP32 // ESP32 needs special analog stuff

#ifndef ADC_WIDTH // max resolution by default
    static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
#else
    static const adc_bits_width_t width = ADC_WIDTH;
#endif
#ifndef BAT_MEASURE_ADC_UNIT // ADC1
    adc1_config_width(width);
    adc1_config_channel_atten(adc_channel, atten);
#else // ADC2
    adc2_config_channel_atten(adc_channel, atten);
#ifndef CONFIG_IDF_TARGET_ESP32S3
    // ADC2 wifi bug workaround
    // Not required with ESP32S3, breaks compile
    RTC_reg_b = READ_PERI_REG(SENS_SAR_READ_CTRL2_REG);
#endif
#endif
    // calibrate ADC
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_characs);
    // show ADC characterization base
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        LOG_INFO("ADCmod: ADC characterization based on Two Point values stored in eFuse\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        LOG_INFO("ADCmod: ADC characterization based on reference voltage stored in eFuse\n");
    }
#ifdef CONFIG_IDF_TARGET_ESP32S3
    // ESP32S3
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP_FIT) {
        LOG_INFO("ADCmod: ADC Characterization based on Two Point values and fitting curve coefficients stored in eFuse\n");
    }
#endif
    else {
        LOG_INFO("ADCmod: ADC characterization based on default reference voltage\n");
    }
#endif // ARCH_ESP32

#ifdef ARCH_NRF52
#ifdef VBAT_AR_INTERNAL
    analogReference(VBAT_AR_INTERNAL);
#else
    analogReference(AR_INTERNAL); // 3.6V
#endif
#endif // ARCH_NRF52

#ifndef ARCH_ESP32
    analogReadResolution(BATTERY_SENSE_RESOLUTION_BITS);
#endif

    batteryLevel = &analogLevel;
    return true;
#else
    return false;
#endif
}

/**
 * Initializes the Power class.
 *
 * @return true if the setup was successful, false otherwise.
 */
bool Power::setup()
{
    bool found = axpChipInit() || analogInit();

    enabled = found;
    low_voltage_counter = 0;

    return found;
}

void Power::shutdown()
{
    LOG_INFO("Shutting down\n");

#if defined(ARCH_NRF52) || defined(ARCH_ESP32)
#ifdef PIN_LED1
    ledOff(PIN_LED1);
#endif
#ifdef PIN_LED2
    ledOff(PIN_LED2);
#endif
#ifdef PIN_LED3
    ledOff(PIN_LED3);
#endif
    doDeepSleep(DELAY_FOREVER, false);
#endif
}

/// Reads power status to powerStatus singleton.
//
// TODO(girts): move this and other axp stuff to power.h/power.cpp.
void Power::readPowerStatus()
{
    if (batteryLevel) {
        bool hasBattery = batteryLevel->isBatteryConnect();
        uint32_t batteryVoltageMv = 0;
        int8_t batteryChargePercent = 0;
        if (hasBattery) {
            batteryVoltageMv = batteryLevel->getBattVoltage();
            // If the AXP192 returns a valid battery percentage, use it
            if (batteryLevel->getBatteryPercent() >= 0) {
                batteryChargePercent = batteryLevel->getBatteryPercent();
            } else {
                // If the AXP192 returns a percentage less than 0, the feature is either not supported or there is an error
                // In that case, we compute an estimate of the charge percent based on open circuite voltage table defined
                // in power.h
                batteryChargePercent = clamp((int)(((batteryVoltageMv - (OCV[NUM_OCV_POINTS - 1] * NUM_CELLS)) * 1e2) /
                                                   ((OCV[0] * NUM_CELLS) - (OCV[NUM_OCV_POINTS - 1] * NUM_CELLS))),
                                             0, 100);
            }
        }

        OptionalBool NRF_USB = OptFalse;

#ifdef NRF_APM // Section of code detects USB power on the RAK4631 and updates the power states.  Takes 20 seconds or so to detect
               // changes.

        static nrfx_power_usb_state_t prev_nrf_usb_state = (nrfx_power_usb_state_t)-1; // -1 so that state detected at boot
        nrfx_power_usb_state_t nrf_usb_state = nrfx_power_usbstatus_get();

        // If state changed
        if (nrf_usb_state != prev_nrf_usb_state) {
            // If changed to DISCONNECTED
            if (nrf_usb_state == NRFX_POWER_USB_STATE_DISCONNECTED) {
                powerFSM.trigger(EVENT_POWER_DISCONNECTED);
                NRF_USB = OptFalse;
            }
            // If changed to CONNECTED / READY
            else {
                powerFSM.trigger(EVENT_POWER_CONNECTED);
                NRF_USB = OptTrue;
            }

            // Cache the current state
            prev_nrf_usb_state = nrf_usb_state;
        }
#endif
        // Notify any status instances that are observing us
        const PowerStatus powerStatus2 = PowerStatus(
            hasBattery ? OptTrue : OptFalse, batteryLevel->isVbusIn() || NRF_USB == OptTrue ? OptTrue : OptFalse,
            batteryLevel->isCharging() || NRF_USB == OptTrue ? OptTrue : OptFalse, batteryVoltageMv, batteryChargePercent);
        LOG_DEBUG("Battery: usbPower=%d, isCharging=%d, batMv=%d, batPct=%d\n", powerStatus2.getHasUSB(),
                  powerStatus2.getIsCharging(), powerStatus2.getBatteryVoltageMv(), powerStatus2.getBatteryChargePercent());
        newStatus.notifyObservers(&powerStatus2);
#ifdef DEBUG_HEAP
        if (lastheap != memGet.getFreeHeap()) {
            LOG_DEBUG("Threads running:");
            int running = 0;
            for (int i = 0; i < MAX_THREADS; i++) {
                auto thread = concurrency::mainController.get(i);
                if ((thread != nullptr) && (thread->enabled)) {
                    LOG_DEBUG(" %s", thread->ThreadName.c_str());
                    running++;
                }
            }
            LOG_DEBUG("\n");
            LOG_DEBUG("Heap status: %d/%d bytes free (%d), running %d/%d threads\n", memGet.getFreeHeap(), memGet.getHeapSize(),
                      memGet.getFreeHeap() - lastheap, running, concurrency::mainController.size(false));
            lastheap = memGet.getFreeHeap();
        }
#ifdef DEBUG_HEAP_MQTT
        if (mqtt) {
            // send MQTT-Packet with Heap-Size
            uint8_t dmac[6];
            getMacAddr(dmac); // Get our hardware ID
            char mac[18];
            sprintf(mac, "!%02x%02x%02x%02x", dmac[2], dmac[3], dmac[4], dmac[5]);

            auto newHeap = memGet.getFreeHeap();
            std::string heapTopic =
                (*moduleConfig.mqtt.root ? moduleConfig.mqtt.root : "msh") + std::string("/2/heap/") + std::string(mac);
            std::string heapString = std::to_string(newHeap);
            mqtt->pubSub.publish(heapTopic.c_str(), heapString.c_str(), false);
            auto wifiRSSI = WiFi.RSSI();
            std::string wifiTopic =
                (*moduleConfig.mqtt.root ? moduleConfig.mqtt.root : "msh") + std::string("/2/wifi/") + std::string(mac);
            std::string wifiString = std::to_string(wifiRSSI);
            mqtt->pubSub.publish(wifiTopic.c_str(), wifiString.c_str(), false);
        }
#endif

#endif

        // If we have a battery at all and it is less than 0%, force deep sleep if we have more than 10 low readings in
        // a row. NOTE: min LiIon/LiPo voltage is 2.0 to 2.5V, current OCV min is set to 3100 that is large enough.
        //
        if (powerStatus2.getHasBattery() && !powerStatus2.getHasUSB()) {
            if (batteryLevel->getBattVoltage() < OCV[NUM_OCV_POINTS - 1]) {
                low_voltage_counter++;
                LOG_DEBUG("Low voltage counter: %d/10\n", low_voltage_counter);
                if (low_voltage_counter > 10) {
#ifdef ARCH_NRF52
                    // We can't trigger deep sleep on NRF52, it's freezing the board
                    LOG_DEBUG("Low voltage detected, but not triggering deep sleep\n");
#else
                    LOG_INFO("Low voltage detected, triggering deep sleep\n");
                    powerFSM.trigger(EVENT_LOW_BATTERY);
#endif
                }
            } else {
                low_voltage_counter = 0;
            }
        }
    } else {
        // No power sensing on this board - tell everyone else we have no idea what is happening
        const PowerStatus powerStatus3 = PowerStatus(OptUnknown, OptUnknown, OptUnknown, -1, -1);
        newStatus.notifyObservers(&powerStatus3);
    }
}

int32_t Power::runOnce()
{
    readPowerStatus();

#ifdef HAS_PMU
    // WE no longer use the IRQ line to wake the CPU (due to false wakes from sleep), but we do poll
    // the IRQ status by reading the registers over I2C
    if (PMU) {

        PMU->getIrqStatus();

        if (PMU->isVbusRemoveIrq()) {
            LOG_INFO("USB unplugged\n");
            powerFSM.trigger(EVENT_POWER_DISCONNECTED);
        }

        if (PMU->isVbusInsertIrq()) {
            LOG_INFO("USB plugged In\n");
            powerFSM.trigger(EVENT_POWER_CONNECTED);
        }

        /*
        Other things we could check if we cared...

        if (PMU->isBatChagerStartIrq()) {
            LOG_DEBUG("Battery start charging\n");
        }
        if (PMU->isBatChagerDoneIrq()) {
            LOG_DEBUG("Battery fully charged\n");
        }
        if (PMU->isBatInsertIrq()) {
            LOG_DEBUG("Battery inserted\n");
        }
        if (PMU->isBatRemoveIrq()) {
            LOG_DEBUG("Battery removed\n");
        }
        */
#ifndef T_WATCH_S3 // FIXME - why is this triggering on the T-Watch S3?
        if (PMU->isPekeyLongPressIrq()) {
            LOG_DEBUG("PEK long button press\n");
            screen->setOn(false);
        }
#endif

        PMU->clearIrqStatus();
    }
#endif
    // Only read once every 20 seconds once the power status for the app has been initialized
    return (statusHandler && statusHandler->isInitialized()) ? (1000 * 20) : RUN_SAME;
}

/**
 * Init the power manager chip
 *
 * axp192 power
    DCDC1 0.7-3.5V @ 1200mA max -> OLED // If you turn this off you'll lose comms to the axp192 because the OLED and the
 axp192 share the same i2c bus, instead use ssd1306 sleep mode DCDC2 -> unused DCDC3 0.7-3.5V @ 700mA max -> ESP32 (keep this
 on!) LDO1 30mA -> charges GPS backup battery // charges the tiny J13 battery by the GPS to power the GPS ram (for a couple of
 days), can not be turned off LDO2 200mA -> LORA LDO3 200mA -> GPS
 *
 */
bool Power::axpChipInit()
{

#ifdef HAS_PMU

    TwoWire *w = NULL;

    // Use macro to distinguish which wire is used by PMU
#ifdef PMU_USE_WIRE1
    w = &Wire1;
#else
    w = &Wire;
#endif

    /**
     * It is not necessary to specify the wire pin,
     * just input the wire, because the wire has been initialized in main.cpp
     */
    if (!PMU) {
        PMU = new XPowersAXP2101(*w);
        if (!PMU->init()) {
            LOG_WARN("Failed to find AXP2101 power management\n");
            delete PMU;
            PMU = NULL;
        } else {
            LOG_INFO("AXP2101 PMU init succeeded, using AXP2101 PMU\n");
        }
    }

    if (!PMU) {
        PMU = new XPowersAXP192(*w);
        if (!PMU->init()) {
            LOG_WARN("Failed to find AXP192 power management\n");
            delete PMU;
            PMU = NULL;
        } else {
            LOG_INFO("AXP192 PMU init succeeded, using AXP192 PMU\n");
        }
    }

    if (!PMU) {
        /*
         * In XPowersLib, if the XPowersAXPxxx object is released, Wire.end() will be called at the same time.
         * In order not to affect other devices, if the initialization of the PMU fails, Wire needs to be re-initialized once,
         * if there are multiple devices sharing the bus.
         * * */
#ifndef PMU_USE_WIRE1
        w->begin(I2C_SDA, I2C_SCL);
#endif
        return false;
    }

    batteryLevel = PMU;

    if (PMU->getChipModel() == XPOWERS_AXP192) {

        // lora radio power channel
        PMU->setPowerChannelVoltage(XPOWERS_LDO2, 3300);
        PMU->enablePowerOutput(XPOWERS_LDO2);

        // oled module power channel,
        // disable it will cause abnormal communication between boot and AXP power supply,
        // do not turn it off
        PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
        // enable oled power
        PMU->enablePowerOutput(XPOWERS_DCDC1);

        // gnss module power channel -  now turned on in setGpsPower
        PMU->setPowerChannelVoltage(XPOWERS_LDO3, 3300);
        // PMU->enablePowerOutput(XPOWERS_LDO3);

        // protected oled power source
        PMU->setProtectedChannel(XPOWERS_DCDC1);
        // protected esp32 power source
        PMU->setProtectedChannel(XPOWERS_DCDC3);

        // disable not use channel
        PMU->disablePowerOutput(XPOWERS_DCDC2);

        // disable all axp chip interrupt
        PMU->disableIRQ(XPOWERS_AXP192_ALL_IRQ);

        // Set constant current charging current
        PMU->setChargerConstantCurr(XPOWERS_AXP192_CHG_CUR_450MA);

        // Set up the charging voltage
        PMU->setChargeTargetVoltage(XPOWERS_AXP192_CHG_VOL_4V2);
    } else if (PMU->getChipModel() == XPOWERS_AXP2101) {

        /*The alternative version of T-Beam 1.1 differs from T-Beam V1.1 in that it uses an AXP2101 power chip*/
        if (HW_VENDOR == meshtastic_HardwareModel_TBEAM) {
            // Unuse power channel
            PMU->disablePowerOutput(XPOWERS_DCDC2);
            PMU->disablePowerOutput(XPOWERS_DCDC3);
            PMU->disablePowerOutput(XPOWERS_DCDC4);
            PMU->disablePowerOutput(XPOWERS_DCDC5);
            PMU->disablePowerOutput(XPOWERS_ALDO1);
            PMU->disablePowerOutput(XPOWERS_ALDO4);
            PMU->disablePowerOutput(XPOWERS_BLDO1);
            PMU->disablePowerOutput(XPOWERS_BLDO2);
            PMU->disablePowerOutput(XPOWERS_DLDO1);
            PMU->disablePowerOutput(XPOWERS_DLDO2);

            // GNSS RTC PowerVDD 3300mV
            PMU->setPowerChannelVoltage(XPOWERS_VBACKUP, 3300);
            PMU->enablePowerOutput(XPOWERS_VBACKUP);

            // ESP32 VDD 3300mV
            //  ! No need to set, automatically open , Don't close it
            //  PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
            //  PMU->setProtectedChannel(XPOWERS_DCDC1);

            // LoRa VDD 3300mV
            PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
            PMU->enablePowerOutput(XPOWERS_ALDO2);

            // GNSS VDD 3300mV
            PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
            PMU->enablePowerOutput(XPOWERS_ALDO3);
        } else if (HW_VENDOR == meshtastic_HardwareModel_LILYGO_TBEAM_S3_CORE ||
                   HW_VENDOR == meshtastic_HardwareModel_T_WATCH_S3) {
            // t-beam s3 core
            /**
             * gnss module power channel
             * The default ALDO4 is off, you need to turn on the GNSS power first, otherwise it will be invalid during
             * initialization
             */
            PMU->setPowerChannelVoltage(XPOWERS_ALDO4, 3300);
            PMU->enablePowerOutput(XPOWERS_ALDO4);

            // lora radio power channel
            PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
            PMU->enablePowerOutput(XPOWERS_ALDO3);

            // m.2 interface
            PMU->setPowerChannelVoltage(XPOWERS_DCDC3, 3300);
            PMU->enablePowerOutput(XPOWERS_DCDC3);

            /**
             * ALDO2 cannot be turned off.
             * It is a necessary condition for sensor communication.
             * It must be turned on to properly access the sensor and screen
             * It is also responsible for the power supply of PCF8563
             */
            PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
            PMU->enablePowerOutput(XPOWERS_ALDO2);

            // 6-axis , magnetometer ,bme280 , oled screen power channel
            PMU->setPowerChannelVoltage(XPOWERS_ALDO1, 3300);
            PMU->enablePowerOutput(XPOWERS_ALDO1);

            // sdcard power channel
            PMU->setPowerChannelVoltage(XPOWERS_BLDO1, 3300);
            PMU->enablePowerOutput(XPOWERS_BLDO1);

#ifdef T_WATCH_S3
            // DRV2605 power channel
            PMU->setPowerChannelVoltage(XPOWERS_BLDO2, 3300);
            PMU->enablePowerOutput(XPOWERS_BLDO2);
#endif

            // PMU->setPowerChannelVoltage(XPOWERS_DCDC4, 3300);
            // PMU->enablePowerOutput(XPOWERS_DCDC4);

            // not use channel
            PMU->disablePowerOutput(XPOWERS_DCDC2); // not elicited
            PMU->disablePowerOutput(XPOWERS_DCDC5); // not elicited
            PMU->disablePowerOutput(XPOWERS_DLDO1); // Invalid power channel, it does not exist
            PMU->disablePowerOutput(XPOWERS_DLDO2); // Invalid power channel, it does not exist
            PMU->disablePowerOutput(XPOWERS_VBACKUP);
        }

        // disable all axp chip interrupt
        PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

        // Set the constant current charging current of AXP2101, temporarily use 500mA by default
        PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);

        // Set up the charging voltage
        PMU->setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
    }

    PMU->clearIrqStatus();

    // TBeam1.1 /T-Beam S3-Core has no external TS detection,
    // it needs to be disabled, otherwise it will cause abnormal charging
    PMU->disableTSPinMeasure();

    // PMU->enableSystemVoltageMeasure();
    PMU->enableVbusVoltageMeasure();
    PMU->enableBattVoltageMeasure();

    LOG_DEBUG("=======================================================================\n");
    if (PMU->isChannelAvailable(XPOWERS_DCDC1)) {
        LOG_DEBUG("DC1  : %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_DCDC1) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_DCDC1));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC2)) {
        LOG_DEBUG("DC2  : %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_DCDC2) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_DCDC2));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC3)) {
        LOG_DEBUG("DC3  : %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_DCDC3) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_DCDC3));
    }
    if (PMU->isChannelAvailable(XPOWERS_DCDC4)) {
        LOG_DEBUG("DC4  : %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_DCDC4) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_DCDC4));
    }
    if (PMU->isChannelAvailable(XPOWERS_LDO2)) {
        LOG_DEBUG("LDO2 : %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_LDO2) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_LDO2));
    }
    if (PMU->isChannelAvailable(XPOWERS_LDO3)) {
        LOG_DEBUG("LDO3 : %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_LDO3) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_LDO3));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO1)) {
        LOG_DEBUG("ALDO1: %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_ALDO1) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_ALDO1));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO2)) {
        LOG_DEBUG("ALDO2: %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_ALDO2) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_ALDO2));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO3)) {
        LOG_DEBUG("ALDO3: %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_ALDO3) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_ALDO3));
    }
    if (PMU->isChannelAvailable(XPOWERS_ALDO4)) {
        LOG_DEBUG("ALDO4: %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_ALDO4) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_ALDO4));
    }
    if (PMU->isChannelAvailable(XPOWERS_BLDO1)) {
        LOG_DEBUG("BLDO1: %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_BLDO1) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_BLDO1));
    }
    if (PMU->isChannelAvailable(XPOWERS_BLDO2)) {
        LOG_DEBUG("BLDO2: %s   Voltage:%u mV \n", PMU->isPowerChannelEnable(XPOWERS_BLDO2) ? "+" : "-",
                  PMU->getPowerChannelVoltage(XPOWERS_BLDO2));
    }
    LOG_DEBUG("=======================================================================\n");

// We can safely ignore this approach for most (or all) boards because MCU turned off
// earlier than battery discharged to 2.6V.
//
// Unfortanly for now we can't use this killswitch for RAK4630-based boards because they have a bug with
// battery voltage measurement. Probably it sometimes drops to low values.
#ifndef RAK4630
    // Set PMU shutdown voltage at 2.6V to maximize battery utilization
    PMU->setSysPowerDownVoltage(2600);
#endif

#ifdef PMU_IRQ
    uint64_t pmuIrqMask = 0;

    if (PMU->getChipModel() == XPOWERS_AXP192) {
        pmuIrqMask = XPOWERS_AXP192_VBUS_INSERT_IRQ | XPOWERS_AXP192_BAT_INSERT_IRQ | XPOWERS_AXP192_PKEY_SHORT_IRQ;
    } else if (PMU->getChipModel() == XPOWERS_AXP2101) {
        pmuIrqMask = XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_PKEY_SHORT_IRQ;
    }

    pinMode(PMU_IRQ, INPUT);
    attachInterrupt(
        PMU_IRQ, [] { pmu_irq = true; }, FALLING);

    // we do not look for AXPXXX_CHARGING_FINISHED_IRQ & AXPXXX_CHARGING_IRQ because it occurs repeatedly while there is
    // no battery also it could cause inadvertent waking from light sleep just because the battery filled
    // we don't look for AXPXXX_BATT_REMOVED_IRQ because it occurs repeatedly while no battery installed
    // we don't look at AXPXXX_VBUS_REMOVED_IRQ because we don't have anything hooked to vbus
    PMU->enableIRQ(pmuIrqMask);

    PMU->clearIrqStatus();
#endif /*PMU_IRQ*/

    readPowerStatus();

    pmu_found = true;

    return pmu_found;

#else
    return false;
#endif
}
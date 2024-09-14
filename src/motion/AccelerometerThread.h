#pragma once
#ifndef _ACCELEROMETER_H_
#define _ACCELEROMETER_H_

#include "configuration.h"

#if !defined(ARCH_PORTDUINO) && !defined(ARCH_STM32WL) && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR

#include "../concurrency/OSThread.h"
#include "BMA423Sensor.h"
#include "BMX160Sensor.h"
#include "ICM20948Sensor.h"
#include "LIS3DHSensor.h"
#include "LSM6DS3Sensor.h"
#include "MPU6050Sensor.h"
#include "MotionSensor.h"
#include "STK8XXXSensor.h"

class AccelerometerThread : public concurrency::OSThread
{
  private:
    MotionSensor *sensor = nullptr;
    ScanI2C::FoundDevice device = ScanI2C::DEVICE_NONE;
    bool isInitialised = false;

  public:
    explicit AccelerometerThread(ScanI2C::FoundDevice found) : OSThread("AccelerometerThread")
    {
        if (found.address.port == ScanI2C::I2CPort::NO_I2C) {
            LOG_DEBUG("AccelerometerThread disabling due to no sensors found\n");
            disable();
            return;
        }

#ifndef RAK_4631
        if (!config.display.wake_on_tap_or_motion && !config.device.double_tap_as_button_press) {
            LOG_DEBUG("AccelerometerThread disabling due to no interested configurations\n");
            disable();
            return;
        }
#endif
        device = ScanI2C::FoundDevice(found.type, ScanI2C::DeviceAddress(found.address.port, found.address.address));
        init();
    }

    void start()
    {
        init();
        setIntervalFromNow(0);
    };

  protected:
    int32_t runOnce() override
    {
        // Assume we should not keep the board awake
        canSleep = true;

        if (isInitialised)
            return sensor->runOnce();
        else
            return MOTION_SENSOR_CHECK_INTERVAL_MS;
    }

  private:
    void init()
    {
        if (isInitialised)
            return;

        switch (device.type) {
        case ScanI2C::DeviceType::BMA423:
            sensor = new BMA423Sensor(device.address);
            break;
        case ScanI2C::DeviceType::MPU6050:
            sensor = new MPU6050Sensor(device.address);
            break;
        case ScanI2C::DeviceType::BMX160:
            sensor = new BMX160Sensor(device.address);
            break;
        case ScanI2C::DeviceType::LIS3DH:
            sensor = new LIS3DHSensor(device.address);
            break;
        case ScanI2C::DeviceType::LSM6DS3:
            sensor = new LSM6DS3Sensor(device.address);
            break;
        case ScanI2C::DeviceType::STK8BAXX:
            sensor = new STK8XXXSensor(device.address);
            break;
        case ScanI2C::DeviceType::ICM20948:
            sensor = new ICM20948Sensor(device.address);
            break;
        default:
            disable();
            return;
        }

        isInitialised = sensor->init();
        if (!isInitialised) {
            clean();
        }
        LOG_DEBUG("AccelerometerThread::init %s\n", isInitialised ? "ok" : "failed");
    }

    // Copy constructor (included to avoid cppcheck warnings)
    AccelerometerThread(const AccelerometerThread &other) : OSThread::OSThread("AccelerometerThread") { this->copy(other); }

    // Destructor (included to avoid cppcheck warnings)
    virtual ~AccelerometerThread() { clean(); }

    // Copy assignment (included to avoid cppcheck warnings)
    AccelerometerThread &operator=(const AccelerometerThread &other)
    {
        this->copy(other);
        return *this;
    }

    // Take a very shallow copy (does not copy OSThread state nor the sensor object)
    // If for some reason this is ever used, make sure to call init() after any copy
    void copy(const AccelerometerThread &other)
    {
        if (this != &other) {
            clean();
            this->device = ScanI2C::FoundDevice(other.device.type,
                                                ScanI2C::DeviceAddress(other.device.address.port, other.device.address.address));
        }
    }

    // Cleanup resources
    void clean()
    {
        isInitialised = false;
        if (sensor != nullptr) {
            delete sensor;
            sensor = nullptr;
        }
    }
};

#endif

#endif
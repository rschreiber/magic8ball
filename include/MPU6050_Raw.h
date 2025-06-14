#ifndef MPU6050_RAW_H
#define MPU6050_RAW_H

#include <Arduino.h>
#include <Wire.h>

// MPU6050 Register Addresses
#define MPU6050_DEFAULT_ADDR 0x68
#define MPU6050_ALT_ADDR     0x69
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define WHO_AM_I     0x75
#define ACCEL_CONFIG 0x1C

class MPU6050_Raw {
public:
    // Constructor
    MPU6050_Raw(uint8_t address = MPU6050_ALT_ADDR);
    
    // Initialization
    bool begin(uint8_t sda_pin, uint8_t scl_pin);
    bool isInitialized() const { return initialized; }
    
    // I2C device scanning
    void scanI2CDevices();
      // Accelerometer functions
    void readAccelerometer(float &x, float &y, float &z);
    bool detectShake(float threshold);
    
    // Configuration
    void setAccelerometerRange(uint8_t range); // 0=±2g, 1=±4g, 2=±8g, 3=±16g
    
    // Debug functions
    void printAccelData();
    uint8_t getWhoAmI();
    uint8_t getPowerManagement();

private:
    uint8_t mpuAddress;
    bool initialized;
    float accelSensitivity;
    unsigned long lastPrintTime;
    
    // Raw I2C communication
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    int16_t readRegister16(uint8_t reg);
    
    // Internal functions
    bool testConnection();
    bool wakeUpDevice();
    void updateAccelSensitivity();
};

#endif // MPU6050_RAW_H

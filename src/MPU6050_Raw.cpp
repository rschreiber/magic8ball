#include "MPU6050_Raw.h"

MPU6050_Raw::MPU6050_Raw(uint8_t address) {
    mpuAddress = address;
    initialized = false;
    accelSensitivity = 4096.0; // Default for ±8g range
    lastPrintTime = 0;
}

bool MPU6050_Raw::begin(uint8_t sda_pin, uint8_t scl_pin) {
    Serial.println("Initializing MPU6050 with raw I2C...");
    
    // Initialize I2C
    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(100000); // 100kHz I2C speed
    
    Serial.print("I2C initialized on pins SDA=");
    Serial.print(sda_pin);
    Serial.print(", SCL=");
    Serial.println(scl_pin);
    
    // Scan for devices
    scanI2CDevices();
    
    // Test connection
    if (!testConnection()) {
        return false;
    }
    
    // Wake up device
    if (!wakeUpDevice()) {
        return false;
    }
    
    // Configure accelerometer range to ±8g
    setAccelerometerRange(2); // 2 = ±8g
    
    Serial.println("MPU6050 initialized successfully with raw I2C!");
    initialized = true;
    return true;
}

void MPU6050_Raw::scanI2CDevices() {
    Serial.println("Scanning I2C devices...");
    
    int deviceCount = 0;
    for (byte address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        byte error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("I2C device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            
            switch(address) {
                case MPU6050_DEFAULT_ADDR:
                    Serial.print(" (MPU6050 - default address)");
                    break;
                case MPU6050_ALT_ADDR:
                    Serial.print(" (MPU6050 - alternate address)");
                    break;
                default:
                    Serial.print(" (Unknown device)");
                    break;
            }
            Serial.println();
            deviceCount++;
        }
    }
    
    if (deviceCount == 0) {
        Serial.println("No I2C devices found!");
    } else {
        Serial.print("Found ");
        Serial.print(deviceCount);
        Serial.println(" I2C device(s)");
    }
}

bool MPU6050_Raw::testConnection() {
    // Check if MPU6050 responds
    Wire.beginTransmission(mpuAddress);
    byte error = Wire.endTransmission();
    
    if (error != 0) {
        Serial.print("MPU6050 not responding at address 0x");
        Serial.print(mpuAddress, HEX);
        Serial.print(" (error: ");
        Serial.print(error);
        Serial.println(")");
        return false;
    }
    
    Serial.println("MPU6050 responds to I2C ping");
    
    // Read WHO_AM_I register to verify device
    uint8_t whoAmI = getWhoAmI();
    Serial.print("WHO_AM_I register: 0x");
    Serial.println(whoAmI, HEX);
    
    // WHO_AM_I should be 0x68 for MPU6050
    if (whoAmI != 0x68) {
        Serial.print("Unexpected WHO_AM_I value: 0x");
        Serial.print(whoAmI, HEX);
        Serial.println(" (expected 0x68)");
        Serial.println("Continuing anyway - might still work...");
    }
    
    return true;
}

bool MPU6050_Raw::wakeUpDevice() {
    // Wake up the MPU6050 (it starts in sleep mode)
    Serial.println("Waking up MPU6050...");
    writeRegister(PWR_MGMT_1, 0x00);
    delay(100);
    
    // Verify wake-up by reading power management register
    uint8_t pwrMgmt = getPowerManagement();
    Serial.print("Power management register: 0x");
    Serial.println(pwrMgmt, HEX);
    
    if (pwrMgmt & 0x40) {
        Serial.println("WARNING: Device still in sleep mode");
        // Try again
        writeRegister(PWR_MGMT_1, 0x00);
        delay(100);
        pwrMgmt = getPowerManagement();
        Serial.print("Power management after retry: 0x");
        Serial.println(pwrMgmt, HEX);
        
        if (pwrMgmt & 0x40) {
            Serial.println("ERROR: Failed to wake up device");
            return false;
        }
    }
    
    return true;
}

void MPU6050_Raw::setAccelerometerRange(uint8_t range) {
    // Set accelerometer range (register 0x1C)
    // 0 = ±2g, 1 = ±4g, 2 = ±8g, 3 = ±16g
    writeRegister(ACCEL_CONFIG, range << 3);
    updateAccelSensitivity();
    
    Serial.print("Accelerometer range set to ±");
    switch(range) {
        case 0: Serial.println("2g"); break;
        case 1: Serial.println("4g"); break;
        case 2: Serial.println("8g"); break;
        case 3: Serial.println("16g"); break;
        default: Serial.println("unknown"); break;
    }
}

void MPU6050_Raw::updateAccelSensitivity() {
    uint8_t config = readRegister(ACCEL_CONFIG);
    uint8_t range = (config >> 3) & 0x03;
    
    switch(range) {
        case 0: accelSensitivity = 16384.0; break; // ±2g
        case 1: accelSensitivity = 8192.0;  break; // ±4g
        case 2: accelSensitivity = 4096.0;  break; // ±8g
        case 3: accelSensitivity = 2048.0;  break; // ±16g
        default: accelSensitivity = 4096.0; break; // Default to ±8g
    }
}

void MPU6050_Raw::readAccelerometer(float &x, float &y, float &z) {
    if (!initialized) {
        x = y = z = 0.0;
        return;
    }
    
    // Read raw accelerometer data (6 bytes starting from ACCEL_XOUT_H)
    int16_t rawX = readRegister16(ACCEL_XOUT_H);
    int16_t rawY = readRegister16(ACCEL_YOUT_H);
    int16_t rawZ = readRegister16(ACCEL_ZOUT_H);
    
    // Convert to g (gravitational acceleration)
    x = rawX / accelSensitivity;
    y = rawY / accelSensitivity;
    z = rawZ / accelSensitivity;
}

bool MPU6050_Raw::detectShake(float threshold) {
    if (!initialized) {
        return false;
    }
    
    float x, y, z;
    readAccelerometer(x, y, z);
    
    // Calculate total acceleration magnitude
    float totalAccel = sqrt(x*x + y*y + z*z);
    
    // Detect shake (acceleration significantly different from 1g)
    return fabs(totalAccel - 1.0) > threshold;
}

void MPU6050_Raw::printAccelData() {
    if (!initialized) {
        Serial.println("MPU6050 not initialized");
        return;
    }
    
    // Print acceleration data every 500ms
    if (millis() - lastPrintTime > 500) {
        float x, y, z;
        readAccelerometer(x, y, z);
        
        float totalAccel = sqrt(x*x + y*y + z*z);
        
        Serial.print("Accel: X=");
        Serial.print(x, 2);
        Serial.print("g, Y=");
        Serial.print(y, 2);
        Serial.print("g, Z=");
        Serial.print(z, 2);
        Serial.print("g, Total=");
        Serial.print(totalAccel, 2);
        Serial.print("g, Diff from 1g=");
        Serial.println(fabs(totalAccel - 1.0), 2);
        
        lastPrintTime = millis();
    }
}

uint8_t MPU6050_Raw::getWhoAmI() {
    return readRegister(WHO_AM_I);
}

uint8_t MPU6050_Raw::getPowerManagement() {
    return readRegister(PWR_MGMT_1);
}

// Private methods
void MPU6050_Raw::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(mpuAddress);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t MPU6050_Raw::readRegister(uint8_t reg) {
    Wire.beginTransmission(mpuAddress);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(mpuAddress, (uint8_t)1);
    return Wire.read();
}

int16_t MPU6050_Raw::readRegister16(uint8_t reg) {
    Wire.beginTransmission(mpuAddress);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(mpuAddress, (uint8_t)2);
    
    int16_t value = Wire.read() << 8;
    value |= Wire.read();
    return value;
}

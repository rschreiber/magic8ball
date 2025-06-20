#ifndef MAGIC8BALL_H
#define MAGIC8BALL_H

#include <Arduino.h>
#include <math.h>
#include <U8g2lib.h>
#include <Wire.h>

// OLED display settings for SH1106
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // I2C address for SH1106

// I2C pin definitions for MPU6050 and OLED (shared I2C bus)
#define MPU_SDA D2      // GPIO4 - Standard I2C pins
#define MPU_SCL D1      // GPIO5

// Button pin for manual trigger (fallback)
#define BUTTON_PIN D3   // GPIO0 - Built-in button on NodeMCU

// Function declarations
void handleShakeDetection();
void handleButtonPress();
void showRandomResponse();
void initializeDisplay();
void scanI2CForDisplay();
void displayText(const char* text, bool center = true);
void displayMagic8BallResponse(const char* response);
void displayWelcomeMessage();
void displayAnimatedWelcome();
void draw8Ball(int centerX, int centerY, int radius, int shakeOffset = 0);
void initializeDFPlayer();
void playRandomSound();

// External variables (defined in main.cpp)
extern const char* responses[];
extern const int numResponses;
extern float shakeThreshold;
extern bool isShaking;
extern bool responseShown;
extern unsigned long lastShakeTime;
extern unsigned long responseDisplayTime;
extern const unsigned long responseDisplayDuration;
extern unsigned long welcomeAnimationTime;
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C display;

#endif // MAGIC8BALL_H

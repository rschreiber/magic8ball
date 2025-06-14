#include <Arduino.h>
#include <Wire.h>
#include "magic8ball.h"
#include "MPU6050_Raw.h"

// Initialize SH1106 display object
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Magic 8-ball responses
const char* responses[] = {
  "It is certain",
  "Reply hazy, try again",
  "Don't count on it",
  "It is decidedly so",
  "Ask again later",
  "My reply is no",
  "Without a doubt",
  "Better not tell you now",
  "My sources say no",
  "Yes definitely",
  "Cannot predict now",
  "Outlook not so good",
  "You may rely on it",
  "Concentrate and ask again",
  "Very doubtful",
  "As I see it, yes",
  "Most likely",
  "Outlook good",
  "Yes",
  "Signs point to yes"
};

const int numResponses = sizeof(responses) / sizeof(responses[0]);

// Global variables
MPU6050_Raw mpu(MPU6050_ALT_ADDR); // Use alternate address 0x69
float shakeThreshold = 1.5; // Threshold in g
bool isShaking = false;
bool responseShown = false;
unsigned long lastShakeTime = 0;
unsigned long responseDisplayTime = 0;
const unsigned long responseDisplayDuration = 3000;
unsigned long welcomeAnimationTime = 0;

void scanI2CForDisplay() {
  Serial.println("Scanning I2C for OLED display...");
  
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      Serial.print(address, HEX);
      if (address == 0x3C) Serial.print(" (SH1106/SSD1306 - common address)");
      if (address == 0x3D) Serial.print(" (SH1106/SSD1306 - alternate address)");
      Serial.println();
    }
  }
}

void initializeDisplay() {
  // Scan for I2C devices first
  scanI2CForDisplay();
  
  Serial.println("Initializing SH1106 display...");
  
  // Initialize U8g2 display
  display.begin();
  display.clearBuffer();
  display.sendBuffer();
  
  Serial.println("SH1106 Display initialized successfully!");
  
  // Test pattern to verify display is working
  Serial.println("Testing display with pattern...");
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(0, 15, "SH1106 Test");
  display.drawStr(0, 30, "If you can read");
  display.drawStr(0, 45, "this, display");
  display.drawStr(0, 60, "is working!");
  display.sendBuffer();
  delay(3000);
}

void draw8Ball(int centerX, int centerY, int radius, int shakeOffset) {
  // Draw the outer black circle (8-ball body)
  display.drawCircle(centerX + shakeOffset, centerY + shakeOffset, radius);
  display.drawCircle(centerX + shakeOffset, centerY + shakeOffset, radius - 1);
  
  // Draw the white circle for the "8"
  int whiteCircleRadius = radius / 3;
  display.drawCircle(centerX + shakeOffset, centerY - radius/3 + shakeOffset, whiteCircleRadius);
  
  // Draw the "8" in the white circle
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(centerX - 3 + shakeOffset, centerY - radius/3 + 3 + shakeOffset, "8");
  
  // Add some highlight lines to make it look more 3D
  display.drawLine(centerX - radius/2 + shakeOffset, centerY - radius/2 + shakeOffset, 
                   centerX - radius/3 + shakeOffset, centerY - radius/3 + shakeOffset);
}

void displayText(const char* text, bool center) {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  if (center) {
    // Calculate text position for centering
    int textWidth = display.getStrWidth(text);
    int x = (SCREEN_WIDTH - textWidth) / 2;
    int y = SCREEN_HEIGHT / 2;
    display.drawStr(x, y, text);
  } else {
    display.drawStr(0, 15, text);
  }
  
  display.sendBuffer();
}

void displayMagic8BallResponse(const char* response) {
  // Animation sequence: shake effect, then reveal response
  
  // Shake animation (6 frames)
  for (int shake = 0; shake < 6; shake++) {
    display.clearBuffer();
    
    // Draw shaking 8-ball
    int shakeOffset = (shake % 2 == 0) ? -2 : 2;
    draw8Ball(SCREEN_WIDTH/2, 30, 18, shakeOffset);
    
    // Draw "Thinking..." text with dots animation
    display.setFont(u8g2_font_6x10_tf);
    String thinkingText = "Thinking";
    for (int i = 0; i < (shake % 4); i++) {
      thinkingText += ".";
    }
    int textWidth = display.getStrWidth(thinkingText.c_str());
    display.drawStr((SCREEN_WIDTH - textWidth) / 2, 58, thinkingText.c_str());
    
    display.sendBuffer();
    delay(200);
  }
  
  // Fade-in effect for the response
  for (int fade = 0; fade < 3; fade++) {
    display.clearBuffer();
      // Draw static 8-ball
    draw8Ball(SCREEN_WIDTH/2, 30, 18, 0);
    
    // Display the response with fade effect (by showing more of it each frame)
    display.setFont(u8g2_font_6x10_tf);
    String responseStr = String(response);
    int maxCharsPerLine = 21;
      if ((int)responseStr.length() > maxCharsPerLine) {
      String line = responseStr.substring(0, maxCharsPerLine);
      int lastSpace = line.lastIndexOf(' ');
      if (lastSpace > 0 && lastSpace < maxCharsPerLine - 3) {
        line = line.substring(0, lastSpace) + "...";
      }
      
      // Show characters progressively during fade
      int totalChars = (int)line.length();
      int charsToShow = (fade + 1) * totalChars / 3;
      if (charsToShow > totalChars) charsToShow = totalChars;
      String fadeText = line.substring(0, charsToShow);
      
      int textWidth = display.getStrWidth(fadeText.c_str());
      display.drawStr((SCREEN_WIDTH - textWidth) / 2, 58, fadeText.c_str());
    } else {
      // Show characters progressively during fade
      int totalChars = (int)responseStr.length();
      int charsToShow = (fade + 1) * totalChars / 3;
      if (charsToShow > totalChars) charsToShow = totalChars;
      String fadeText = responseStr.substring(0, charsToShow);
      
      int textWidth = display.getStrWidth(fadeText.c_str());
      display.drawStr((SCREEN_WIDTH - textWidth) / 2, 58, fadeText.c_str());
    }
    
    display.sendBuffer();
    delay(300);
  }
    // Final display - show complete response
  display.clearBuffer();
  draw8Ball(SCREEN_WIDTH/2, 30, 18, 0);
  
  display.setFont(u8g2_font_6x10_tf);
  String responseStr = String(response);
  int maxCharsPerLine = 21;
  
  if ((int)responseStr.length() > maxCharsPerLine) {
    String line = responseStr.substring(0, maxCharsPerLine);
    int lastSpace = line.lastIndexOf(' ');
    if (lastSpace > 0 && lastSpace < maxCharsPerLine - 3) {
      line = line.substring(0, lastSpace) + "...";
    }
    int textWidth = display.getStrWidth(line.c_str());
    display.drawStr((SCREEN_WIDTH - textWidth) / 2, 58, line.c_str());
  } else {
    int textWidth = display.getStrWidth(response);
    display.drawStr((SCREEN_WIDTH - textWidth) / 2, 58, response);
  }
  
  display.sendBuffer();
}

void displayWelcomeMessage() {
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  
  // Create a pulsing effect for the 8-ball
  unsigned long currentTime = millis();
  float pulsePhase = (currentTime % 2000) / 2000.0 * 2 * PI; // 2 second cycle
  int radiusVariation = (int)(2 * sin(pulsePhase)); // +/- 2 pixels
  int baseRadius = 16;
    // Draw the 8-ball in the center-top area with pulsing effect
  draw8Ball(SCREEN_WIDTH/2, 22, baseRadius + radiusVariation, 0);
  
  // Title below the 8-ball
  int titleWidth = display.getStrWidth("MAGIC 8-BALL");
  display.drawStr((SCREEN_WIDTH - titleWidth) / 2, 45, "MAGIC 8-BALL");
  
  // Instructions at the bottom
  int instructWidth = display.getStrWidth("Shake to ask!");
  display.drawStr((SCREEN_WIDTH - instructWidth) / 2, 58, "Shake to ask!");
  
  display.sendBuffer();
}

void displayAnimatedWelcome() {
  // Only show animated welcome when not showing a response
  if (!responseShown) {
    displayWelcomeMessage();
  }
}

void showRandomResponse() {
  randomSeed(millis());
  int responseIndex = random(numResponses);
  
  // Display on Serial
  Serial.println();
  Serial.println("=== MAGIC 8-BALL RESPONSE ===");
  Serial.print(">> ");
  Serial.print(responses[responseIndex]);
  Serial.println(" <<");
  Serial.println("=============================");
  Serial.println();
  
  // Display on OLED
  displayMagic8BallResponse(responses[responseIndex]);
}

void handleShakeDetection() {
  if (mpu.isInitialized()) {
    // Print accelerometer data for debugging
    mpu.printAccelData();
    
    // Use accelerometer for shake detection
    if (mpu.detectShake(shakeThreshold)) {
      if (!isShaking && (millis() - lastShakeTime > 1000)) {
        isShaking = true;
        lastShakeTime = millis();
        Serial.println("SHAKE DETECTED!");
        showRandomResponse();
        responseShown = true;
        responseDisplayTime = millis();
      }
    } else {
      isShaking = false;
    }
  } else {
    // Use button as fallback
    handleButtonPress();
  }
    // Clear response flag after display duration
  if (responseShown && (millis() - responseDisplayTime > responseDisplayDuration)) {
    responseShown = false;
    welcomeAnimationTime = millis(); // Start welcome animation
    if (mpu.isInitialized()) {
      Serial.println("Ready for next shake...");
    } else {
      Serial.println("Ready for next button press...");
    }
  }
  
  // Show animated welcome screen when not showing response
  if (!responseShown) {
    displayAnimatedWelcome();
  }
}

void handleButtonPress() {
  static bool lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;
  
  bool buttonState = digitalRead(BUTTON_PIN);
  
  if (buttonState != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonState == LOW && lastButtonState == HIGH) {
      // Button pressed (active low)
      if (!responseShown && (millis() - lastShakeTime > 1000)) {
        Serial.println("BUTTON PRESSED!");
        lastShakeTime = millis();
        showRandomResponse();
        responseShown = true;
        responseDisplayTime = millis();
      }
    }
  }
  
  lastButtonState = buttonState;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("=== MAGIC 8-BALL ===");
  Serial.println("With SH1106 OLED Display");
  Serial.println();
  
  // Initialize I2C
  Wire.begin(MPU_SDA, MPU_SCL);
  
  // Initialize SH1106 OLED display
  initializeDisplay();
  displayWelcomeMessage();
  
  // Initialize button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize MPU6050
  bool mpuInitialized = mpu.begin(MPU_SDA, MPU_SCL); // D2=GPIO4, D1=GPIO5
  
  if (mpuInitialized) {
    Serial.println("Shake detection enabled!");
    Serial.println("   Shake the device to get a response!");
    Serial.println("   Monitoring accelerometer data...");
  } else {
    Serial.println("MPU6050 initialization failed");
    Serial.println("   Button mode enabled - press built-in button (D3)");
  }
  
  Serial.println();
  Serial.println("Ask the Magic 8-Ball a question...");
  Serial.println("====================================");
}

void loop() {
  handleShakeDetection();
  delay(100); // Small delay to prevent excessive polling
}

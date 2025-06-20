#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "magic8ball.h"
#include "MPU6050_Raw.h"
#include "wifi_config.h"

// Initialize SH1106 display object
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// DFPlayer Mini setup - Using D0 and D3 pins (GPIO16, GPIO0)
SoftwareSerial mySoftwareSerial(D0, D3); // RX=D0(GPIO16), TX=D3(GPIO0)
DFRobotDFPlayerMini myDFPlayer;

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

// WiFi Access Point settings
const char* ap_ssid = WIFI_AP_SSID;
const char* ap_password = WIFI_AP_PASSWORD;
IPAddress local_ip(AP_IP_ADDRESS);
IPAddress gateway(AP_GATEWAY);
IPAddress subnet(AP_SUBNET);

// Web server
ESP8266WebServer server(WEB_SERVER_PORT);

// WiFi status variables
bool wifiEnabled = ENABLE_WIFI;
String lastWebResponse = "";
unsigned long lastWebResponseTime = 0;

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
  Serial.println("Displaying startup message...");
  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(0, 15, "Starting the");
  display.drawStr(0, 30, "Magic 8 Ball...");
  display.drawStr(0, 45, "Please wait");
  display.drawStr(0, 60, "while loading");
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
  draw8Ball(SCREEN_WIDTH/2, 18, baseRadius + radiusVariation, 0);
  
  // Title below the 8-ball
  int titleWidth = display.getStrWidth("MAGIC 8-BALL");
  display.drawStr((SCREEN_WIDTH - titleWidth) / 2, 38, "MAGIC 8-BALL");
  
  // Instructions - alternate between shake and wifi info
  static unsigned long lastToggle = 0;
  static bool showWiFiInfo = false;
  
  if (millis() - lastToggle > 3000) { // Change every 3 seconds
    showWiFiInfo = !showWiFiInfo;
    lastToggle = millis();
  }
  
  if (wifiEnabled && showWiFiInfo) {
    // Show WiFi info
    int wifiWidth = display.getStrWidth("WiFi: Magic8Ball-WiFi");
    display.drawStr((SCREEN_WIDTH - wifiWidth) / 2, 50, "WiFi: Magic8Ball-WiFi");
    int ipWidth = display.getStrWidth("192.168.4.1");
    display.drawStr((SCREEN_WIDTH - ipWidth) / 2, 62, "192.168.4.1");
  } else {
    // Show shake instruction
    int instructWidth = display.getStrWidth("Shake to ask!");
    display.drawStr((SCREEN_WIDTH - instructWidth) / 2, 56, "Shake to ask!");
  }
  
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

void initializeDFPlayer() {
  Serial.println("Initializing DFPlayer Mini...");
  
  // Initialize SoftwareSerial for DFPlayer communication
  mySoftwareSerial.begin(9600);
  delay(3000); // Give more time for module to stabilize
  
  Serial.println("Attempting DFPlayer connection...");
  
  // Try different initialization methods
  bool success = false;
  
  // First try without acknowledgment (more reliable)
  Serial.print("Attempt 1 (no ACK): ");
  if (myDFPlayer.begin(mySoftwareSerial, false, false)) {
    Serial.println("SUCCESS!");
    success = true;
  } else {
    Serial.println("Failed");
  }
  
  if (!success) {
    Serial.print("Attempt 2 (with ACK): ");
    if (myDFPlayer.begin(mySoftwareSerial, true, false)) {
      Serial.println("SUCCESS!");
      success = true;
    } else {
      Serial.println("Failed");
    }
  }
  
  if (!success) {
    Serial.print("Attempt 3 (with reset): ");
    if (myDFPlayer.begin(mySoftwareSerial, true, true)) {
      Serial.println("SUCCESS!");
      success = true;
    } else {
      Serial.println("Failed");
    }
  }
  
  // Even if begin() fails, the module might still work
  // Let's try sending commands anyway since you see the light flashing
  if (!success) {
    Serial.println("DFPlayer begin() failed, but trying commands anyway...");
    Serial.println("(Light flashing suggests module is responding)");
  } else {
    Serial.println("DFPlayer Mini online!");
  }  // Give time before sending commands
  delay(2000);
  
  // Set volume value (0~30) - start lower for testing
  Serial.println("Setting volume...");
  myDFPlayer.volume(20);
  delay(1000);
    // Configure playback mode to prevent auto-looping
  Serial.println("Configuring single play mode...");
  
  // Set single cycle mode (play one track and stop)
  myDFPlayer.disableLoopAll(); // Disable loop all tracks
  myDFPlayer.disableLoop(); // Disable loop current track
  myDFPlayer.disableDAC(); // Disable DAC output (if applicable)
  delay(500);
  
  // Try to set single play mode using EQ setting trick
  // Some DFPlayer modules use EQ settings to control play mode
  myDFPlayer.EQ(DFPLAYER_EQ_NORMAL); // Reset to normal mode
  delay(500);
  
  // Ensure we're using SD card as source
  myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD);
  delay(500);
  
  Serial.println("DFPlayer initialization complete (may work despite errors)");
}

void playRandomSound() {
  // Randomly choose between the two sound files
  // Files: 0001.mp3, 0002.mp3
  randomSeed(millis());
  int soundChoice = random(1, 3); // Random between 1 and 2
  
  Serial.print("Playing sound file: ");
  if (soundChoice == 1) {
    Serial.println("0001.mp3");
  } else {
    Serial.println("0002.mp3");
  }
  
  // Stop any currently playing audio first
  myDFPlayer.stop();
  delay(100);
  
  // Play the selected file
  myDFPlayer.play(soundChoice);
  
  // Note: File should stop automatically when finished
  // If it continues to next file, this indicates the DFPlayer 
  // is in repeat/loop mode which we try to prevent in initialization
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
        
        // Play sound effect on shake
        playRandomSound();
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

void initializeWiFi() {
  Serial.println("Initializing WiFi Access Point...");
  
  // Configure Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ap_ssid, ap_password);
  
  delay(100);
  
  Serial.println("WiFi Access Point started!");
  Serial.print("AP SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP Password: ");
  Serial.println(ap_password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Connect to the WiFi network and visit http://192.168.4.1");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Magic 8-Ball WiFi</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; margin: 0; padding: 20px; }";
  html += ".container { max-width: 500px; margin: 0 auto; background: rgba(255,255,255,0.1); padding: 30px; border-radius: 20px; box-shadow: 0 8px 32px rgba(0,0,0,0.3); }";
  html += "h1 { font-size: 2.5em; margin-bottom: 20px; text-shadow: 2px 2px 4px rgba(0,0,0,0.5); }";
  html += ".ball { width: 120px; height: 120px; background: #000; border-radius: 50%; margin: 20px auto; position: relative; box-shadow: 0 0 20px rgba(0,0,0,0.5); }";
  html += ".ball::after { content: '8'; position: absolute; top: 25px; left: 50%; transform: translateX(-50%); color: white; font-size: 24px; font-weight: bold; }";
  html += "button { background: #ff6b6b; color: white; border: none; padding: 15px 30px; font-size: 18px; border-radius: 50px; cursor: pointer; margin: 10px; transition: all 0.3s; }";
  html += "button:hover { background: #ff5252; transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.2); }";
  html += ".response { background: rgba(255,255,255,0.2); padding: 20px; border-radius: 15px; margin: 20px 0; min-height: 60px; display: flex; align-items: center; justify-content: center; }";
  html += ".response h2 { margin: 0; font-size: 1.5em; }";
  html += ".info { font-size: 0.9em; opacity: 0.8; margin-top: 20px; }";
  html += "@keyframes shake { 0%, 100% { transform: translateX(0); } 25% { transform: translateX(-5px); } 75% { transform: translateX(5px); } }";
  html += ".shaking { animation: shake 0.5s ease-in-out; }";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>STR Magic 8-Ball</h1>";
  html += "<div class='ball' id='ball'></div>";
  html += "<button onclick='askQuestion()'>Ask the Magic 8-Ball!</button>";
  html += "<div class='response' id='response'>";
  if (lastWebResponse != "") {
    html += "<h2>" + lastWebResponse + "</h2>";
  } else {
    html += "<p>Click the button to get a response!</p>";
  }
  html += "</div>";
  html += "<div class='info'>";
  html += "<p>You can also shake the physical device!</p>";
  html += "<p>Connected clients: " + String(WiFi.softAPgetStationNum()) + "</p>";
  html += "</div></div>";
  
  html += "<script>";
  html += "function askQuestion() {";
  html += "  document.getElementById('ball').classList.add('shaking');";
  html += "  document.getElementById('response').innerHTML = '<p>Thinking...</p>';";
  html += "  fetch('/ask').then(response => response.text()).then(data => {";
  html += "    setTimeout(() => {";
  html += "      document.getElementById('response').innerHTML = '<h2>' + data + '</h2>';";
  html += "      document.getElementById('ball').classList.remove('shaking');";
  html += "    }, 1000);";
  html += "  });";
  html += "}";
  html += "</script></body></html>";
  
  server.send(200, "text/html", html);
}

void handleAsk() {
  // Generate random response
  randomSeed(millis());
  int responseIndex = random(numResponses);
  String response = String(responses[responseIndex]);
  
  // Store for display on physical device
  lastWebResponse = response;
  lastWebResponseTime = millis();
  
  // Show on physical display
  displayMagic8BallResponse(responses[responseIndex]);
  responseShown = true;
  responseDisplayTime = millis();
  
  // Play sound if available
  playRandomSound();
  
  Serial.println("Web request - Response: " + response);
  
  server.send(200, "text/plain", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "Page not found");
}

void initializeWebServer() {
  server.on("/", handleRoot);
  server.on("/ask", handleAsk);
  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("Web server started on port 80");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();  Serial.println("=== MAGIC 8-BALL ===");
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
    // Initialize DFPlayer Mini
  initializeDFPlayer();
  
  // Initialize WiFi Access Point and Web Server
  if (wifiEnabled) {
    initializeWiFi();
    initializeWebServer();
  }
  
  Serial.println();
  Serial.println("Ask the Magic 8-Ball a question...");
  if (wifiEnabled) {
    Serial.println("You can also connect to WiFi and visit http://192.168.4.1");
  }
  Serial.println("====================================");
}

void loop() {
  // Handle web server requests
  if (wifiEnabled) {
    server.handleClient();
  }
  
  handleShakeDetection();
  delay(100); // Small delay to prevent excessive polling
}

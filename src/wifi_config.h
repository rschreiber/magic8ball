#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

// WiFi Access Point Configuration
// Change these settings to customize your Magic 8-Ball WiFi network

#define WIFI_AP_SSID "Magic8Ball-WiFi"
#define WIFI_AP_PASSWORD "magic123"      // Must be at least 8 characters
#define WIFI_AP_CHANNEL 1                // WiFi channel (1-13)
#define WIFI_MAX_CONNECTIONS 4           // Maximum simultaneous connections

// Web Server Configuration
#define WEB_SERVER_PORT 80

// Network Configuration
#define AP_IP_ADDRESS {192, 168, 4, 1}   // Access Point IP
#define AP_GATEWAY {192, 168, 4, 1}      // Gateway IP
#define AP_SUBNET {255, 255, 255, 0}     // Subnet mask

// Optional: Enable/Disable WiFi at compile time
#define ENABLE_WIFI true

#endif // WIFI_CONFIG_H

/* Copy this file to settings.h and fill in your personal settings */

/* Display */
#include <GxGDEP015OC1/GxGDEP015OC1.cpp>    // 1.54" b/w
// GxIO_SPI(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst = -1, int8_t bl = -1);
GxIO_Class io(SPI, SS, D3); // arbitrary selection of D3(=0), D4(=2), selected for default of GxEPD_Class
// GxGDEP015OC1(GxIO& io, uint8_t rst = 2, uint8_t busy = 4);
GxEPD_Class display(io, D1, D2); // default selection of D4(=2), D2(=4)

/* WiFi */
char ssid[] = "ssid";  //  your network SSID (name)
char pass[] = "pass";       // your network password

/* MQTT */
#define MQTT_SERVER      "192.168.1.2"
#define MQTT_SERVERPORT  1883                   // use 8883 for SSL
#define MQTT_USERNAME    "user"
#define MQTT_KEY         "password"
#define MQTT_TEMP        "sensors/temp"

/* Time */
char tz = 0; //your timezone

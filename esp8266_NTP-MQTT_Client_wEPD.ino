/*

  Udp NTP Client

  Get the time from a Network Time Protocol (NTP) time server
  Demonstrates use of UDP sendPacket and ReceivePacket
  For more on NTP time servers and the messages needed to communicate with them,
  see http://en.wikipedia.org/wiki/Network_Time_Protocol

  created 4 Sep 2010
  by Michael Margolis
  modified 9 Apr 2012
  by Tom Igoe
  updated for the ESP8266 12 Apr 2015
  by Ivan Grokhotkov

  Made it output to e-ink display and include mqtt temperature
  by Rikard Lindström Dec 2017,2018

  This code is in the public domain.

*/

#include <GxEPD.h> // Install from ZIP: https://github.com/ZinggJM/GxEPD
// Specific display driver is loaded from settings.h
#include <GxIO/GxIO_SPI/GxIO_SPI.cpp>
#include <GxIO/GxIO.cpp>
#include <Fonts/FreeMono9pt7b.h> //Adafruit GFX Library

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "DSEG7ClassicMini-Bold18.h" //https://www.keshikan.net/fonts-e.html / tools in Adafruit GFX Library

#include "settings.h" //Your settings (display model and connection, WiFi, Timezone, mqtt)



unsigned long ms = 0; //last millis count
unsigned long s = 0; //overflow ms into seconds
unsigned long offset = 0; // offset between count and unix timestamp
unsigned long epoch = 0; //current posix time
char hour = 0; //hour (GMT)
char minute = 0;
char second = 0;
int checkTime = 0; //Countdown to check NTP
unsigned char  tStr[20]; //Temperature from mqtt
float temp = 0.0; //temp to show

unsigned int localPort = 2390;      // local port to listen for UDP packets

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_SERVERPORT, MQTT_USERNAME, MQTT_KEY);

Adafruit_MQTT_Subscribe temp_out = Adafruit_MQTT_Subscribe(&mqtt, MQTT_TEMP);


/* Don't hardwire the IP address or we won't get the benefits of the pool.
*  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

void setup()
{
  Serial.begin(115200);
  display.init();


  Serial.println();
  Serial.println("NTP Clock");

  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMono9pt7b);
  display.setCursor(0, 0);
  display.println();
  display.println("NTP Clock");
  display.update();

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  display.print("WiFi: ");
  display.print(ssid);
  display.updateWindow(0, 0, display.width(), display.height());
  WiFi.mode(WIFI_OFF);
  delay(500);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.updateWindow(0, 0, display.width(), display.height());
  }
  Serial.println("");
  display.println("OK");
  display.updateWindow(0, 0, display.width(), display.height());

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.updateWindow(0, 0, display.width(), display.height());

  Serial.println("Starting UDP");
  display.println("Starting UDP");
  display.updateWindow(0, 0, display.width(), display.height());
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  display.print("Port: ");
  display.println(udp.localPort());
  display.updateWindow(0, 0, display.width(), display.height());

  mqtt.subscribe(&temp_out);

  display.print("Getting time");

}

void loop()
{
  //Count seconds since startup.
  if (millis() < ms)
  {
    s += (unsigned long)0xffff/1000; //add how many secons fit before overflowing
  }
  ms = millis();

  if (!checkTime)
  {
    refreshNTP();
  }
  else
  {
    unsigned long oldEpoch = epoch;
    updateTime();
    if( epoch > oldEpoch )
    {
      updateDisplay();
      checkTime--;
    }
  }
  MQTT_connect();
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(100))) {
    if (subscription == &temp_out) {
      for(int i = 0; i < 20; i++)
        temp = ((String)(char *)temp_out.lastread).toFloat();
    }
  }
}

void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

void updateDisplay()
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&DSEG7ClassicMini_Bold18pt7b);
  display.setCursor(0, 0);

  display.println();
  if ( hour < 10 ) display.print('0');
  display.print((int)hour);
  display.print(':');
  if ( minute < 10 ) display.print('0'); // In the first 10 minutes of each hour, we'll want a leading '0'
  display.print((int)minute); // print the minute
  display.print(':');
  if ( second < 10 ) display.print('0');
  display.println((int)second); // print the second

  //Display weather
  //display.setFont(&DSEGWeather50pt7b);
  //display.setCursor(0, 40);
  //display.println();
  //display.print('0');
  display.print(temp,1);
  display.println("C");

  if (!minute & !second) display.update(); //do full refresh every hour
  else display.updateWindow(0, 0, display.width(), display.height());
}

void updateTime(void)
{
  epoch = offset + s + ms/1000;
  hour = ( (tz * 3600 + epoch)  % 86400L) / 3600;
  minute = (epoch  % 3600) / 60;
  second = epoch % 60;
}

void refreshNTP(void)
{

  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);

  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
    display.print(".");
    display.updateWindow(0, 0, display.width(), display.height());
  }
  else {
    display.print("OK");
    display.updateWindow(0, 0, display.width(), display.height());
    Serial.print("packet received, length = ");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    offset = epoch - s - ms/1000;

    checkTime = 3600;
  }

}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

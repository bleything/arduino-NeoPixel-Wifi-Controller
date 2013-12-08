/*******************************************************************************
 A wifi controller for your NeoPixel strips.

 Designed for the Adafruit CC3000 shield, but you can use it with a CC3000
 module separately by either using the pins defined below or redefining the
 pins.

 Copyright (c) 2013 Ben Bleything <ben@bleything.net>
 Released under the terms of the MIT License. See LICENSE for details.
 ******************************************************************************/

#include <Adafruit_CC3000.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <CC3000_MDNS.h>
#include "../../libraries/Adafruit_CC3000/utility/debug.h"

// NeoPixel configuration
#define PIN_NEOPIXEL   6
#define NEOPIXEL_COUNT 60

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

// pin assignments for the Adafruit CC3000 Shield
#define PIN_IRQ  3
#define PIN_VBAT 5
#define PIN_CS   10
#define PIN_MOSI 11 // these are the hardware SPI pins
#define PIN_MISO 12 // ...
#define PIN_SCK  13 // ...

// Your wireless network details. Change these!
#define WIFI_SSID     "CHANGEME"
#define WIFI_PASSWORD "ALSOME"
#define WIFI_SECURITY WLAN_SEC_WPA2 // or UNSEC, WEP, or WPA in place of WPA2

// Server details. Probably leave these alone.
#define LISTEN_PORT 49572
#define MDNS_NAME   "neopixels"

Adafruit_CC3000 wifi = Adafruit_CC3000(PIN_CS, PIN_IRQ, PIN_VBAT, SPI_CLOCK_DIV2);
Adafruit_CC3000_Server server(LISTEN_PORT);
MDNSResponder mdns;

////////////////////////////////////////////////////////////////////////////////

void setup(void) {
  // shut up, Adafruit_CC3000
  CC3KPrinter = 0;

  Serial.begin(115200);
  Serial.print("Initializing CC3000...");

  if( wifi.begin() ) {
    Serial.println("success!");
  } else {
    Serial.println("failed. Check your wiring.");
  }

  Serial.print("Connecting to AP.");
  connect();
  Serial.println("success!");

  Serial.print("Getting an IP address from DHCP.");
  while( !wifi.checkDHCP() ) {
    Serial.print(".");
    delay(100);
  }

  Serial.print(" ");
  printIP();
  Serial.println();

  Serial.print("Configuring mDNS...");
  if( !mdns.begin(MDNS_NAME, wifi) ) {
    Serial.println("failed! Halting.");
    while(1);
  } else {
    Serial.print(MDNS_NAME);
    Serial.println(".local");
  }

  Serial.print("Starting server on port ");
  Serial.print(LISTEN_PORT, DEC);
  Serial.println("...");
  server.begin();

  Serial.print("Configuring NeoPixels...");
  setupStrip();
  Serial.println("success!");

  Serial.print("Done! Free RAM: ");
  Serial.print(getFreeRam(), DEC);
  Serial.println(" bytes");
}

void loop(void) {
  // handle mDNS requests
  mdns.update();

  Adafruit_CC3000_ClientRef client = server.available();
  if( client ) {
    if( client.available() > 0 ) {
      uint8_t ch = client.read();
      server.write(ch);
    }
  }
}

void connect(void) {
  if( wifi.deleteProfiles() ) {
    Serial.print(".");
  } else {
    Serial.println("failed! Halting.");
    while(1);
  }

  if( wifi.connectToAP(WIFI_SSID, WIFI_PASSWORD, WIFI_SECURITY) ) {
    Serial.print(".");
  } else {
    Serial.println("failed! Halting.");
    while(1);
  }
}

// pretty much stolen from Adafruit_CC3000::printIPdotsRev()
void printIP(void) {
  uint32_t ip;
  tNetappIpconfigRetArgs ipconfig;

  wifi.getIPConfig(&ipconfig);
  memcpy(&ip, ipconfig.aucIP, 4);

  Serial.print((uint8_t)(ip >> 24));
  Serial.print(".");
  Serial.print((uint8_t)(ip >> 16));
  Serial.print(".");
  Serial.print((uint8_t)(ip >> 8));
  Serial.print(".");
  Serial.print((uint8_t)(ip));
}

void setupStrip(void) {
  // initialize and blank the strip
  strip.begin();
  strip.show();

  uint32_t white = strip.Color(255,255,255);

  for(int i = 0; i < 255; i++) {
    setStripColor(white);
    strip.setBrightness(i);
    strip.show();
    delay(3);
  }

  for(int i = 255; i > 0; i--) {
    setStripColor(white);
    strip.setBrightness(i);
    strip.show();
    delay(3);
  }

  // blank the strip again
  setStripColor(strip.Color(0,0,0));
  strip.setBrightness(255);
  strip.show();
}

void setStripColor(uint32_t color) {
  for(int i = 0; i < NEOPIXEL_COUNT; i++) {
    strip.setPixelColor(i, color);
  }
}

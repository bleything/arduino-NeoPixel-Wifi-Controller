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

// Stash these here for RAM savings and also DRY. Change at your own peril.
#define MSG_HALTING F("failed! Halting.")
#define MSG_SUCCESS F("success!")
#define MSG_DOT     F(".")

Adafruit_CC3000 wifi = Adafruit_CC3000(PIN_CS, PIN_IRQ, PIN_VBAT, SPI_CLOCK_DIV2);
Adafruit_CC3000_Server server(LISTEN_PORT);
MDNSResponder mdns;

////////////////////////////////////////////////////////////////////////////////

void setup(void) {
  // shut up, Adafruit_CC3000
  CC3KPrinter = 0;

  Serial.begin(115200);
  Serial.print(F("Initializing CC3000..."));

  if( wifi.begin() ) {
    Serial.println(MSG_SUCCESS);
  } else {
    halt();
  }

  Serial.print(F("Connecting to AP."));
  connect();
  Serial.println(MSG_SUCCESS);

  Serial.print(F("Getting an IP address from DHCP."));
  while( !wifi.checkDHCP() ) {
    Serial.print(MSG_DOT);
    delay(100);
  }

  printIP();
  Serial.println();

  Serial.print(F("Configuring mDNS..."));
  if( !mdns.begin(MDNS_NAME, wifi) ) {
    halt();
  } else {
    Serial.print(MDNS_NAME);
    Serial.println(F(".local"));
  }

  Serial.print(F("Starting server on port "));
  Serial.print(LISTEN_PORT, DEC);
  Serial.println(F("..."));
  server.begin();

  Serial.print(F("Configuring NeoPixels..."));
  setupStrip();
  Serial.println(MSG_SUCCESS);

  Serial.print(F("Done! Free RAM: "));
  Serial.print(getFreeRam(), DEC);
  Serial.println(F(" bytes"));
}

void loop(void) {
  // handle mDNS requests
  mdns.update();

  // grab a waiting client connection
  Adafruit_CC3000_ClientRef client = server.available();

  // commands are 5 bytes long where the first two bytes are the address of the
  // pixel to change (or 0xFFFF for the entire strip) followed by R, G, and B
  // values.
  //
  if( client && client.available() > 0 ) {
    Serial.print(F("Reading command"));
    byte cmd[5];

    for( int i = 0; i < 5; i++ ) {
      cmd[i] = client.read();
      Serial.print(MSG_DOT);
    }

    unsigned int addr = cmd[0] | (cmd[1] << 8);

    Serial.print(addr);
    Serial.print(F(": "));
    Serial.print(cmd[2]);
    Serial.print(F(","));
    Serial.print(cmd[3]);
    Serial.print(F(","));
    Serial.println(cmd[4]);

    uint32_t color = strip.Color(cmd[2], cmd[3], cmd[4]);

    if( addr == 0xFFFF ) {
      for(uint16_t i = 0; i < NEOPIXEL_COUNT; i++) {
        strip.setPixelColor(i, color);
      }
    } else {
      strip.setPixelColor(addr, color);
    }

    strip.show();
  }
}

void connect(void) {
  if( wifi.deleteProfiles() ) {
    Serial.print(MSG_DOT);
  } else {
    halt();
  }

  if( wifi.connectToAP(WIFI_SSID, WIFI_PASSWORD, WIFI_SECURITY) ) {
    Serial.print(MSG_DOT);
  } else {
    halt();
  }
}

// pretty much stolen from Adafruit_CC3000::printIPdotsRev()
void printIP(void) {
  uint32_t ip;
  tNetappIpconfigRetArgs ipconfig;

  wifi.getIPConfig(&ipconfig);
  memcpy(&ip, ipconfig.aucIP, 4);

  Serial.print((uint8_t)(ip >> 24));
  Serial.print(MSG_DOT);
  Serial.print((uint8_t)(ip >> 16));
  Serial.print(MSG_DOT);
  Serial.print((uint8_t)(ip >> 8));
  Serial.print(MSG_DOT);
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

void halt(void) {
  Serial.println(MSG_HALTING);
  while(1);
}

#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <NeoPixelBus.h>
#include <NetworkClient.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_mac.h>
#include <LittleFS.h>
#include <FS.h>

#include <NetworkManager.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include <const.h>


// NeoPixelBus pointer (will be initialized later)
NeoPixelBus<RGB_ORDER, STRIP_TYPE>* strip;

Preferences preferences;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Initialization of variables for FPS calculation
unsigned long lastTime = 0;
unsigned long frameTime = 0;
unsigned long frames = 0;
float fps;

// Conditional setup depending on the enviroment
#ifdef HAS_ETH
    #include <ETH.h>
    #include <ArtnetETH.h>
    ArtnetETHReceiver artnet;
    static bool eth_connected = false;
    void onEvent(arduino_event_id_t event);
#else
    #include <ArtnetWiFi.h>
    ArtnetWiFiReceiver artnet;
#endif


// Initialization of functions
void onArtNetFrame(const uint8_t* data, const uint16_t size, const art_net::art_dmx::Metadata& metadata, const art_net::RemoteInfo& remote);

void checkAndUpdate();

void fill_ledstrip(RgbColor color);
void stars_ledstrip(uint8_t colorMax, uint8_t starFrequency);

void setup_network();
void setup_webserver();

void loadSettings();
void saveSettings();

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

#endif // MAIN_H
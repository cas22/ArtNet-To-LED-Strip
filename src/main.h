#ifndef MAIN_H
#define MAIN_H

// Basic includes
#include <Arduino.h>
#include <NeoPixelBus.h>
#include <NetworkClient.h>
#include <HTTPClient.h>
#include <Update.h>
#include <const.h>

#if ((NUM_PIXELS / GROUP_LED) * 3) % 510 != 0
    #warning The last DMX universe is NOT getting completely used! Check if making groups smaller helps. (This warning can be ignored if this behaviour is intended)
#endif

#pragma message("firmware_url is set to: " firmware_url)

NeoPixelBus<RGB_ORDER, STRIP_TYPE> strip(NUM_PIXELS, DATA_PIN);

// Conditional setup depending on the env
#ifdef wt32eth01
    #include <ETH.h>
    #include <ArtnetETH.h>
    ArtnetETHReceiver artnet;
    static bool eth_connected = false;
    void onEvent(arduino_event_id_t event);
#endif

#ifdef esp32
    #include <ArtnetWiFi.h>
    ArtnetWiFiReceiver artnet;
#endif


// Initialization of functions
void onArtNetFrame(const uint8_t* data, const uint16_t size, const art_net::art_dmx::Metadata& metadata, const art_net::RemoteInfo& remote);

void checkAndUpdate();

void fill_ledstrip(RgbColor color);
void stars_ledstrip(uint8_t colorMax, uint8_t starFrequency);
void setup_network();

#endif // MAIN_H
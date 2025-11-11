#ifndef MAIN_H
#define MAIN_H

// Basic includes
#include <Arduino.h>
#include <NeoPixelBus.h>
#include <ETH.h>
#include <ArtnetETH.h>
#include <NetworkClient.h>
#include <HTTPClient.h>
#include <Update.h>
#include <const.h>

#pragma message("firmware_url is set to: " firmware_url)

static bool eth_connected = false;

// Initialization of functions
void onArtNetFrame(const uint8_t* data, const uint16_t size, const art_net::art_dmx::Metadata& metadata, const art_net::RemoteInfo& remote);

void onEvent(arduino_event_id_t event);

void checkAndUpdate();

void fill_ledstrip(RgbColor color);
void stars_ledstrip(uint8_t colorMax, uint8_t starFrequency);


#endif // MAIN_H
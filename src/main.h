#ifndef MAIN_H
#define MAIN_H

// Basic includes
#include <Arduino.h>
#include <NeoPixelBus.h>
#include <ArtnetETH.h>

#if __has_include("secrets.h")
	#include "secrets.h"
# else

/*
	Customization

		· OTA Setup
		
			firmware_url: La URL donde se encuentra el archivo .bin para actualizar el ESP
			version_url: La URL donde se encuentra el archivo que contiene la version actual de el firmware que se
						 encuentra en el servidor OTA, el cual usaremos para comprobar si es necesario actualizar el ESP
		
		· Name Reporting

			ETH_HOST_NAME: The host name for the ETH interface to report on your LAN network
			ARTNET_ShortName: Short name for ArtNet Nodes to display
			ARTNET_LongName: A longer, more descriptive name for ArtNet Nodes to display
			ARTNET_NodeReport: Name for the ArtNet Node, usually the sane as the ShortName
*/

	const char* firmware_url = "http://example.com/OTA/firmware.bin";
	const char* version_url = "http://example.com/OTA/version.txt";

	#define ETH_HOST_NAME "ESP_ArtNet"
	#define ARTNET_ShortName "ESP_ArtNet"
	#define ARTNET_LongName "ESP_ArtNet2NeoPixel"
	#define ARTNET_NodeReport "ESP_ArtNet"

#endif

/* 
	LED Strip Setup:

	NUM_PIXELS: Cuantos LEDs tiene la tira (*)
	GROUP_LED: Para cada 3 canales de DMX cuantos LEDs queremos controlar
	DATA_PIN: El pin al que esta conectada la tira LED
	RGB_ORDER: Si la tira led tiene los colores cambiados podemos cambiar el orden (RGB, GRB, BGR...)
	STRIP_TYPE: Tipo de tira LED que estamos usando

		(*) Si tienes una tira LED con un chip IC cada n LEDs, el numero de PIXELES que hay que introducir
			en el programa sera NUM_LEDs / n (Siendo NUM_LEDs la cantidad de LEDs que tiene fisicamente la tira).
			Veamos un ejemplo:
				900 LEDs -> Agrupados en 3 -> 900/3=300 pixeles (x 3 = 900 canales DMX)
			No debemos confundir la agrupacion de LEDs fisica con la realizada por software, ya que esta ultima
			se realiza configurando GROUP_LED, mientras que la otra se hace con las instrucciones anteriores.
*/

#define NUM_PIXELS 626
#define GROUP_LED 1
#define DATA_PIN 2
#define RGB_ORDER NeoGrbFeature
#define STRIP_TYPE NeoWs2812xMethod



// Initialization of functions
void onArtNetFrame(const uint8_t* data, const uint16_t size, const art_net::art_dmx::Metadata& metadata, const art_net::RemoteInfo& remote);

void onEvent(arduino_event_id_t event);

void checkAndUpdate();

void fill_ledstrip(RgbColor color);
void stars_ledstrip(uint8_t colorMax, uint8_t starFrequency);


#endif // MAIN_H
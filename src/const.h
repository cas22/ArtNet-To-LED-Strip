#ifndef CONST_H
#define CONST_H

/*
	Customization

		· OTA Setup
		
			BASE_URL: The base of the URL where the OTA files are located
            firmware_url: The URL where we can find the .bin to update the ESP
			version_url: The URL where we can find the file which contains the current version of the firmware located
					     on the OTA server, which is used to check if the ESP needs to be updated
		
		· Name Reporting

			ETH_HOST_NAME: The host name for the ETH interface to report on your LAN network
			ARTNET_ShortName: Short name for ArtNet Nodes to display
			ARTNET_LongName: A longer, more descriptive name for ArtNet Nodes to display
			ARTNET_NodeReport: Name for the ArtNet Node, usually the sane as the ShortName
*/

#define BASE_URL "https://raw.githubusercontent.com/cas22/ArtNet-To-LED-Strip/refs/heads/main/builds/" ENV_NAME

#define firmware_url BASE_URL "/firmware.bin"
#define version_url BASE_URL "/version.txt"


#define ETH_HOST_NAME "ESP_ArtNet"
#define ARTNET_ShortName "ESP_ArtNet"
#define ARTNET_LongName "ESP_ArtNet2NeoPixel"
#define ARTNET_NodeReport "ESP_ArtNet"



/* 
	LED Strip Setup:

	NUM_PIXELS: How many LEDs the strip has (*)
	GROUP_LED: For every 3 DMX channels, how many LEDs we want to control
	DATA_PIN: The pin to which the LED strip is connected
	RGB_ORDER: If the LED strip has swapped color order, we can change it (RGB, GRB, BGR...)
	STRIP_TYPE: Type of LED strip we're using

		(*) If your LED strip has a driver IC every n LEDs, the number of PIXELS to enter
			in the program should be NUM_LEDs / n (where NUM_LEDs is the physical LED count of the strip).
			Example:
				900 LEDs -> grouped in 3 -> 900/3 = 300 pixels (x 3 = 900 DMX channels)
			Don't confuse the physical grouping of LEDs with software grouping — the latter is configured
			with GROUP_LED, while the former is determined by the instructions above.
*/

#define NUM_PIXELS 626
#define GROUP_LED 1
#define DATA_PIN 2
#define RGB_ORDER NeoGrbFeature
#define STRIP_TYPE NeoWs2812xMethod

#endif

// WiFi Setup
#ifdef esp32
	const char* ssid = "your-ssid";
	const char* pwd = "your-password";
	const IPAddress ip(192, 168, 1, 201);
	const IPAddress gateway(192, 168, 1, 1);
	const IPAddress subnet_mask(255, 255, 255, 0);
#endif
#ifndef CONST_H
#define CONST_H

/*
	OTA Setup
	
		BASE_URL: The base of the URL where the OTA files are located (Not required)
		firmware_url: The URL where we can find the .bin to update the ESP
		version_url: The URL where we can find the file which contains the current version of the firmware located
						on the OTA server, which is used to check if the ESP needs to be updated
*/
#define BASE_URL "https://raw.githubusercontent.com/cas22/ArtNet-To-LED-Strip/refs/heads/main/builds/" ENV_NAME
struct OTADeviceSettings {
	String firmware_url = BASE_URL "/firmware.bin";
	String version_url = BASE_URL "/version.txt";
};
OTADeviceSettings OTASettings;


/*
	ArtNet Setup

		shortName: Short name for ArtNet Nodes to display
		longName: A longer, more descriptive name for ArtNet Nodes to display
		nodeReport: Name for the ArtNet Node, usually the sane as the ShortName
*/
struct ArtNetDeviceSettings {
	String shortName = "ESP_ArtNet";
	String longName = "ESP_ArtNet2NeoPixel";
	String nodeReport = "ESP_ArtNet";
};
ArtNetDeviceSettings ArtNetSettings;


/* 
	LED Strip Setup:

	numPixels: How many LEDs the strip has (*)
	groupLED: For every 3 DMX channels, how many LEDs we want to control
	startUniverse: Starting universe for ArtNet (0 is the first universe)
	dataPin: The pin to which the LED strip is connected
	RGB_ORDER: If the LED strip has swapped color order, we can change it (RGB, GRB, BGR...)
	STRIP_TYPE: Type of LED strip we're using

		(*) If your LED strip has a driver IC every n LEDs, the number of PIXELS to enter
			in the program should be NUM_LEDs / n (where NUM_LEDs is the physical LED count of the strip).
			Example:
				900 LEDs -> grouped in 3 -> 900/3 = 300 pixels (x 3 = 900 DMX channels)
			Don't confuse the physical grouping of LEDs with software grouping — the latter is configured
			with GROUP_LED, while the former is determined by the instructions above.
*/

struct DeviceSettings {
    int numPixels = 626;
	int groupLED = 1; 
	int startUniverse = 0; 
    int dataPin = 4; 
    bool isConfigured = false; // Flag to check if settings have been saved before
};
DeviceSettings Settings;

#define RGB_ORDER NeoGrbFeature
#define STRIP_TYPE NeoWs2812xMethod

/*
	LAN/WiFi Setup

	hostName: The hostname for the device on the network

	ssid: Your WiFi network's name
	pwd: Your WiFi network's password

	ip: The static IP address of the ESP
	gateway: Your router's IP address
	subnet_mask: The IP mask of your network
*/

String hostName = "ESP_ArtNet";

struct WiFiDeviceSettings {
	String ssid = "-";
	String pwd = "-";

	IPAddress ip = IPAddress(192, 168, 1, 222);
	IPAddress gateway = IPAddress(192, 168, 1, 1);
	IPAddress subnet_mask = IPAddress(255, 255, 255, 0);
	
	bool dhcpEnabled = false;
};
WiFiDeviceSettings WiFiSettings;

#endif
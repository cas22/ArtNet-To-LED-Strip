#include <main.h>

/*
	Since each pixel has 3 DMX channels, we cannot use all channels, because DMX supports 512 channels
	which is not divisible by 3, leaving us with 2 unused channels.
	Therefore, we work with 510 channels and due to how the ArtNet library works, we subtract one since
	the vector starts at 0.

	510 / 3 = 170, which is multiplied by the universe to calculate which LED we are at when we are in a
	universe != 0.
*/


void onArtNetFrame(const uint8_t *data, const uint16_t size, const art_net::art_dmx::Metadata &metadata, const art_net::RemoteInfo &remote)
{
	int ledpos = metadata.universe * 170;
	for (size_t i = 0; i <= 509 && ledpos < Settings.numPixels; i += 3)
	{
		strip->SetPixelColor(ledpos, RgbColor(data[i], data[i + 1], data[i + 2]));
		ledpos++;
	}
	strip->Show();
}

void onArtNetFrameGroup(const uint8_t *data, const uint16_t size, const art_net::art_dmx::Metadata &metadata, const art_net::RemoteInfo &remote)
{
	int ledpos = metadata.universe * (170);
	for (size_t i = 0; i <= 509 && ledpos < Settings.numPixels; i += 3)
	{
		for (size_t j = 1; j <= Settings.groupLED && ledpos < Settings.numPixels; j++)
		{
			strip->SetPixelColor(ledpos, RgbColor(data[i], data[i + 1], data[i + 2]));
			ledpos++;
		}
	}
	strip->Show();
}


void setup()
{
	Serial.begin(115200);
	Serial.println("[Info] Current version: " + String(VERSION));

	loadSettings();

	// NeoPixelBus setup
	strip = new NeoPixelBus<RGB_ORDER, STRIP_TYPE>(
        Settings.numPixels, 
        Settings.dataPin
    );

	Serial.println("[NeoPixelBus] Starting using pin " + String(Settings.dataPin));
	strip->Begin();
	Serial.printf("[NeoPixelBus] Started %d pixels", Settings.numPixels);
	strip->Show();

	setup_network();

	Serial.println("\n[OTA] Checking if updates are available");
	checkAndUpdate();

	Serial.println("\n[ArtNet] setArtPollReplyConfig");
	artnet.setArtPollReplyConfigShortName(ARTNET_ShortName);
	artnet.setArtPollReplyConfigLongName(ARTNET_LongName);
	artnet.setArtPollReplyConfigNodeReport(ARTNET_NodeReport);
	stars_ledstrip(200, 10);

	Serial.println("\n[ArtNet] Starting");
	artnet.begin();

	Serial.println("[ArtNet] Subscribing to Universes");
	if (Settings.groupLED == 1) {
		artnet.subscribeArtDmx(onArtNetFrame);
	} else {
		artnet.subscribeArtDmx(onArtNetFrameGroup);
	}
	

	Serial.printf("[Info] Free Heap %d out of %d", ESP.getFreeHeap(), ESP.getHeapSize());
}

void loop()
{
	artnet.parse();
}

/*----- LED Strip Helpers -----*/
void fill_ledstrip(RgbColor color)
{
	for (size_t i = 0; i < Settings.numPixels; i++)
	{
		strip->SetPixelColor(i, color);
	}
	strip->Show();
}

void stars_ledstrip(uint8_t colorMax, uint8_t starFrequency)
{
	for (size_t i = 0; i < Settings.numPixels; i++)
	{
		if (random(0, starFrequency) == starFrequency)
		{
			strip->SetPixelColor(i, RgbColor(random(0, colorMax)));
		}
	}
	strip->Show();
}

/*----- Network -----*/
#ifdef wt32eth01
void setup_network()
{
	uint8_t animBrightness = 0;
	Network.onEvent(onEvent);
	ETH.begin();
	while (!eth_connected)
	{
		delay(20);
		fill_ledstrip(RgbColor(animBrightness));
		animBrightness = (animBrightness == 250) ? 0 : animBrightness + 1;
	}
	fill_ledstrip(RgbColor(0, 255, 0));
	Serial.println("\n[ETH] Connected to the network");
}

void onEvent(arduino_event_id_t event)
{
	switch (event)
	{
	case ARDUINO_EVENT_ETH_START:
		Serial.println("[ETH] Started");
		ETH.setHostname(ETH_HOST_NAME);
		break;
	case ARDUINO_EVENT_ETH_CONNECTED:
		Serial.println("[ETH] Connected");
		break;
	case ARDUINO_EVENT_ETH_GOT_IP:
		Serial.println("[ETH] Got IP " + String(ETH.localIP()));
		eth_connected = true;
		break;
	case ARDUINO_EVENT_ETH_DISCONNECTED:
		Serial.println("[ETH] Disconnected");
		eth_connected = false;
		break;
	case ARDUINO_EVENT_ETH_STOP:
		Serial.println("[ETH] Stopped");
		eth_connected = false;
		break;
	default:
		break;
	}
}
#endif

#ifdef esp32
void setup_network()
{
	uint8_t animBrightness = 0;
	WiFi.begin(WiFiSettings.ssid, WiFiSettings.pwd);
	WiFi.config(WiFiSettings.ip, WiFiSettings.gateway, WiFiSettings.subnet_mask);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		fill_ledstrip(RgbColor(animBrightness));
		animBrightness = (animBrightness == 250) ? 0 : animBrightness + 1;
	}
	fill_ledstrip(RgbColor(0, 255, 0));
	Serial.printf("\n[WiFi] Connected to the network with IP: %d", WiFi.localIP());
	Serial.printf("[WiFi] Provided IP was: %d", WiFiSettings.ip);
}
#endif

/*----- OTA -----*/
void checkAndUpdate()
{
	HTTPClient http;
	// Step 1: Check version
	http.begin(OTASettings.version_url);
	int versionCode = http.GET();
	if (versionCode == HTTP_CODE_OK)
	{
		String serverVersion = http.getString();
		serverVersion.trim();

		Serial.println("[OTA] Current version: " + String(VERSION));
		Serial.println("[OTA] Server version: " + serverVersion);

		if (serverVersion == VERSION)
		{
			Serial.println("[OTA] Firmware is up to date.");
			http.end();
			return;
		}
		else
		{
			Serial.println("[OTA] New firmware available. Starting OTA update...");
		}
	}
	else
	{
		Serial.println("[OTA] Failed to fetch version info. HTTP Code: " + String(versionCode));
		http.end();
		return;
	}
	http.end();

	// Step 2: Get Firmware
	http.begin(OTASettings.firmware_url);
	int httpCode = http.GET();

	if (httpCode == HTTP_CODE_OK)
	{
		int contentLength = http.getSize();
		NetworkClient *stream = http.getStreamPtr();
		fill_ledstrip(RgbColor(255, 255, 0));
		if (Update.begin(contentLength))
		{
			size_t written = Update.writeStream(*stream);
			if (written == contentLength)
			{
				Serial.println("[OTA] Written : " + String(written) + " successfully");
			}
			else
			{
				Serial.println("[OTA] Written only : " + String(written) + "/" + String(contentLength));
			}

			if (Update.end())
			{
				if (Update.isFinished())
				{
					Serial.println("[OTA] Update successfully completed. Rebooting...");
					ESP.restart();
				}
				else
				{
					Serial.println("[OTA] Update not finished? Something went wrong.");
				}
			}
			else
			{
				Serial.printf("[OTA] Error Occurred. Error #: %d\n", Update.getError());
			}
		}
		else
		{
			Serial.println("[OTA] Not enough space to begin");
			fill_ledstrip(RgbColor(255, 0, 0));
		}
	}
	else
	{
		Serial.println("[OTA] Firmware not available. HTTP Code: " + String(httpCode));
	}
	http.end();
}

/*----- Preferences -----*/
void loadSettings() {
    preferences.begin("device-config", false);

    // Load each setting with a default fallback
    Settings.isConfigured = preferences.getBool("configured", false);

    if (Settings.isConfigured) {
        Serial.println("[Preferences] Loading saved settings...");
        Settings.numPixels = preferences.getInt("num-pixels", Settings.numPixels);
        Settings.dataPin = preferences.getInt("data-pin", Settings.dataPin);
		Settings.groupLED = preferences.getInt("group-led", Settings.groupLED);
		Settings.startUniverse = preferences.getInt("start-universe", Settings.startUniverse);
		#ifdef esp32
			WiFiSettings.ssid = preferences.getString("wifi-ssid", WiFiSettings.ssid).c_str();
			WiFiSettings.pwd = preferences.getString("wifi-pwd", WiFiSettings.pwd).c_str();
			WiFiSettings.ip = IPAddress(preferences.getUInt("wifi-ip", WiFiSettings.ip));
			WiFiSettings.gateway = IPAddress(preferences.getUInt("wifi-gateway", WiFiSettings.gateway));
			WiFiSettings.subnet_mask = IPAddress(preferences.getUInt("wifi-subnet", WiFiSettings.subnet_mask));
		#endif
    } else {
        Serial.println("[Preferences] First run! Using default settings.");
        // Defaults are already set in the structure definition
        saveSettings(); 
    }

    preferences.end(); // Close the Preferences
}

void saveSettings() {
    preferences.begin("device-config", false);

    // Save each setting
    preferences.putInt("num-pixels", Settings.numPixels);
    preferences.putInt("data-pin", Settings.dataPin);
	preferences.putInt("group-led", Settings.groupLED);
    preferences.putInt("start-universe", Settings.startUniverse);

	#ifdef esp32
		preferences.putString("wifi-ssid", String(WiFiSettings.ssid));
		preferences.putString("wifi-pwd", String(WiFiSettings.pwd));
		preferences.putUInt("wifi-ip", (uint32_t)WiFiSettings.ip);
		preferences.putUInt("wifi-gateway", (uint32_t)WiFiSettings.gateway);
		preferences.putUInt("wifi-subnet", (uint32_t)WiFiSettings.subnet_mask);
	#endif

    // Mark as configured after initial save
    preferences.putBool("configured", true); 
    Settings.isConfigured = true;

    preferences.end();
    Serial.println("[Preferences] Settings saved successfully.");
}
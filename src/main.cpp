#include <main.h>

ArtnetETHReceiver artnet;
NeoPixelBus<RGB_ORDER, STRIP_TYPE> strip(NUM_PIXELS, DATA_PIN);

/*
	Since each pixel has 3 DMX channels, we cannot use all channels, because DMX supports 512 channels
	which is not divisible by 3, leaving us with 2 unused channels.
	Therefore, we work with 510 channels and due to how the ArtNet library works, we subtract one since
	the vector starts at 0.
	
	510 / 3 = 170, which is multiplied by the universe to calculate which LED we are at when we are in a
	universe != 0.
*/

#if GROUP_LED == 1 
void onArtNetFrame(const uint8_t* data, const uint16_t size, const art_net::art_dmx::Metadata& metadata, const art_net::RemoteInfo& remote) {
	int ledpos = metadata.universe*170;
	for (size_t i = 0; i <= 509 && ledpos < NUM_PIXELS; i += 3) {
		strip.SetPixelColor(ledpos, RgbColor(data[i], data[i+1], data[i+2]));
		ledpos++;
	}
    strip.Show();
}
#else
void onArtNetFrame(const uint8_t* data, const uint16_t size, const art_net::art_dmx::Metadata& metadata, const art_net::RemoteInfo& remote) {
	int ledpos = metadata.universe*(170);
	for (size_t i = 0; i <= 509 && ledpos < NUM_PIXELS; i += 3) {
		for(size_t j=1; j <= GROUP_LED; j++){
			strip.SetPixelColor(ledpos, RgbColor(data[i], data[i+1], data[i+2]));
			ledpos++;
		}
	}
    strip.Show();
}
#endif

void setup() {
	Serial.begin(115200);
	Serial.println("[Info] Current version: " + String(VERSION));

	// NeoPixelBus setup
	Serial.println("[NeoPixelBus] Starting using pin " + String(DATA_PIN));
	strip.Begin();
	Serial.printf("[NeoPixelBus] Started %d pixels", NUM_PIXELS);
  strip.Show();

	uint8_t animBrightness = 0;
	Network.onEvent(onEvent);
	ETH.begin();
	while(!eth_connected) {
		delay(20);
		fill_ledstrip(RgbColor(animBrightness));
		animBrightness = (animBrightness == 250) ? 0 : animBrightness+1;
	}
	fill_ledstrip(RgbColor(0,255,0));
	Serial.println("\n[ETH] Connected to the network");

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
	artnet.subscribeArtDmx(onArtNetFrame);

	Serial.printf("[Info] Free Heap %d out of %d", ESP.getFreeHeap(), ESP.getHeapSize());
}

void loop() {	
	artnet.parse();
}


/*----- LED Strip Helpers -----*/
void fill_ledstrip(RgbColor color){
	for(size_t i = 0; i<NUM_PIXELS; i++){
		strip.SetPixelColor(i, color);
	}
	strip.Show();
}

void stars_ledstrip(uint8_t colorMax, uint8_t starFrequency){
	for(size_t i = 0; i<NUM_PIXELS; i++){
		if(random(0,starFrequency) == starFrequency) {
			strip.SetPixelColor(i, RgbColor(random(0,colorMax)));
		}
	}
	strip.Show();
}


/*----- Network -----*/
void onEvent(arduino_event_id_t event) {
  switch (event) {
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
    default: break;
  }
}


/*----- OTA -----*/
void checkAndUpdate() {
	HTTPClient http;
	// Step 1: Check version
	http.begin(version_url);
	int versionCode = http.GET();
	if (versionCode == HTTP_CODE_OK) {
		String serverVersion = http.getString();
		serverVersion.trim();

		Serial.println("[OTA] Current version: " + String(VERSION));
		Serial.println("[OTA] Server version: " + serverVersion);

		if (serverVersion == VERSION) {
			Serial.println("[OTA] Firmware is up to date.");
			http.end();
			return;
		}
		else {
			Serial.println("[OTA] New firmware available. Starting OTA update...");
		}
	}
	else {
		Serial.println("[OTA] Failed to fetch version info. HTTP Code: " + String(versionCode));
		http.end();
		return;
	}
	http.end();
	
	// Step 2: Get Firmware
	http.begin(firmware_url);
	int httpCode = http.GET();

	if (httpCode == HTTP_CODE_OK) {
		int contentLength = http.getSize();
		NetworkClient* stream = http.getStreamPtr();
		fill_ledstrip(RgbColor(255, 255, 0));
		if (Update.begin(contentLength)) {
			size_t written = Update.writeStream(*stream);
			if (written == contentLength) {
				Serial.println("[OTA] Written : " + String(written) + " successfully");
			}
			else {
				Serial.println("[OTA] Written only : " + String(written) + "/" + String(contentLength));
			}

			if (Update.end()) {
				if (Update.isFinished()) {
					Serial.println("[OTA] Update successfully completed. Rebooting...");
					ESP.restart();
				}
				else {
					Serial.println("[OTA] Update not finished? Something went wrong.");
				}
			}
			else {
				Serial.printf("[OTA] Error Occurred. Error #: %d\n", Update.getError());
			}
		}
		else {
			Serial.println("[OTA] Not enough space to begin");
			fill_ledstrip(RgbColor(255, 0, 0));
		}
	}
	else {
		Serial.println("[OTA] Firmware not available. HTTP Code: " + String(httpCode));
	}
	http.end();
}
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
	unsigned long currentTime = millis();
	int ledpos = metadata.universe * 170;
	for (size_t i = 0; i <= 509 && ledpos < Settings.numPixels; i += 3)
	{
		strip->SetPixelColor(ledpos, RgbColor(data[i], data[i + 1], data[i + 2]));
		ledpos++;
	}
	strip->Show();

	frameTime = millis() - lastTime;
	frames++;

	if (frameTime >= 1000) { // After 1 second, print the FPS
		fps = frames / (frameTime / 1000.0);
		frames = 0;
		lastTime = millis();
	}
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
	#ifdef DEBUG
	pinMode(2, OUTPUT);
	delay(5000);
	Serial.println("[Info] DEBUG ON");
	#endif
	Serial.println("[Info] Current version: " + String(VERSION));

	Serial.println("\n--- Memory Info ---");

	// --- Internal RAM (Heap) ---
	uint32_t totalHeap = ESP.getHeapSize();
	uint32_t freeHeap = ESP.getFreeHeap();

	Serial.println("--- Internal RAM (Heap) ---");
	// Print Total Heap in Bytes, KB, and MB
	Serial.printf("Total: %u bytes | %.2f KB | %.2f MB\n",
					totalHeap,
					totalHeap / 1024.0,
					totalHeap / (1024.0 * 1024.0));

	// Print Free Heap in Bytes, KB, and MB
	Serial.printf("Free:  %u bytes | %.2f KB | %.2f MB\n",
					freeHeap,
					freeHeap / 1024.0,
					freeHeap / (1024.0 * 1024.0));

	// --- PSRAM (if available) ---
	if (psramFound()) {
		uint32_t totalPsram = ESP.getPsramSize();
		uint32_t freePsram = ESP.getFreePsram();

		Serial.println("\n--- PSRAM Info ---");
		// Print Total PSRAM in Bytes, KB, and MB
		Serial.printf("Total: %u bytes | %.2f KB | %.2f MB\n",
					totalPsram,
					totalPsram / 1024.0,
					totalPsram / (1024.0 * 1024.0));

		// Print Free PSRAM in Bytes, KB, and MB
		Serial.printf("Free:  %u bytes | %.2f KB | %.2f MB\n",
					freePsram,
					freePsram / 1024.0,
					freePsram / (1024.0 * 1024.0));
	} else {
		Serial.println("\nNo PSRAM found on this board.");
	}

	Serial.println("-------------------------");


	loadSettings();

	// NeoPixelBus setup
	strip = new NeoPixelBus<RGB_ORDER, STRIP_TYPE>(
        Settings.numPixels, 
        Settings.dataPin
    );

	Serial.println("[NeoPixelBus] Starting using pin " + String(Settings.dataPin));
	strip->Begin();
	Serial.printf("[NeoPixelBus] Started %d pixels\n", Settings.numPixels);
	strip->Show();

	setup_network();

	Serial.println("\n[OTA] Checking if updates are available");
	checkAndUpdate();

	// --- Initialize WebSocket Server ---
	ws.onEvent(onWsEvent);        // Attach the event handler
	server.addHandler(&ws);       // Add WebSocket handler to the server

	// --- Initialize Web Server ---
	// Serve the embedded index.html file at the root URL ("/")
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		// Send the HTML file stored in PROGMEM
		request->send_P(200, "text/html", index_html);
	});

	// API: GET /api/settings - Return current settings as JSON
	server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
		String json = "{";
		json += "\"numPixels\":" + String(Settings.numPixels) + ",";
		json += "\"dataPin\":" + String(Settings.dataPin) + ",";
		json += "\"groupLED\":" + String(Settings.groupLED) + ",";
		json += "\"startUniverse\":" + String(Settings.startUniverse);
		json += ",";
		json += "\"ssid\":\"" + WiFiSettings.ssid + "\",";
		json += "\"pwd\":\"" + WiFiSettings.pwd + "\",";
		json += "\"dhcpEnabled\":" + String(WiFiSettings.dhcpEnabled ? "true" : "false") + ",";
		json += "\"ip\":\"" + WiFiSettings.ip.toString() + "\",";
		json += "\"gateway\":\"" + WiFiSettings.gateway.toString() + "\",";
		json += "\"subnet\":\"" + WiFiSettings.subnet_mask.toString() + "\"";
		json += "}";
		request->send(200, "application/json", json);
		Serial.println("[API] GET /api/settings - Settings returned");
	});

	// API: POST /api/settings - Update settings and save to preferences
	server.on("/api/settings", HTTP_POST, [](AsyncWebServerRequest *request) {
		// Response will be sent in onBody handler
	}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
		// Handle JSON body
		StaticJsonDocument<512> doc;
		DeserializationError error = deserializeJson(doc, data);

		if (error) {
			request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
			Serial.println("[API] POST /api/settings - JSON parse error");
			return;
		}

		// Update settings from JSON
		if (doc.containsKey("numPixels")) Settings.numPixels = doc["numPixels"];
		if (doc.containsKey("dataPin")) Settings.dataPin = doc["dataPin"];
		if (doc.containsKey("groupLED")) Settings.groupLED = doc["groupLED"];
		if (doc.containsKey("startUniverse")) Settings.startUniverse = doc["startUniverse"];

		#ifndef HAS_ETH
			if (doc.containsKey("ssid")) WiFiSettings.ssid = doc["ssid"].as<String>();
			if (doc.containsKey("pwd")) WiFiSettings.pwd = doc["pwd"].as<String>();
		#endif
		
		if (doc.containsKey("dhcpEnabled")) WiFiSettings.dhcpEnabled = doc["dhcpEnabled"].as<bool>();
		if (doc.containsKey("ip")) WiFiSettings.ip.fromString(doc["ip"].as<String>());
		if (doc.containsKey("gateway")) WiFiSettings.gateway.fromString(doc["gateway"].as<String>());
		if (doc.containsKey("subnet")) WiFiSettings.subnet_mask.fromString(doc["subnet"].as<String>());

		// Save settings to preferences
		saveSettings();

		request->send(200, "application/json", "{\"status\":\"Settings updated successfully\"}");
		Serial.println("[API] POST /api/settings - Settings updated and saved");
	});

	// Start server
	server.begin();
	Serial.println("Web server started.");

	// API: GET /api/debug - Return system debug information
	server.on("/api/debug", HTTP_GET, [](AsyncWebServerRequest *request) {
		uint32_t freeHeap = ESP.getFreeHeap();
		uint32_t totalHeap = ESP.getHeapSize();
		uint32_t freePsram = psramFound() ? ESP.getFreePsram() : 0;
		uint32_t totalPsram = psramFound() ? ESP.getPsramSize() : 0;
		
		// Calculate percentages
		float ramUsagePercent = ((totalHeap - freeHeap) * 100.0) / totalHeap;
		float psramUsagePercent = (totalPsram > 0) ? ((totalPsram - freePsram) * 100.0) / totalPsram : 0;
		
		// Get Flash info (simplified)
		uint32_t sketchSize = ESP.getSketchSize();
		uint32_t freeSketchSpace = ESP.getFreeSketchSpace();
		float flashUsagePercent = (sketchSize * 100.0) / (sketchSize + freeSketchSpace);
		
		// Build JSON response
		String json = "{";
		json += "\"fps\":" + String(fps) + ",";
		json += "\"version\":\"" + String(VERSION) + "\",";
		
		// RAM Usage
		char ramBuf[50];
		snprintf(ramBuf, sizeof(ramBuf), "%.1f%%", ramUsagePercent);
		json += "\"ram_usage\":\"" + String(ramBuf) + "\",";
		
		// PSRAM Usage
		char psramBuf[50];
		if (psramFound()) {
			snprintf(psramBuf, sizeof(psramBuf), "%.1f%%", psramUsagePercent);
		} else {
			snprintf(psramBuf, sizeof(psramBuf), "N/A");
		}
		json += "\"psram_usage\":\"" + String(psramBuf) + "\",";
		
		// Flash Usage
		char flashBuf[50];
		snprintf(flashBuf, sizeof(flashBuf), "%.1f%%", flashUsagePercent);
		json += "\"flash_usage\":\"" + String(flashBuf) + "\",";
		
		// MAC Address
		uint8_t mac[6];
		esp_read_mac(mac, ESP_MAC_WIFI_STA);
		char macStr[18];
		snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", 
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		json += "\"mac\":\"" + String(macStr) + "\",";
		
		// Current IP
		#ifdef HAS_ETH
			json += "\"ip\":\"" + String(ETH.localIP().toString()) + "\"";
		#else
			json += "\"ip\":\"" + String(WiFi.localIP().toString()) + "\"";
		#endif
		
		json += "}";
		request->send(200, "application/json", json);
		Serial.println("[API] GET /api/debug - Debug info returned");
	});

	Serial.println("\n[ArtNet] setArtPollReplyConfig");
	artnet.setArtPollReplyConfigShortName(ArtNetSettings.shortName);
	artnet.setArtPollReplyConfigLongName(ArtNetSettings.longName);
	artnet.setArtPollReplyConfigNodeReport(ArtNetSettings.nodeReport);
	stars_ledstrip(200, 10);

	Serial.println("\n[ArtNet] Starting");
	artnet.begin();

	Serial.println("[ArtNet] Subscribing to Universes");
	if (Settings.groupLED == 1) {
		artnet.subscribeArtDmx(onArtNetFrame);
	} else {
		artnet.subscribeArtDmx(onArtNetFrameGroup);
	}
	

	Serial.printf("[Info] Free Heap %d out of %d\n", ESP.getFreeHeap(), ESP.getHeapSize());
}

void loop()
{
	artnet.parse();
	ws.cleanupClients();
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
#ifdef HAS_ETH
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
		Serial.println("[ETH] Started " + hostName);
		ETH.setHostname(hostName.c_str());
		if(!WiFiSettings.dhcpEnabled) {
			ETH.config(WiFiSettings.ip, WiFiSettings.gateway, WiFiSettings.subnet_mask);
		}
		break;
	case ARDUINO_EVENT_ETH_CONNECTED:
		Serial.println("[ETH] Connected");
		break;
	case ARDUINO_EVENT_ETH_GOT_IP:
		Serial.print("[ETH] Got IP ");
		Serial.println(ETH.localIP());
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
#else
void setup_network()
{
	uint8_t animBrightness = 0;
	Serial.print("[WiFi] Begin ");
	Serial.println(WiFiSettings.ssid);
	WiFi.begin(WiFiSettings.ssid, WiFiSettings.pwd);
	Serial.println("[WiFi] Config");
	if(!WiFiSettings.dhcpEnabled) {
		WiFi.config(WiFiSettings.ip, WiFiSettings.gateway, WiFiSettings.subnet_mask);
	}
	WiFi.setHostname(hostName.c_str());
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		fill_ledstrip(RgbColor(animBrightness));
		animBrightness = (animBrightness == 250) ? 0 : animBrightness + 1;
	}
	fill_ledstrip(RgbColor(0, 255, 0));
	Serial.print("[WiFi] Connected to the network with IP: ");
	Serial.println(WiFi.localIP());
	Serial.print("[WiFi] Provided IP was: ");
	Serial.println(WiFiSettings.ip);
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

// ===== Preferences =====
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
		hostName = preferences.getString("host-name", hostName);
		ArtNetSettings.longName = preferences.getString("long-Name", ArtNetSettings.longName);
		ArtNetSettings.shortName = preferences.getString("short-Name", ArtNetSettings.shortName);
		ArtNetSettings.nodeReport = preferences.getString("node-Report", ArtNetSettings.nodeReport);
		#ifndef HAS_ETH
			WiFiSettings.ssid = preferences.getString("wifi-ssid", WiFiSettings.ssid);
			WiFiSettings.pwd = preferences.getString("wifi-pwd", WiFiSettings.pwd);
		#endif
		WiFiSettings.ip = IPAddress(preferences.getUInt("wifi-ip", WiFiSettings.ip));
		WiFiSettings.gateway = IPAddress(preferences.getUInt("wifi-gateway", WiFiSettings.gateway));
		WiFiSettings.subnet_mask = IPAddress(preferences.getUInt("wifi-subnet", WiFiSettings.subnet_mask));
		WiFiSettings.dhcpEnabled = preferences.getBool("wifi-dhcp", WiFiSettings.dhcpEnabled);
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
	preferences.putString("host-name", hostName);
	preferences.putString("long-Name", ArtNetSettings.longName);
	preferences.putString("short-Name", ArtNetSettings.shortName);
	preferences.putString("node-Report", ArtNetSettings.nodeReport);

	#ifndef HAS_ETH
		preferences.putString("wifi-ssid", WiFiSettings.ssid);
		preferences.putString("wifi-pwd", WiFiSettings.pwd);
	#endif

	preferences.putUInt("wifi-ip", (IPAddress)WiFiSettings.ip);
	preferences.putUInt("wifi-gateway", (IPAddress)WiFiSettings.gateway);
	preferences.putUInt("wifi-subnet", (IPAddress)WiFiSettings.subnet_mask);
	preferences.putBool("wifi-dhcp", WiFiSettings.dhcpEnabled);

    // Mark as configured after initial save
    preferences.putBool("configured", true); 
    Settings.isConfigured = true;

    preferences.end();
    Serial.println("[Preferences] Settings saved successfully.");
}

// ===== WebSocket Event Handler =====
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      // Handle errors
      break;
  }
}
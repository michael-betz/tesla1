#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "ESP8266WebServer.h"
#include "ArduinoWebsockets.h"
#include "player.h"
#include "LittleFS.h"

#include "comms.h"
#include "pulser.h"

// websocket server
using namespace websockets;
WebsocketsServer ws_server;
WebsocketsClient ws_client;

// http server
ESP8266WebServer http_server(80);

// -----------------
//  WS server stuff
// -----------------
static void onEventCallback(WebsocketsEvent event, String data) {
	if(event == WebsocketsEvent::ConnectionOpened) {
		Serial.print(" <open> ");
	} else if(event == WebsocketsEvent::ConnectionClosed) {
		Serial.print(" <close> ");
		stop_pulse();
	} else if(event == WebsocketsEvent::GotPing) {
		Serial.print(" <ping> ");
	} else if(event == WebsocketsEvent::GotPong) {
		Serial.print(" <pong> ");
	}
}

static void onMessageCallback(WebsocketsMessage message) {
	static unsigned op_mode = 0;
	unsigned temp_ftw = 0, temp_duty = 0;
	String tmpStr;
	char *tok = NULL;
	char *s = (char *)message.c_str();
	Dir dir;

	if (message.length() < 1)
		return;

	switch (s[0]) {
		case 'i':  // 'i' command = RSSI
			tmpStr = "{\"RSSI\": ";
			tmpStr += WiFi.RSSI();
			tmpStr += "}";
			ws_client.send(tmpStr);
			break;

		case 'l':  // 'l' list midi files
			dir = LittleFS.openDir("/m");
			while(dir.next()) {
				if (dir.isFile()) {
					ws_client.send("{\"f\": \"" + dir.fileName() + "\"}\n");
				}
			}
			break;

		case 'm':  // play a midi file
			if (message.length() > 1)
				play_file(&s[1]);
			break;

		case 'n':  // stop playback
			stop_playback();
			break;

		case 'p':  // "p,100000", single shot mode, 1000 ms
			if (op_mode == 2)
				set_single_shot(strtoul(&s[2], NULL, 0));
			break;

		case 'r':  // "r,0": set operating mode to DDS / lock mode
			if (s[2] == '0') {
				dds_mode();
				op_mode = 0;
			} else if (s[2] == '1') {
				lock_mode();
				op_mode = 1;
			} else if (s[2] == '2') {
				single_shot_mode();
				op_mode = 2;
			}
			break;

		case 's':   // "s,100,200" set ftw to 100 and duty to 200 (int32 scale)
			// split string into 3 tokens at ',' and convert to int
			for(unsigned tok_id = 0; tok_id <= 2; tok_id++) {
				tok = strsep(&s, ",");
				if (tok == NULL) {
					Serial.printf("parse error!\n");
					return;
				}
				if (tok_id == 1) 		temp_ftw = strtoul(tok, NULL, 0);
				else if (tok_id == 2)	temp_duty = strtoul(tok, NULL, 0);
			}
			set_pulse(0, temp_ftw, temp_duty);
			break;

		case 't':  // "t,100" set phase to 100
			set_phase(0, strtoul(&s[2], NULL, 0));
			break;

		// for 60 Hz locking mode
		case 'u':  // set t_pre (delay after opto rising edge) [us]
			g_t_pre = strtoul(&s[2], NULL, 0);
			break;

		case 'v':  // set t_on [us]
			g_t_on = strtoul(&s[2], NULL, 0);
			break;

		case 'w':  // set t_off [us]
			g_t_off = strtoul(&s[2], NULL, 0);
			break;

		default:
			Serial.println(message.data());
	}
}

void refresh_ws(unsigned cycle) {
	if (ws_server.poll() && !ws_client.available()) {
		ws_client = ws_server.accept();
		ws_client.onMessage(onMessageCallback);
		ws_client.onEvent(onEventCallback);
		// Send hello message and transmit max. duty / on time to JS
		String s;
		s += "{\"hello\": \"This is Tesla1 ws_server!\", \"MAX_DUTY_PERCENT\": ";
		s += MAX_DUTY_PERCENT;
		s += ", \"MAX_T_ON\": ";
		s += MAX_T_ON;
		s += ", \"BITS_PER_SEC\": ";
		s += BITS_PER_SEC;
		s += "}";
		ws_client.send(s);
	}
	if (ws_client.available())
		ws_client.poll();
}


// -----------------
//  http server stuff
// -----------------
static String getContentType(String filename){
	if(filename.endsWith(".htm")) return "text/html";
	else if(filename.endsWith(".html")) return "text/html";
	else if(filename.endsWith(".css")) return "text/css";
	else if(filename.endsWith(".js")) return "application/javascript";
	else if(filename.endsWith(".png")) return "image/png";
	else if(filename.endsWith(".gif")) return "image/gif";
	else if(filename.endsWith(".jpg")) return "image/jpeg";
	else if(filename.endsWith(".ico")) return "image/x-icon";
	else if(filename.endsWith(".xml")) return "text/xml";
	else if(filename.endsWith(".pdf")) return "application/x-pdf";
	else if(filename.endsWith(".zip")) return "application/x-zip";
	else if(filename.endsWith(".gz")) return "application/x-gzip";
	return "text/plain";
}

static void handle_http(void)
{
	String path = http_server.uri();
	Serial.println("handleFileRead: " + path);
	if(path.endsWith("/")) path += "index.htm";
	String contentType = getContentType(path);
	String pathWithGz = path + ".gz";
	if(LittleFS.exists(pathWithGz) || LittleFS.exists(path)){
		if(LittleFS.exists(pathWithGz))
			path += ".gz";
		File file = LittleFS.open(path, "r");
		http_server.streamFile(file, contentType);
		file.close();
		return;
	}
	http_server.send(404, "text/plain", "File not found!");
}

void refresh_http(void)
{
	http_server.handleClient();
}

void init_comms(void)
{
	Serial.printf("\nThis is %s, ", HOST_NAME);
	LittleFS.begin();
	WiFi.hostname(HOST_NAME);
	WiFi.setSleepMode(WIFI_NONE_SLEEP);

	if (strlen(WIFI_SSID) > 0) {
		Serial.printf("connecting to %s \n", WIFI_SSID);
		WiFi.begin(WIFI_SSID, WIFI_PW);
		for (int i=0; i<=500; i++) {
			if (WiFi.status() == WL_CONNECTED) {
				Serial.print("\nConnected!");
				break;
			}
			delay(100);
			Serial.print(".");
		}
	}

	if (WiFi.status() == WL_CONNECTED) {
		Serial.print("Local IP: ");
		Serial.println(WiFi.localIP());
	} else {
		WiFi.softAP(HOST_NAME);
		Serial.print("\nStarting AP mode: ");
		Serial.println(WiFi.softAPIP());
		WiFi.setOutputPower(20.5);  // set highest WiFi power
	}

	http_server.onNotFound(handle_http);
	http_server.begin();

	srand(RANDOM_REG32);

	ws_server.listen(8080);
	Serial.printf("Websocket at 8080 online: %d\n", ws_server.available());
}

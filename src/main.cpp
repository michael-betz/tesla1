#include <Arduino.h>
#include "Ticker.h"
#include "comms.h"
#include "pulser.h"
#include "musical.h"
#include "ESP8266mDNS.h"
#include "main.h"

// Using Ticker gives the pulser a higher priority over web-server stuff
// without it, opening the website causes a buffer under-run
Ticker ticker;

void setup() {
	pinMode(PIN_FIRE, OUTPUT);
	digitalWrite(PIN_FIRE, IS_ACTIVE_LOW);

	Serial.begin(115200);

	init_comms();

	init_musical();

	init_pulser();
	ticker.attach_ms(5, refresh_pulser);

	all_off();
}

void loop() {
	static unsigned cycle = 0;

	// refresh_pulser();

	refresh_ws(cycle);
	refresh_http();

	refresh_musical();
	MDNS.update();

	cycle++;
}

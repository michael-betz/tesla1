#include <Arduino.h>
#include "comms.h"
#include "pulser.h"
#include "main.h"

void setup() {
	digitalWrite(PIN_FIRE, 0);
	pinMode(PIN_FIRE, OUTPUT);

	Serial.begin(115200);

	init_comms();
	init_pulser();
}

void loop() {
	static unsigned cycle = 0;

	refresh_pulser();
	refresh_ws(cycle);
	refresh_http();

	delay(20);
	cycle++;
}

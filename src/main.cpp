#include <Arduino.h>
#include "comms.h"
#include "pulser.h"
#include "main.h"

void setup() {
	pinMode(PIN_FIRE, OUTPUT);
	digitalWrite(PIN_FIRE, IS_ACTIVE_LOW);

	Serial.begin(115200);

	pinMode(PIN_FIRE, OUTPUT);
	digitalWrite(PIN_FIRE, IS_ACTIVE_LOW);

	init_comms();

	refresh_pulser();
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

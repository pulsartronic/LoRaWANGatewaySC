#include <SystemClock.h>
SystemClock* sysclock = NULL; // make shure it tics at least once in a loop

#include "LoRaWanGateway.h"
LoRaWanGateway* loRaWanGateway = NULL;

void setup() {
	Serial.begin(9600);
	while (!Serial);
	sysclock = new SystemClock();
	loRaWanGateway = new LoRaWanGateway();
}

void loop() {
	loRaWanGateway->loop();
}

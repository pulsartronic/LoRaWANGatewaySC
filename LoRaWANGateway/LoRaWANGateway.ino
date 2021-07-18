/*
 * Pulsartronic
 * An Open Source project is a forever Work In Progress, feel free to contribute
 *
 */

#include "LoRaWANGateway.h"
LoRaWANGateway* loRaWANGateway = NULL;

void setup() {
	loRaWANGateway = new LoRaWANGateway();
	loRaWANGateway->setup();
}

void loop() {
	loRaWANGateway->loop();
}

#include "LoRaWanGateway.h"
LoRaWanGateway* loRaWanGateway = NULL;

void setup() {
	loRaWanGateway = new LoRaWanGateway();
	loRaWanGateway->setup();
}

void loop() {
	loRaWanGateway->loop();
}

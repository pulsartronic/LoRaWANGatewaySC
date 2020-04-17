#include "LoRaWanGateway.h"

LoRaWanGateway::LoRaWanGateway() : NetworkNode(NULL, "root") {
	if (SPIFFS.begin()) {
		DEBUG.println("SPIFFS init success");
	} else {
		DEBUG.println("SPIFFS init ERROR !!");
	}

	this->system = new System(this, "system");
	this->nodes->set(this->system->name, this->system);

	this->wifi = new WIFI(this, "wifi");
	this->nodes->set(this->wifi->name, this->wifi);

	this->rfm = new RFM(this, "rfm");
	this->nodes->set(this->rfm->name, this->rfm);

	this->wan = new WAN(this, "wan");
	this->nodes->set(this->wan->name, this->wan);
	this->wan->rfm = this->rfm;
}

LoRaWanGateway::~LoRaWanGateway() {
	delete this->system;
	delete this->wifi;
	delete this->rfm;
	delete this->wan;
}

void LoRaWanGateway::setup() {
	NetworkNode::setup();

	this->system->setup();
	this->wifi->setup();
	this->rfm->setup();
	this->wan->setup();

	DEBUG.println("Starting LoRaWAN Gateway ... OK");
}

void LoRaWanGateway::loop() {
	NetworkNode::loop();

	this->system->loop();
	this->wifi->loop();
	this->rfm->loop();
	this->wan->loop();

	uint64_t now = clock64.mstime();
	uint32_t diff = (uint32_t)(now - this->lping);
	if (diff > this->iping) {
		this->lping = now;
		int connectedClients = this->webSocketServer->connectedClients();
		if (connectedClients) {
			DynamicJsonDocument pongDocument(512);
			JsonObject pongObject = pongDocument.to<JsonObject>();
			this->ping(pongObject);
			this->command(pongObject);
		}
	} 
}


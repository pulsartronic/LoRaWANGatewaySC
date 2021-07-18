#ifndef __LoRaWANGateway__
#define __LoRaWANGateway__

#define ARDUINOJSON_USE_DOUBLE 1
#include <ArduinoJson.h>
#include <NetworkNode.h>
#include <SystemClock.h>
#include <WIFI.h>
#include <System.h>
#include <WAN.h>
#include <RFM.h>

#include "index.html.h"

class LoRaWANGateway : public NetworkNode {
	public:
	WIFI* wifi = NULL;
	System* system = NULL;
	WAN* wan = NULL;
	RFM* rfm = NULL;

	LoRaWANGateway();
	virtual ~LoRaWANGateway();
	void loop();
	void setup();
	virtual void html();
};

#endif

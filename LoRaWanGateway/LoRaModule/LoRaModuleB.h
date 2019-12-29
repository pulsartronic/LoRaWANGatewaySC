#ifndef __LoRaModuleB__
#define __LoRaModuleB__

#include <Logger.h>

// Set the structure for spreading factor
enum sf_t { SF6=6, SF7, SF8, SF9, SF10, SF11, SF12 };

class LoRaWAN;

class LoRaModuleB {
	public:
	Logger* logger = NULL;
	LoRaWAN* loRaWAN;

	String filename = "lora.json";

	// Channel Activity Detection
	// This function will scan for valid LoRa headers and determine the Spreading 
	// factor accordingly. If set to 1 we will use this function which means the 
	// 1-channel gateway will become even more versatile. If set to 0 we will use the
	// continuous listen mode.
	// Using this function means that we HAVE to use more dio pins on the RFM95/sx1276
	// device and also connect enable dio1 to detect this state. 
	bool cad = false;
	bool hop = false;
	uint8_t ch = (uint8_t) 1;
	// Spreading factor (SF7 - SF12)
	sf_t sf = SF7;

	LoRaModuleB(Logger* logger) {
		this->logger = logger;
	}
};

#endif

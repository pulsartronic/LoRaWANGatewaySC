#ifndef __LoRaModuleB_Methods__
#define __LoRaModuleB_Methods__

void Log(LoRaModuleB* loRaModule, String text) {
	if (NULL != loRaModule->logger) {
		String log = "{\"n\":\"lora\",\"p\":" + text + "}";
		loRaModule->logger->log(log);
	}
}

void fromJSON(LoRaModuleB* loRaModule, JsonObject lora) {
	if (lora.containsKey("cad")) loRaModule->cad = (bool) (int) lora["cad"];
	if (lora.containsKey("hop")) loRaModule->hop = (bool) (int) lora["hop"];
	if (lora.containsKey("sf")) loRaModule->sf = (sf_t) (int) lora["sf"];
	if (lora.containsKey("ch")) loRaModule->ch = (int) lora["ch"];
	if (lora.containsKey("pl")) loRaModule->ch = (int) lora["pl"];
}

DynamicJsonDocument JSON(LoRaModuleB* loRaModule) {
	DynamicJsonDocument doc(512);
	JsonObject lora = doc.to<JsonObject>();
	lora["cad"] = (int) loRaModule->cad;
	lora["hop"] = (int) loRaModule->hop;
	lora["sf"] = (int) loRaModule->sf;
	lora["ch"] = (int) loRaModule->ch;
	lora["pl"] = (int) loRaModule->pl;
	return doc;
}

void saveFile(LoRaModuleB* loRaModule) {
	DynamicJsonDocument doc = JSON(loRaModule);
	String json = "";
	serializeJson(doc, json);
	File file = SPIFFS.open(loRaModule->filename, "w");
	file.println(json);
	file.close();
}

void readFile(LoRaModuleB* loRaModule) {
	if (SPIFFS.exists(loRaModule->filename)) {
		File file = SPIFFS.open(loRaModule->filename, "r");
		String json = file.readStringUntil('\n');
		file.close();
		DynamicJsonDocument doc(512);
		DeserializationError error = deserializeJson(doc, json);
		if (!error) {
			JsonObject lora = doc.as<JsonObject>();
			fromJSON(loRaModule, lora);
		}
	}
}

void setup(LoRaModuleB* loRaModule) {
	Serial.println(F("Starting LoRaModule system ..."));

	// Pins are defined and set in loraModem.h
	pinMode(pins.ss, OUTPUT);
	pinMode(pins.rst, OUTPUT);
	pinMode(pins.dio0, INPUT);								// This pin is interrupt
	pinMode(pins.dio1, INPUT);								// This pin is interrupt
	//pinMode(pins.dio2, INPUT);

	// Init the SPI pins
	#if ESP32_ARCH==1
		SPI.begin(SCK, MISO, MOSI, SS);
	#else
		SPI.begin();
	#endif

	delay(500);
	
	if (SPIFFS.exists(loRaModule->filename)) {
		readFile(loRaModule);
	}

	// Setup ad initialise LoRa state machine of _loramModem.ino
	_state = S_INIT;
	initLoraModem(loRaModule);
	
	if (loRaModule->cad) {
		_state = S_SCAN;
		loRaModule->sf = SF7;
		cadScanner(loRaModule); // Always start at SF7
	} else {
		_state = S_RX;
		rxLoraModem(loRaModule);
	}
	LoraUp.payLoad[0]= 0;
	LoraUp.payLength = 0; // Init the length to 0

	// init interrupt handlers, which are shared for GPIO15 / D8, 
	// we switch on HIGH interrupts
	if (pins.dio0 == pins.dio1) {
		//SPI.usingInterrupt(digitalPinToInterrupt(pins.dio0));
		attachInterrupt(pins.dio0, Interrupt_0, RISING);		// Share interrupts
	} else { // Or in the traditional Comresult case
		//SPI.usingInterrupt(digitalPinToInterrupt(pins.dio0));
		//SPI.usingInterrupt(digitalPinToInterrupt(pins.dio1));
		attachInterrupt(pins.dio0, Interrupt_0, RISING);	// Separate interrupts
		attachInterrupt(pins.dio1, Interrupt_1, RISING);	// Separate interrupts		
	}
}

void reinit(LoRaModuleB* loRaModule) {
	String msg = "REINIT: CAD=" + String(loRaModule->cad) + " CH=" + String(loRaModule->ch) + " HOP=" + String(loRaModule->hop) + " SF=" + String(loRaModule->sf);
	String log = "{\"n\":\"log\",\"p\":\"" + msg + "\"}";
	Log(loRaModule, log);

	initLoraModem(loRaModule); // XXX 180326, after adapting this function 
	if ((loRaModule->cad) || (loRaModule->hop)) {
		_state = S_SCAN;
		loRaModule->sf = SF7;
		cadScanner(loRaModule);
	} else {
		_state = S_RX;
		rxLoraModem(loRaModule);
	}
	writeRegister(REG_IRQ_FLAGS_MASK, (uint8_t) 0x00);
	writeRegister(REG_IRQ_FLAGS, (uint8_t) 0xFF);			// Reset all interrupt flags
}

void Loop(LoRaModuleB* loRaModule) {
	// check for event value, which means that an interrupt has arrived.
	// In this case we handle the interrupt ( e.g. message received)
	// in userspace in loop().
	stateMachine(loRaModule); // do the state machine

	// After a quiet period, make sure we reinit the modem and state machine.
	// The interval is in seconds (about 15 seconds) as this re-init
	// is a heavy operation. 
	// SO it will kick in if there are not many messages for the gateway.
	// Note: Be careful that it does not happen too often in normal operation.
	uint32_t nowSeconds = now();
	if ( ((nowSeconds - statr[0].tmst) > _MSG_INTERVAL ) && (msgTime <= statr[0].tmst) ) {
		reinit(loRaModule);
		msgTime = nowSeconds;
	}
}

#endif

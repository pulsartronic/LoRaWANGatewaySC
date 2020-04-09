#include <RFM.h>

RFM::RFM(Node* parent, const char* name) : Node(parent, name) {

}

RFM::~RFM() {

}

void RFM::setup() {
	DEBUG.print("Strating LoRa module : ");

	this->readFile();

	// This call is optional and only needs to be used if you need to
	// change the default SPI interface used
	// LoRa.setSPI(spi);

	int nss = System::Pins::VALUE[this->pins.nss];
	int rst = System::Pins::VALUE[this->pins.rst];
	int dio0 = System::Pins::VALUE[this->pins.dio[0]];
	LoRa.setPins(nss, rst, dio0); // ss-reset-dio0
	this->active = LoRa.begin(settings.freq.curr);
	LoRa.disableInvertIQ(); // normal mode // TODO:: configure from GUI

	if (this->active) {
		LoRa.setTxPower(settings.txpw);
		LoRa.setSpreadingFactor(settings.sfac);
		LoRa.setSignalBandwidth(settings.sbw);
		LoRa.setCodingRate4(settings.crat);
		LoRa.setPreambleLength(settings.plength);
		LoRa.setSyncWord(settings.sw);
	}

	DEBUG.println(this->active ? "OK" : "FAILED");
}

void RFM::loop() {
	// schedule queues ??
}

void RFM::apply(RFM::Settings* settings) {
	LoRa.setFrequency(settings->freq.curr);
	LoRa.setTxPower(settings->txpw);
	LoRa.setSpreadingFactor(settings->sfac);
	LoRa.setSignalBandwidth(settings->sbw);
	LoRa.setCodingRate4(settings->crat);
	LoRa.setPreambleLength(settings->plength);
	LoRa.setSyncWord(settings->sw);

	if (settings->crc) {
		LoRa.enableCrc();
	} else {
		LoRa.disableCrc();
	}

	if (settings->iiq) {
		LoRa.enableInvertIQ();
	} else {
		LoRa.disableInvertIQ();
	}

	yield();
}

int RFM::send(Data::Packet* packet) {
	int sent = 0;
	if (this->active) {
		// TODO:: make more checks against transmission
		sent = LoRa.beginPacket();
		if (sent) {
			LoRa.write(packet->buffer, packet->size);
			yield();
			// TODO:: should we use async ending ?? !!!potential dead-lock!!!
			// if the RFM is not working properly, here it locks
			sent = LoRa.endPacket();
			if (sent) {
				String logMessage = "TX: freq:" + String(this->settings.freq.curr);	
				logMessage += ", sf:" + String(this->settings.sfac) + ", rssi:" + String(LoRa.packetRssi()) + ", dev:";
				logMessage += ((packet->buffer[4] < 0x10) ? "0" : "") + String(packet->buffer[4], HEX);
				logMessage += ((packet->buffer[3] < 0x10) ? "0" : "") + String(packet->buffer[3], HEX);
				logMessage += ((packet->buffer[2] < 0x10) ? "0" : "") + String(packet->buffer[2], HEX);
				logMessage += ((packet->buffer[1] < 0x10) ? "0" : "") + String(packet->buffer[1], HEX);
				logMessage += ", len:" + String(packet->size);
				this->log(logMessage);
			} else {
				String logMessage = "ERROR: RFM failed to transmit " + String(packet->size) + " bytes !";
				this->log(logMessage);
				// TODO:: save it somewhere 
			}
		} else {
			String logMessage = "ERROR: RFM is not ready for transmission !";
			this->log(logMessage);
			// TODO:: save it somewhere 
		}
	}
	return sent;
}

void RFM::read(RFM::Handler* handler) {
	int size = LoRa.parsePacket();
	if (size) {
		Data::Packet* packet = new Data::Packet(size);
		int available = LoRa.available();
		uint16_t i = 0;
		while (available && (i < size)) {
			packet->buffer[i++] = LoRa.read();
			available = LoRa.available();
		}
		handler->onRFMPacket(packet);

		String logMessage = "RX: freq:" + String(this->settings.freq.curr);	
		logMessage += ", sf:" + String(this->settings.sfac) + ", rssi:" + String(LoRa.packetRssi()) + ", dev:";
		logMessage += ((packet->buffer[4] < 0x10) ? "0" : "") + String(packet->buffer[4], HEX);
		logMessage += ((packet->buffer[3] < 0x10) ? "0" : "") + String(packet->buffer[3], HEX);
		logMessage += ((packet->buffer[2] < 0x10) ? "0" : "") + String(packet->buffer[2], HEX);
		logMessage += ((packet->buffer[1] < 0x10) ? "0" : "") + String(packet->buffer[1], HEX);
		logMessage += ", len:" + String(packet->size);
		this->log(logMessage);

		delete packet;
	}
}

void RFM::getState(JsonObject& rfm) {
	this->JSON(rfm);
	rfm["status"] = this->active; // TODO:: differentiate between 'idle' and 'failed'
}

void RFM::JSON(JsonObject& rfm) {
	rfm.createNestedObject("freq");
	rfm["freq"]["curr"] = this->settings.freq.curr;
	rfm["freq"]["min"] = this->settings.freq.min;
	rfm["freq"]["max"] = this->settings.freq.max;
	rfm.createNestedObject("pins");
	rfm["pins"]["miso"] = this->pins.miso;
	rfm["pins"]["mosi"] = this->pins.mosi;
	rfm["pins"]["sck"] = this->pins.sck;
	rfm["pins"]["nss"] = this->pins.nss;
	rfm["pins"]["rst"] = this->pins.rst;
	rfm["pins"].createNestedArray("dio");
	for (int i = 0; i <= 5; i++)
		rfm["pins"]["dio"].add(this->pins.dio[i]);
	rfm["sfac"] = this->settings.sfac;
	rfm["sbw"] = this->settings.sbw;
	rfm["crat"] = this->settings.crat;
	rfm["plength"] = this->settings.plength;
	rfm["sw"] = this->settings.sw;
	rfm["cad"] = this->settings.cad;
	rfm["txpw"] = this->settings.txpw;
}

void RFM::fromJSON(JsonObject& params) {
	if (params.containsKey("freq")) {
		JsonObject freq = params["freq"];
		if (freq.containsKey("curr")) { this->settings.freq.curr = freq["curr"]; }
		if (freq.containsKey("min"))  { this->settings.freq.min  = freq["min"];  }
		if (freq.containsKey("max"))  { this->settings.freq.max  = freq["max"];  }
	}

	if (params.containsKey("pins")) {
		JsonObject pins = params["pins"];
		if (pins.containsKey("miso")) { this->pins.miso = pins["miso"]; }
		if (pins.containsKey("mosi")) { this->pins.mosi = pins["mosi"]; }
		if (pins.containsKey("sck"))  { this->pins.sck  = pins["sck"];  }
		if (pins.containsKey("nss"))  { this->pins.nss  = pins["nss"];  }
		if (pins.containsKey("rst"))  { this->pins.rst  = pins["rst"];  }

		if (pins.containsKey("dio")) {
			JsonArray dio = pins["dio"];
			for (int i = 0; i <= 5; i++) {
				this->pins.dio[i] = dio[i];
			}
		}
	}

	if (params.containsKey("sfac"))    { this->settings.sfac    = params["sfac"];    }
	if (params.containsKey("sbw"))     { this->settings.sbw     = params["sbw"];     }
	if (params.containsKey("crat"))    { this->settings.crat    = params["crat"];    }
	if (params.containsKey("plength")) { this->settings.plength = params["plength"]; }
	if (params.containsKey("sw"))      { this->settings.sw      = params["sw"];      }
	if (params.containsKey("cad"))     { this->settings.cad     = params["cad"];     }
	if (params.containsKey("txpw"))    { this->settings.txpw    = params["txpw"];    }
}

void RFM::save(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	this->fromJSON(params);
	this->saveFile();
	this->setup();
	yield();
	JsonObject object = this->rootIT(broadcast);
	JsonObject mparams = object.createNestedObject("state");
	this->getState(mparams);
}

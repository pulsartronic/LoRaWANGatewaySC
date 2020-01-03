#ifndef __LoRaModuleConfigurable__
#define __LoRaModuleConfigurable__

class LoRaModuleConfigurable : public Configurable {
	public:
	String name = "lora";
	LoRaModuleB* loRaModule = NULL;

	virtual ~LoRaModuleConfigurable() {
		
	}

	LoRaModuleConfigurable(LoRaModuleB* loRaModule) {
		this->loRaModule = loRaModule;
	}

	void loop() {
		
	}

	String getName() {
		return this->name;
	}

	String hrdwr() {
		String hrdwr = "{}";
		return hrdwr;
	}

	String ping() {
		String lora = "{";
		lora += "\"cad\":" + String(this->loRaModule->cad);
		lora += ",\"pl\":" + String(this->loRaModule->pl);
		lora += ",\"ch\":" + String(this->loRaModule->ch);
		lora += ",\"hop\":" + String(this->loRaModule->hop);
		lora += ",\"sf\":" + String(this->loRaModule->sf);
		lora += "}";
		return lora;
	}

	String command(JsonObject command) {
		String name = command["n"];
		JsonObject params = command["p"];
		String response = "{\"error\":2}";
		if (String(F("save")).equals(name)) {
			response = this->save(params);
		}
		return response;
	}

	String save(JsonObject params) {
		fromJSON(this->loRaModule, params);
		// Serial.println("SAViNG: CAD:" + String(this->loRaModule->cad) + " CH:" + String(this->loRaModule->ch) + " HOP:" + String(this->loRaModule->hop) + " SF:" + String(this->loRaModule->sf));
		saveFile(this->loRaModule);
		reinit(this->loRaModule);
		String save = this->ping();
		return save;
	}
};

#endif


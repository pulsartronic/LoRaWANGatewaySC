#ifndef __LoRaWANConfigurable__
#define __LoRaWANConfigurable__

class LoRaWANConfigurable : public Configurable {
	public:
	String name = "wan";
	LoRaWAN* loRaWAN = NULL;

	virtual ~LoRaWANConfigurable() {

	}

	LoRaWANConfigurable(LoRaWAN* loRaWAN) {
		this->loRaWAN = loRaWAN;
	}

	void loop() {

	}

	String getName() {
		return this->name;
	}

	String hrdwr() {
		String hrdwr = Hrdwr(this->loRaWAN);
		return hrdwr;
	}

	String ping() {
		String lorawan = "{}";
		return lorawan;
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
		fromJSON(this->loRaWAN, params);
		saveFile(this->loRaWAN);

		if (params.containsKey("server")) {
			updateTTNAddress(this->loRaWAN);
		}

		bool ntp = params.containsKey("ntp");
		if (ntp) {
			updateNTPAddress(this->loRaWAN);
		}

		bool tzones = params.containsKey("tzones");
		if (ntp || tzones) {
			updateTime(this->loRaWAN);
		}

		String response = this->hrdwr();
		return response;
	}
};

#endif


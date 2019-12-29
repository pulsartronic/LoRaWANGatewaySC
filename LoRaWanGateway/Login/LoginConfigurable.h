#ifndef __LoginConfigurable__
#define __LoginConfigurable__

class LoginConfigurable : public Configurable {
	public:
	String name = "login";
	Login* login = NULL;

	virtual ~LoginConfigurable() {
		
	}

	LoginConfigurable(Login* login) {
		this->login = login;
	}

	void loop() {
		
	}

	String getName() {
		return this->name;
	}

	String ping() {
		String pong = "{}";
		return pong;
	}

	String hrdwr() {
		String hrdwr = "{}";
		return hrdwr;
	}

	String command(JsonObject command) {
		String name = command["n"];
		JsonObject params = command["p"];

		String response = "{\"data\":\"\"}";

		if (String(F("signin")).equals(name)) {
			response = this->signin(params);
		} else if (String(F("ping")).equals(name)) {
			response = this->pingall(params);
		} else if (String(F("change")).equals(name)) {
			response = this->change(params);
		}

		return response;
	}

	String signin(JsonObject params) {
		String response = "{";
		unsigned int length = this->login->deviceSet->length();
		for (unsigned int i = 0u; i < length; i++) {
			Configurable* configurable = this->login->deviceSet->configurable(i);
			String name = configurable->getName();
			String chrdwr = configurable->hrdwr();
			if (0u < i) response += ",";
			response += "\"" + name + "\":" + chrdwr;
		}
		response += "}";
		return response;
	}

	String pingall(JsonObject params) {
		String response = "{";
		unsigned int length = this->login->deviceSet->length();
		for (unsigned int i = 0u; i < length; i++) {
			Configurable* configurable = this->login->deviceSet->configurable(i);
			String name = configurable->getName();
			String cpong = configurable->ping();
			if (0u < i) response += ",";
			response += "\"" + name + "\":" + cpong;
		}
		response += "}";
		return response;
	}

	String change(JsonObject params) {
		this->login->fromJSON(params);
		delete this->login->aesm;
		this->login->aesm = new AESM(this->login->key);
		this->login->saveFile();
		String response = this->signin(params);
		return response;
	}
};

#endif


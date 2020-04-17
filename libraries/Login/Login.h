#include <DebugM.h>

#include <SystemClock.h>
#include <FS.h>
#include <ESP8266WebServer.h>

#include <AESM.h>
#include <Node.h>

#ifndef __Login__
#define __Login__

class Login : public Node {
	public:
	AESM* aesm = NULL;
	uint32_t bid = 0ul;
	uint32_t lid = 0ul;

	byte* key = NULL;

	Login(Node* parent, const char* name) : Node(parent, name) {
		Method* login = new Method(std::bind(&Login::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		this->methods->set("login", login);

		Method* change = new Method(std::bind(&Login::change, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		this->methods->set("change", change);
	}

	~Login() {
		delete this->aesm;
		delete[] this->key;
	}

	void setup() {
		this->bid = RANDOM_REG32;
		this->lid = millis();

		byte defaultKey[] = { // SHA256("adminadmin") "userpass"
			0xd8, 0x24, 0x94, 0xf0, 0x5d, 0x69, 0x17, 0xba, 
			0x02, 0xf7, 0xaa, 0xa2, 0x96, 0x89, 0xcc, 0xb4,
			0x44, 0xbb, 0x73, 0xf2, 0x03, 0x80, 0x87, 0x6c,
			0xb0, 0x5d, 0x1f, 0x37, 0x53, 0x7b, 0x78, 0x92};
		this->key = new byte[32];
		for (int i = 0; i < 32; i++) this->key[i] = defaultKey[i];
		this->readFile();
		this->aesm = new AESM((byte*) key);
		DEBUG.println("Starting Login system ... OK");
	}

	void loop() {

	}

	void LID() {
		uint32_t m = millis();
		if (this->lid > m) {
			this->bid = RANDOM_REG32;
			this->lid = m;
		}
	}

	void JLID(JsonObject& response) {
		response["b"] = this->bid;
		response["l"] = this->lid;
	}

	void session(JsonObject& response) {
		this->LID(); // don't forget to lid
		this->JLID(response);
	}

	void user(String& edata, JsonObject& response, JsonObject& broadcast) {
		this->LID(); // don't forget to lid
		String data = this->aesm->decrypt((byte*) edata.c_str());
		// DEBUG.println(data);

		DynamicJsonDocument requestDocument(1024);
		DeserializationError error = deserializeJson(requestDocument, data);
		if (!error) {
			JsonObject command = requestDocument.as<JsonObject>();
			JsonObject id = command["id"];
			uint32_t l = id["l"];
			uint32_t b = id["b"];
			if (this->bid == b && this->lid < l) {
				this->lid = l;
				JsonObject params = command["p"];

				DynamicJsonDocument responseDocument(1024);
				JsonObject dresponse = responseDocument.to<JsonObject>();

				this->parent->oncommand(params, dresponse, broadcast);

				String data = "";
				serializeJson(dresponse, data);
				//DEBUG.println("decrypted: " + data);
				String edata = this->aesm->encrypt((byte*) data.c_str(), data.length());
				response["data"] = edata;
			} else {
				DEBUG.println("this->bid == b && this->lid < l");
				this->LIDError(response);
			}
		} else {
			DEBUG.println("ERROR: DeserializationError");
			this->LIDError(response);
		}
	}

	void LIDError(JsonObject& response) {
		response["error"] = 1;
		JsonObject jlid = response.createNestedObject("jlid");
		this->JLID(jlid);
	}

	void fromJSON(JsonObject& params) {
		if (params.containsKey("key")) {
			String keySTR = params["key"];
			byte* base64 = (byte*) keySTR.c_str();
			unsigned int binary_length = Base64::decode_length(base64);
			if (32 == binary_length) {
				Base64::decode(base64, this->key);
			}
		}
	}

	virtual void JSON(JsonObject& login) {
		unsigned int b64Length = (4 * (32 + 2) / 3) + 2; // TODO:: what ?
		unsigned char base64[b64Length];
		for (int i = 0; i < b64Length; i++) base64[i] = '\0';
		unsigned int base64_length = Base64::encode(this->key, 32, base64);
		login["key"] = String((char*) base64);
	}

	void login(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
		response["error"] = false;
	}

	void change(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
		this->fromJSON(params);
		delete this->aesm;
		this->aesm = new AESM(this->key);
		this->saveFile();
		this->login(params, response, broadcast);
	}
};

#endif

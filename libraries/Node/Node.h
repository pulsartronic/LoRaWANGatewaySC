#include <Arduino.h>
#include <ArduinoJson.h>
#include <KeyValueMap.h>
//#include <functional>
#include <LittleFS.h>
#include <sntp.h>

#ifndef __Node__
#define __Node__

class Method {
	public:
	std::function<void(JsonObject&, JsonObject&, JsonObject&)> call;
	Method(std::function<void(JsonObject&, JsonObject&, JsonObject&)> call) : call(call) {}
};

class Node {
	public:
	Node* parent = NULL;
	String name = "";
	KeyValueMap<Node>* nodes = NULL;
	KeyValueMap<Method>* methods = NULL;

	Node(Node* parent, const char* name);
	virtual ~Node();

	String path() {
		String path = "";
		if (NULL != this->parent){		
			path = this->parent->path();
			path += this->name + "/";
		}
		return path;
	}

	String filename() {
		String path = this->path();
		String filename = path + this->name + ".json";
		return filename;
	}

	void saveFile() {
		DynamicJsonDocument jsonDocument = DynamicJsonDocument(1024);
		JsonObject json = jsonDocument.to<JsonObject>();
		this->JSON(json);
		String jsonSTR = "";
		serializeJson(jsonDocument, jsonSTR);
		String filename = this->filename();
		File file = LittleFS.open(filename, "w");
		file.println(jsonSTR);
		file.close();
	}

	void readFile() {
		String filename = this->filename();
		if (LittleFS.exists(filename)) {
			File file = LittleFS.open(filename, "r");
			String jsonSTR = file.readStringUntil('\n');
			file.close();
			DynamicJsonDocument jsonDocument(1024); // TODO:: unharcode
			DeserializationError error = deserializeJson(jsonDocument, jsonSTR);
			if (!error) {
				JsonObject loadedJSON = jsonDocument.as<JsonObject>();
				this->fromJSON(loadedJSON);
			} else {
				// debug ?
			}
		} else {
			// debug ?
		}
	}

	virtual void fromJSON(JsonObject& params) {
		
	}

	virtual void JSON(JsonObject& params) {

	}

	// TODO:: deprecated
	virtual void getState(JsonObject& state) {
		
	}

	virtual void applySettings() {
		
	}

	virtual void save(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
		this->fromJSON(params);
		this->saveFile();
		this->applySettings();
		yield();
		this->state(params, broadcast, broadcast);
	}

	// -->>
	virtual void oncommand(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
		String paramsSTR = "";
		serializeJson(params, paramsSTR);
		for (JsonPair kv : params) {
			const char* ckey = kv.key().c_str();
			JsonObject iparams = kv.value().as<JsonObject>();
			Node* implementer = this->nodes->get(ckey);
			Method* method = this->methods->get(ckey);
			if (NULL != implementer) {
				implementer->oncommand(iparams, response, broadcast);
			} else if (NULL != method) {
				method->call(iparams, response, broadcast);
			}
		}
	}

	virtual JsonObject rootIT(JsonObject& root) {
		JsonObject parentObject = this->parent->rootIT(root);
		bool existent = parentObject.containsKey(this->name);
		JsonObject createdObject;
		if (existent) {
			createdObject = parentObject[this->name].as<JsonObject>();
		} else {
			createdObject = parentObject.createNestedObject(this->name);
		}
		return createdObject;
	}

	// <<--
	virtual void command(JsonObject& command) {
		this->parent->command(command);
	}

	virtual void state(JsonObject& params, JsonObject& response, JsonObject& broadcast) {

	}

	virtual void log(String& text) {
		DynamicJsonDocument rootDocument(512);
		JsonObject command = rootDocument.to<JsonObject>();
		JsonObject object = this->rootIT(command);
		JsonObject log = object.createNestedObject("log");
		log["text"] = text;
		log["tstm"] = sntp_get_current_timestamp();
		this->command(command);
	}

	virtual void ping(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
		this->getPing(response);
		for (uint16_t i = 0; i < this->nodes->length; i++) {
			KeyValue<Node>* keyValue = this->nodes->keyValues[i];
			Node* node = keyValue->value;
			node->ping(params, response, broadcast);
		}
	}

	virtual void getPing(JsonObject& response) {

	}
};

#endif


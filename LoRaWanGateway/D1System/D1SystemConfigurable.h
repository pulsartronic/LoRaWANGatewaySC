#ifndef __D1SystemConfigurable__
#define __D1SystemConfigurable__

const char D1SystemConfigurableScripts[] PROGMEM = "<script></script>";
const char D1SystemConfigurableStyles[] PROGMEM = "<style></style>";

class D1SystemConfigurable : public Configurable {
	public:
	String name = "system";
	D1System* d1System = NULL;

	virtual ~D1SystemConfigurable() {
		
	}

	D1SystemConfigurable(D1System* d1System) {
		this->d1System = d1System;
	}

	void loop() {
		
	}

	String getName() {
		return this->name;
	}

	String hrdwr() {
/*
		//gets the size of the flash as set by the compiler
		uint32_t getFlashChipSize();
		uint32_t getFlashChipSpeed();
		FlashMode_t getFlashChipMode();
		uint32_t getFlashChipSizeByChipId();

		uint32_t getSketchSize();
		String getSketchMD5();
		uint32_t getFreeSketchSpace();
		bool updateSketch(Stream& in, uint32_t size, bool restartOnFail = false, bool restartOnSuccess = true);

		String getResetReason();
		String getResetInfo();
		struct rst_info * getResetInfoPtr();
*/

		uint32_t id = ESP.getChipId();
		String fullv = ESP.getFullVersion();
		uint8_t bootv = ESP.getBootVersion();
		uint8_t bootm = ESP.getBootMode();
		uint8_t cpuf = ESP.getCpuFreqMHz();
		uint32_t flid = ESP.getFlashChipId();
		uint8_t fvid = ESP.getFlashChipVendorId();

		uint32_t csize = ESP.getFlashChipRealSize();
		uint32_t fsize = ESP.getFlashChipSpeed();

		// FlashMode_t getFlashChipMode();
		// uint32_t getFlashChipSizeByChipId();

		uint32_t ssize = ESP.getSketchSize();
		String md5 = ESP.getSketchMD5();
		uint32_t sfree = ESP.getFreeSketchSpace();

		String hrdwr = "{";
		hrdwr += "\"version\":\"" + String(VERSION) + "\"";
		hrdwr += ",\"id\":\"" + String(id) + "\"";
		hrdwr += ",\"fullv\":\"" + fullv + "\"";
		hrdwr += ",\"bootv\":\"" + String(bootv) + "\"";
		hrdwr += ",\"bootm\":\"" + String(bootm) + "\"";
		hrdwr += ",\"cpuf\":\"" + String(cpuf) + "\"";
		hrdwr += ",\"flid\":\"" + String(flid) + "\"";
		hrdwr += ",\"fvid\":\"" + String(fvid) + "\"";
		hrdwr += ",\"csize\":\"" + String(csize) + "\"";
		hrdwr += ",\"fsize\":\"" + String(fsize) + "\"";
		hrdwr += ",\"ssize\":\"" + String(ssize) + "\"";
		hrdwr += ",\"md5\":\"" + md5 + "\"";
		hrdwr += ",\"sfree\":\"" + String(sfree) + "\"";
		hrdwr += "}";

		return hrdwr;
	}

	String ping() {
		uint32_t freeHeap = ESP.getFreeHeap();
		uint32_t heapFramentation = ESP.getHeapFragmentation();
		String system = "{";
		system += "\"heap\":" + String(freeHeap);
		system += ",\"heapf\":" + String(heapFramentation);
		system += ",\"up\":{\"high\":" + String(sysclock->high()) + ",\"low\":" + String(sysclock->low) + "}";
		system += "}";
		return system;
	}

	String command(JsonObject command) {
		String name = command["n"];
		JsonObject params = command["p"];
		String response = "{\"error\":2}";
		if (String(F("upgrade")).equals(name)) {
			response = this->upgrade(params);
		}
		return response;
	}

	String upgrade(JsonObject params) {
		String response = this->d1System->upgrade();
		return response;
	}
};

#endif


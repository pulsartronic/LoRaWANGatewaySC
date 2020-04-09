#include <System.h>

System::ESPS::ESPS(Node* parent, const char* name) : Node(parent, name) {

}

System::ESPS::~ESPS() {
	
}

void System::ESPS::setup() {

}

void System::ESPS::loop() {

}

void System::ESPS::getState(JsonObject& esps) {
	////gets the size of the flash as set by the compiler
	//uint32_t getFlashChipSize();
	//bool updateSketch(Stream& in, uint32_t size, bool restartOnFail = false, bool restartOnSuccess = true);
	//String getResetReason();
	//String getResetInfo();
	//struct rst_info * getResetInfoPtr();

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
	
	uint32_t freeHeap = ESP.getFreeHeap(); // For ping ??
	uint32_t heapFramentation = ESP.getHeapFragmentation(); // For ping ??

	esps["id"] = id;
	esps["fullv"] = fullv;
	esps["bootv"] = bootv;
	esps["bootm"] = bootm;
	esps["cpuf"] = cpuf;
	esps["flid"] = flid;
	esps["fvid"] = fvid;
	esps["csize"] = csize;
	esps["fsize"] = fsize;
	esps["ssize"] = ssize;
	esps["md5"] = md5;
	esps["sfree"] = String(sfree);
	esps["heap"] = freeHeap;
	esps["heapf"] = heapFramentation;
}

void System::ESPS::getPing(JsonObject& response) {
	JsonObject object = this->rootIT(response);
	JsonObject mparams = object.createNestedObject("state");

	uint32_t freeHeap = ESP.getFreeHeap();
	uint32_t heapFramentation = ESP.getHeapFragmentation();
	mparams["heap"] = freeHeap;
	mparams["heapf"] = heapFramentation;
}

String System::ESPS::upgrade() {
	String response = "{}";

	DEBUG.println("UPGRADING ::::::");
	String host = F("http://pulsartronic.com/firmware/lorawan/version.php");
	WiFiClient client;
	HTTPClient http;
	if (http.begin(client, host)) {
		int httpCode = http.GET();
		if (httpCode > 0) {
			if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
				String payload = http.getString();
				int last = payload.toInt();
				DEBUG.println("payload: " + payload);
				if (last > VERSION) {
					DEBUG.println("Last is greather than actual, UPGRADING !!!");
					WiFiClient client;
					ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
					ESPhttpUpdate.rebootOnUpdate(true);
					t_httpUpdate_return ret = ESPhttpUpdate.update(client, "http://pulsartronic.com/firmware/lorawan/d1_80mhz_4mb_1mbspifss_latest.bin");
					switch (ret) {
						case HTTP_UPDATE_FAILED:
							response = "{\"error\":9}";
							// DEBUG.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
						break;
						case HTTP_UPDATE_NO_UPDATES:
							response = "{\"error\":10}";
							DEBUG.println("HTTP_UPDATE_NO_UPDATES");
						break;
						case HTTP_UPDATE_OK:
							response = "{\"error\":false}";
							DEBUG.println("HTTP_UPDATE_OK");
						break;
						default:
							response = "{\"error\":15}";
						break;
					}
				} else {
					response = "{\"error\":14}";
				}
			} else {
				response = "{\"error\":16}";
			}
		} else {
			response = "{\"error\":13}";
			DEBUG.println(String(httpCode));
			//DEBUG.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
		}
		http.end();
	} else {
		response = "{\"error\":12}";
		//DEBUG.printf("[HTTP} Unable to connect\n");
	}

	return response;
}

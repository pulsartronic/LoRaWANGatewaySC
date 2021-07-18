#include <System.h>

System::ESPS::ESPS(Node* parent, const char* name) : Node(parent, name) {

}

System::ESPS::~ESPS() {
	
}

void System::ESPS::setup() {

}

void System::ESPS::loop() {

}

void System::ESPS::state(JsonObject& params, JsonObject& response, JsonObject& broadcast) {
	JsonObject object = this->rootIT(response);
	JsonObject esps = object.createNestedObject("state");
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


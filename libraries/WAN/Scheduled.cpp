#include <WAN.h>

WAN::Scheduled::Scheduled(RFData* rfData, uint32_t tmst) : rfData(rfData), tmst(tmst) {

}

WAN::Scheduled::~Scheduled() {
	delete this->rfData->packet;
	delete this->rfData;
}


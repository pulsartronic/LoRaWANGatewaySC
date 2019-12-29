#ifndef __SystemClock__
#define __SystemClock__

class SystemClock {
	public:
	uint32_t low = 0ul;
	uint64_t h = 0ull;

	virtual ~SystemClock() {
		
	}

	SystemClock() {
		//LUP.payLoad[6] = frameCount % 0x100; // LSB
		//LUP.payLoad[7] = frameCount / 0x100; // MSB
	}

	uint64_t mstime() {
		uint32_t now = millis();
		uint64_t multiplier = (uint64_t) (now < this->low);
		uint64_t adition = 0x100000000;
		uint64_t msnow = (uint64_t) now;
		this->h += adition * multiplier;
		this->low = now;
		return this->h + msnow;
	}

	uint32_t high() {
		uint64_t ms = mstime();
		uint32_t h = (uint32_t) (ms >> 4 * 8);
		return h;
	}
};

#endif

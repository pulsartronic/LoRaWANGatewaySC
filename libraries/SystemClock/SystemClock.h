#ifndef __SystemClock__
#define __SystemClock__

#include <Arduino.h>

class SystemClock {
	public:
	uint32_t low = 0ul;
	uint64_t h = 0ull;

	virtual ~SystemClock() {}
	SystemClock();
	uint64_t mstime();
	uint32_t high();
};

extern SystemClock clock64; // make shure it tics at least once in a loop

#endif

#ifndef __Configurable__
#define __Configurable__

#include <Configurable.h>
#include <ArduinoJson.h>

class Configurable {
	public:

	Configurable() {
		
	}

	virtual String getName() = 0;
	virtual String ping() = 0;
	virtual String hrdwr() = 0;
	virtual String command(JsonObject command) = 0;
};

#endif

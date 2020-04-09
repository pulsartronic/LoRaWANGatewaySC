#include <Arduino.h>

#ifndef __Debug__
#define __Debug__

class Debug {
	public:
	Debug();
	void print(char* text);
	void println(char* text);
	void print(const char* text);
	void println(const char* text);
	void print(String text);
	void println(String text);
};

extern Debug DEBUG;
#endif

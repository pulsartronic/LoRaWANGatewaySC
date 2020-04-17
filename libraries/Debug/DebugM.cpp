#include <DebugM.h>

Debug::Debug() {
	Serial.begin(9600l);
}

void Debug::print(char* text) {
	Serial.print(text);
}

void Debug::println(char* text) {
	Serial.println(text);
}

void Debug::print(const char* text) {
	Serial.print(text);
}

void Debug::println(const char* text) {
	Serial.println(text);
}

void Debug::print(String text) {
	Serial.print(text);
}

void Debug::println(String text) {
	Serial.println(text);
}

Debug DEBUG;

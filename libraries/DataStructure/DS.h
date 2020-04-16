#ifndef __DS__
#define __DS__

#include <Arduino.h>

namespace DS { // DS : Data Structure
	template<typename T> class List {
		public:
		T* buffer = new T[0];
		uint32_t length = 0u;
		virtual ~List() { delete[] this->buffer; }

		T get(uint32_t index){
			T element = this->buffer[index];
			return element;
		}

		void add(T element){
			uint32_t newLength = this->length + 1;
			T* newBuffer = new T[newLength];
			for (uint32_t i = 0ul; i < this->length; i++) {
				newBuffer[i] = this->buffer[i];
			}
			newBuffer[this->length] = element;
			delete[] this->buffer;
			this->length = newLength;
			this->buffer = newBuffer;
		}

		void removeAt(uint32_t index) {
			uint32_t newLength = this->length - 1;
			T* newBuffer = new T[newLength];
			for (uint32_t i = 0ul; i < this->length; i++) {
				if (i != index) {
					newBuffer[i] = this->buffer[i];
				}
			}
			delete[] this->buffer;
			this->length = newLength;
			this->buffer = newBuffer;
		}
	};
}
#endif

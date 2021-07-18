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
			if (0 <= index && index < this->length) {
				uint32_t newLength = this->length - 1;
				T* newBuffer = new T[newLength];
				uint16_t j = 0u;
				for (uint32_t i = 0ul; i < this->length; i++) {
					if (i != index) {
						newBuffer[j++] = this->buffer[i];
					}
				}
				delete[] this->buffer;
				this->length = newLength;
				this->buffer = newBuffer;
			}
		}
		
		int32_t indexOf(T element) {
			int32_t index = -1;
			for (int32_t i = 0ul; (i < this->length) && (0 > index); i++) {
				T saved = this->buffer[i];
				if (element == saved) {
					index = i;
				}
			}
			return index;
		}
	};
}
#endif

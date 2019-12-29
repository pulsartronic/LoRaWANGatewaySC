#ifndef __CDOS__
#define __CDOS__

class CDIterator;

class CDElement {
	public:
	static const uint8_t CD_TYPE_FLAGS  = (uint8_t) 192; // 11000000
	static const uint8_t CD_ARRAY_TYPE  = (uint8_t) 0;
	static const uint8_t CD_OBJECT_TYPE = (uint8_t) 64;
	static const uint8_t CD_STRING_TYPE = (uint8_t) 128;
	static const uint8_t CD_NUMBER_TYPE = (uint8_t) 192;
	virtual ~CDElement() { }
	CDElement() { }
	virtual unsigned int length() { return 0u; }
	virtual unsigned int size() { return 0u; }
	virtual void add(CDElement* element) {}
	virtual CDElement* get(unsigned int index) { return NULL; }
	virtual void set(unsigned int index, CDElement* element) { }
	virtual CDElement* get(char* name) { return NULL; } 
	virtual void let(char* name, CDElement* element) { }
	virtual unsigned int calculateLength() { return 0u; }
	virtual void serialize(CDIterator* iterator) { }

	virtual uint8_t uint8() { return (uint8_t) 0;}
	virtual int8_t int8() { return (int8_t) 0;}
	virtual int16_t int16() { return 0;}
	virtual uint16_t uint16() { return 0u;}
	virtual int32_t int32() { return 0l;}
	virtual uint32_t uint32() { return 0ul;}
	virtual int64_t int64() { return 0ll;}
	virtual uint64_t uint64() { return 0ull;}
};

class CDIterator {
	public:
	uint8_t* buffer = NULL;
	unsigned int size = 0u;

	CDIterator(uint8_t* buffer, unsigned int size) {
		this->buffer = buffer;
		this->size = size;
	}

	CDElement* nextElement();
	unsigned int parseNumber(unsigned int HM);
	bool valid();

	unsigned int available() {
		unsigned int available = (this->size >= this->index) ? (this->size - this->index) : 0u;
		return available;
	}

	uint8_t next() {
		unsigned int index = min(this->index, this->size - 1);
		this->index += 1u;
		return this->buffer[index];
	}

	void add(uint8_t value) {
		bool hasNext = this->index < this->size;
		if (hasNext) {
			this->buffer[this->index++] = value;
		} else {
			Serial.println("Error in serialization");
		}
	}

	private:
	unsigned int index = 0u;
	bool check();
};




class CDArray : public CDElement {
	public:
	CDElement** elements = new CDElement*[0];
	unsigned int size = 0u;

	virtual ~CDArray() {
		for (unsigned int i = 0u; i < this->size; i++) {
			CDElement* element = this->elements[i];
			delete element;
		}
		delete[] this->elements;
	}

	CDArray() {
		
	}

	CDArray(unsigned int HM, CDIterator* iterator) {
		unsigned int available = iterator->available();
		for (unsigned int i = 0u; (i < HM) && (0u < available); i++) {
			CDElement* nextElement = iterator->nextElement();
			this->add(nextElement);
			available = iterator->available();
		}
	}

	virtual unsigned int length() {
		return this->size;
	}

	CDElement* get(unsigned int index) {
		return this->elements[index];
	}

	virtual void set(unsigned int index, CDElement* element) {
		this->elements[index] = element;
	}

	virtual void add(CDElement* element) {
		CDElement** newElements = new CDElement*[this->size + 1u];
		for (unsigned int i = 0u; i < this->size; i++) {
			newElements[i] = this->elements[i];
		}
		newElements[this->size] = element;
		this->size += 1u;
		delete[] this->elements;
		this->elements = newElements;
	}

	virtual unsigned int calculateLength() {
		unsigned int blength = 2u; // ET + HM 
		for (unsigned int i = 0u; i < this->size; i++) {
			CDElement* element = this->elements[i];
			if (NULL != element) {
				blength += element->calculateLength();
			} else {
				blength += 1u;
			}
		}
		return blength;
	}

	virtual void serialize(CDIterator* iterator) {
		uint8_t ET = CDElement::CD_ARRAY_TYPE | (uint8_t) 1;
		iterator->add(ET);
		uint8_t HM = (uint8_t) this->size;
		iterator->add(HM);
		for (unsigned int i = 0u; i < this->size; i++) {
			CDElement* element = this->elements[i];
			if (NULL != element) {
				element->serialize(iterator);
			} else { // indicate NULL
				uint8_t ET = CDElement::CD_OBJECT_TYPE | (uint8_t) 0;
				iterator->add(ET);
			}
		}
	}
};




class CDString : public CDElement {
	public:
	char* content = new char[0];	
	unsigned int len = 0u;

	virtual ~CDString() {
		delete[] this->content;
	}

	CDString(unsigned int HM, CDIterator* iterator) {
		unsigned int available = iterator->available();
		if (HM <= available) {
			delete[] this->content;
			this->len = HM;
			this->content = new char[this->len + 1];
			for (unsigned int i = 0u; i < HM; i++) {
				this->content[i] = (char) iterator->next();
			}
			this->content[this->len] = '\0';
		}
	}

	CDString(char* content) {
		delete[] this->content;
		this->len = (unsigned int) strlen(content);
		this->content = new char[this->len + 1];
		memcpy(this->content, content, this->len + 1);
	}

	int cmp(char* other) {
		return strcmp(this->content, other);
	}

	virtual unsigned int calculateLength() {
		unsigned int blength = 2u + this->len;
		return blength;
	}

	virtual void serialize(CDIterator* iterator) {
		uint8_t ET = CDElement::CD_STRING_TYPE | (uint8_t) 1;
		iterator->add(ET);
		uint8_t HM = (uint8_t) this->len;
		iterator->add(HM);
		for (unsigned int i = 0u; i < this->len; i++) {
			uint8_t b = (uint8_t) this->content[i];
			iterator->add(b);
		}
	}
};



class CDKey : public CDString {
	public:
	CDElement* value = NULL;

	virtual ~CDKey() {
		delete this->value;
	}

	CDKey(char* name, CDElement* value) : CDString(name) {
		this->value = value;
	}

	CDKey(unsigned int HM, CDIterator* iterator) : CDString(HM, iterator) {
		this->value = iterator->nextElement();
	}

	virtual unsigned int calculateLength() {
		unsigned int blength = CDString::calculateLength();
		if (NULL != this->value) {
			blength += this->value->calculateLength();
		} else {
			blength += 1u; // to indicate NULL
		}

		return blength;
	}

	virtual void serialize(CDIterator* iterator) {
		CDString::serialize(iterator);
		if (NULL != this->value) {
			this->value->serialize(iterator);
		} else { // indicate NULL
			uint8_t ET = CDElement::CD_OBJECT_TYPE | (uint8_t) 0;
			iterator->add(ET);
		}
	}
};

class CDObject : public CDElement {
	public:
	CDKey** keys = new CDKey*[0];
	unsigned int ksize = 0u;

	virtual ~CDObject() {
		for (unsigned int i = 0u; i < this->ksize; i++) {
			delete this->keys[i];
		}
		delete[] this->keys;
	}

	CDObject() {
		
	}

	CDObject(unsigned int HM, CDIterator* iterator) {
		unsigned int available = iterator->available();
		for (unsigned int i = 0u; (i < HM) && (0u < available); i++) {
			uint8_t initial = iterator->next();
			uint8_t ET = initial & CDElement::CD_TYPE_FLAGS;
			if (CDElement::CD_STRING_TYPE == ET) {
				uint8_t HMuint8_ts = initial & ~CDElement::CD_TYPE_FLAGS;
				available = iterator->available();
				unsigned int maxHMuint8_ts = sizeof(unsigned int);			
				if (maxHMuint8_ts >= HMuint8_ts && HMuint8_ts <= available) {
					unsigned int HM = (unsigned int) iterator->parseNumber(HMuint8_ts);
					available = iterator->available();
					if (0u < HM && HM <= available) {
						// TODO:: check for existance before creating a new one
						CDKey* key = new CDKey(HM, iterator);
						CDKey** newKeys = new CDKey*[this->ksize + 1];
						// TODO:: maybe sort by strcmp to be able to make binary searches ?
						for (unsigned int j = 0u; j < this->ksize; j++) {
							newKeys[j] = this->keys[j];
						}
						delete[] this->keys;
						this->keys = newKeys;
						this->keys[this->ksize] = key;
						this->ksize += 1;
					}
				}
			}
		}
	}

	virtual unsigned int size() {
		return this->ksize;
	}

	CDKey* getKey(char* name) {
		CDKey* key = NULL;
		for (unsigned int i = 0u; (i < this->ksize) && (NULL == key); i++) {
			CDKey* skey = this->keys[i];
			if (NULL != skey) {
				if (0 == skey->cmp(name)) {
					key = skey;
				}
			}
		}
		return key;
	}

	virtual CDElement* get(char* name) {		
		CDElement* element = NULL;
		CDKey* skey = this->getKey(name);
		if (NULL != skey) {
			element = skey->value;
		}
		return element;
	}

	virtual void let(char* name, CDElement* element) {
		bool found = false;
		for (unsigned int i = 0u; (i < this->ksize) && !found; i++) {
			CDKey* skey = this->keys[i];
			if (NULL != skey) {
				found = (0 == skey->cmp(name));
				if (found) {
					CDElement* saved = skey->value;
					if (NULL != saved) {
						if (saved != element) {
							delete saved;
						}
					}
					skey->value = element;
				}
			}
		}

		if (!found) {
			CDKey** newKeys = new CDKey*[this->ksize + 1];
			for (unsigned int i = 0u; i < this->ksize; i++) {
				newKeys[i] = this->keys[i];
			}
			newKeys[this->ksize] = new CDKey(name, element);
			delete[] this->keys;
			this->keys = newKeys;
			this->ksize += 1u;
		}
	}

	virtual unsigned int calculateLength() {
		unsigned int blength = 2u; // ET + HM
		for (unsigned int i = 0u; i < this->ksize; i++) {
			CDKey* key = this->keys[i];
			if (NULL != key) { // it should be never NULL
				blength += key->calculateLength();
			}
		}
		return blength;
	}

	virtual void serialize(CDIterator* iterator) {
		uint8_t ET = CDElement::CD_OBJECT_TYPE | (uint8_t) 1;
		iterator->add(ET);
		uint8_t HM = (uint8_t) this->ksize;
		iterator->add(HM);
		for (unsigned int i = 0u; i < this->ksize; i++) {
			CDKey* key = this->keys[i];
			if (NULL != key) {
				key->serialize(iterator);
			} // TODO:: indicate null key ?? it should never happen
		}
	}
};





class CDNumber : public CDElement {
	public:
	uint8_t* uint8_ts = new uint8_t[0];
	unsigned int bsize = 0u;

	virtual ~CDNumber() {
		delete[] this->uint8_ts;
	}

	template<typename T> CDNumber(T value) {
		delete[] this->uint8_ts;
		this->bsize = (unsigned int) sizeof(T);
		this->uint8_ts = new uint8_t[this->bsize];
		for (unsigned int i = 0u; i < this->bsize; i++) {
			T dvalue = value >> (8u * (this->bsize - i - 1));
			this->uint8_ts[i] = (uint8_t) (dvalue & (T) 255);
		}
	}

	CDNumber(unsigned int HM, CDIterator* iterator) {
		unsigned int available = iterator->available();
		if (HM <= available) {
			delete[] this->uint8_ts;
			this->bsize = HM;
			this->uint8_ts = new uint8_t[this->bsize];
			for (unsigned int i = 0u; i < HM; i++) {
				this->uint8_ts[i] = iterator->next();
			}
		}
	}

	template<typename T> T value() {
		T ret = (T) 0;
		unsigned int typesize = sizeof(T);
		unsigned int total = min(typesize, this->bsize);
		for (unsigned int i = 0u; i < total; i++) {
			T v = (T) this->uint8_ts[this->bsize - 1 - i];
			ret = ret | (v << 8u * i);
		}
		return ret;
	}

	virtual int8_t int8() {
		return this->value<int8_t>();
	}

	virtual uint8_t uint8() {
		return this->value<uint8_t>();
	}

	virtual int16_t int16() {
		return this->value<int16_t>();
	}

	virtual uint16_t uint16() {
		return this->value<uint16_t>();
	}

	virtual int32_t int32() {
		return this->value<int32_t>();
	}

	virtual uint32_t uint32() {
		return this->value<uint32_t>();
	}

	virtual int64_t int64() {
		return this->value<int64_t>();
	}

	virtual uint64_t uint64() {
		return this->value<uint64_t>();
	}

	virtual unsigned int calculateLength() {
		unsigned int blength = 2u + this->bsize;
		return blength;
	}

	virtual void serialize(CDIterator* iterator) {
		uint8_t ET = CDElement::CD_NUMBER_TYPE | (uint8_t) 1;
		iterator->add(ET);
		// TODO:: HM should be uint8_t or unsigned int considering its value
		uint8_t HM = (uint8_t) this->bsize;
		iterator->add(HM);
		for (unsigned int i = 0u; i < this->bsize; i++) {
			uint8_t v = this->uint8_ts[i];
			iterator->add(v);
		}
	}
};



bool CDIterator::valid() {
	bool valid = this->check();
	valid = valid && (this->index == this->size);
	this->index = 0u;
	return valid;
}

bool CDIterator::check() {
	bool valid = true;
	unsigned int available = this->available();
	if (0u < available) {
		uint8_t initial = this->next();
		uint8_t ET = initial & CDElement::CD_TYPE_FLAGS;
		uint8_t HMuint8_ts = initial & ~CDElement::CD_TYPE_FLAGS;
		if (0u < HMuint8_ts) {
			available = this->available();
			// this implementation stores numbers as primitives, thus we have to check against
			// the best type available, considering the platform
			unsigned int maxHMuint8_ts = sizeof(unsigned int);
			if (maxHMuint8_ts >= HMuint8_ts && HMuint8_ts <= available) {
				unsigned int HM = (unsigned int) this->parseNumber(HMuint8_ts);
				available = this->available();
				if (HM <= available) {
					switch(ET) {
						case CDElement::CD_ARRAY_TYPE: {
							for (unsigned int i = 0; i < HM && valid; i++) {
								valid = valid && this->check();
							}
						} break;
						case CDElement::CD_OBJECT_TYPE:
							for (unsigned int i = 0; i < HM && valid; i++) {
								valid = valid && this->check(); // KEY
								valid = valid && this->check(); // VALUE
							}
							break;
						case CDElement::CD_STRING_TYPE:
						case CDElement::CD_NUMBER_TYPE: {
							this->index += HM;
						} break;
						default: {
							valid = false;
						} break;
					}
				} else {
					valid = false;
				}
			} else {
				valid = false;
			}
		}
	} else {
		valid = false;
	}
	return valid;
}

CDElement* CDIterator::nextElement() {
	CDElement* element = NULL;
	unsigned int available = this->available();
	if (0u < available) {
		uint8_t initial = this->next();
		uint8_t ET = initial & CDElement::CD_TYPE_FLAGS;
		uint8_t HMuint8_ts = initial & ~CDElement::CD_TYPE_FLAGS;
		if (0u < HMuint8_ts) {
			available = this->available();
			// this implementation stores numbers as primitives, thus we have to chack against
			// the best type available, considering the platform
			unsigned int maxHMuint8_ts = sizeof(unsigned int);
			if (maxHMuint8_ts >= HMuint8_ts && HMuint8_ts <= available) {
				unsigned int HM = (unsigned int) this->parseNumber(HMuint8_ts);
				switch(ET) {
					case CDElement::CD_ARRAY_TYPE: {
						element = new CDArray(HM, this);
					} break;
					case CDElement::CD_OBJECT_TYPE: {
						element = new CDObject(HM, this);
					} break;
					case CDElement::CD_STRING_TYPE: {
						element = new CDString(HM, this);
					} break;
					case CDElement::CD_NUMBER_TYPE: {
						element = new CDNumber(HM, this);
					} break;
				}
			}
		}
	}
	return element;
}

unsigned int CDIterator::parseNumber(unsigned int HM) {
	unsigned int number = 0ll;
	for (unsigned int i = 0; i < HM; i++) {
		uint8_t next = this->next();
		number = number << 8;
		number |= (unsigned int) next;
	}
	return number;
}

#endif

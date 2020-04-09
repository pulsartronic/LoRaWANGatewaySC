#ifndef __KeyValueMap__
#define __KeyValueMap__

template <class T> class KeyValue {
	public:
	char* key;
	T* value = NULL;

	virtual ~KeyValue() {
		delete[] this->key;
	}

	KeyValue(const char* key, T* value) {
		int length = strlen(key);
		this->key = new char[length + 1];
		memcpy(this->key, key, length + 1);
		this->value = value;
	}
};

template <class T> class KeyValueMap {
	public:
	uint16_t length = 0u;
	KeyValue<T>** keyValues = new KeyValue<T>*[0];

	virtual ~KeyValueMap() {
		for (uint16_t i = 0u; i < this->length; i++) {
			KeyValue<T>* keyValue = this->keyValues[i];
			delete keyValue;
		}
		delete[] this->keyValues;
	}

	KeyValueMap() {
		
	}

	KeyValue<T>* key(const char* key) {
		KeyValue<T>* keyValue = NULL;
		for (uint16_t i = 0u; (i < this->length) && (NULL == keyValue); i++) {
			KeyValue<T>* skeyValue = this->keyValues[i];
			if (NULL != skeyValue) {
				// TODO:: binary search
				if (0 == strcmp(key, skeyValue->key)) {
					keyValue = skeyValue;
				}
			}
		}
		return keyValue;
	}

	void set(String& key, T* value) {
		const char* ckey = key.c_str();
		this->set(ckey, value);
	}

	void set(const char* key, T* value) {
		KeyValue<T>* skeyValue = this->key(key);
		if (NULL != skeyValue) {
			skeyValue->value = value;
		} else {
			KeyValue<T>** newKeyValues = new KeyValue<T>*[this->length + 1];
			for (unsigned int i = 0u; i < this->length; i++) {
				newKeyValues[i] = this->keyValues[i];
			}
			newKeyValues[this->length] = new KeyValue<T>(key, value);
			delete[] this->keyValues;
			this->keyValues = newKeyValues;
			this->length += 1u;
		}
	}

	T* get(const char* key) {
		T* value = NULL;
		KeyValue<T>* skeyValue = this->key(key);
		if (NULL != skeyValue) {
			value = skeyValue->value;
		}
		return value;
	}

	void remove(char* key) {
		// TODO::
	}
};

#endif


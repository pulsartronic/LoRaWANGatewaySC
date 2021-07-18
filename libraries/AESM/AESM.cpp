#include <AESM.h>

AESM::AESM(byte* key) {
	memcpy(this->key, key, 32);
}

uint16_t AESM::calculateCipherLength(int plainLength) {
	unsigned int padedLength = plainLength + N_BLOCK - plainLength % N_BLOCK;
	unsigned int cipherLength = N_BLOCK + padedLength;
	return cipherLength;
}

uint16_t AESM::calculatePlainLength(int cipherLength) {
	unsigned int plainLength = cipherLength - N_BLOCK;
	return plainLength;
}

void AESM::getRidOfPadding(byte* plain, uint16_t plainLength) {
	byte last = plain[plainLength - 1];
	// get rid of pkcs7 padding
	if (0 < last && last <= 16) // maybe memset ????
		for (byte i = plainLength - 1; i >= (plainLength - last); i--)
			plain[i] = '\0';
}

void AESM::encrypt(byte* plain, unsigned int plainLength, byte* cipher, unsigned int cipherLength) {
	byte iv[N_BLOCK];
	for (unsigned int i = 0; i < N_BLOCK; i++) iv[i] = random(256);
	memcpy(cipher, iv, N_BLOCK);
	this->aes.do_aes_encrypt(plain, plainLength, cipher + N_BLOCK, this->key, 256, iv);
}

void AESM::decrypt(byte* plain, byte* cipher, unsigned int cipherLength) {
	byte iv[N_BLOCK];
	memcpy(iv, cipher, N_BLOCK);
	this->aes.do_aes_decrypt(cipher + N_BLOCK, cipherLength - N_BLOCK, plain, this->key, 256, iv);
}

String AESM::encrypt(byte* plain, int plainLength) {
	int padedLength = plainLength + N_BLOCK - plainLength % N_BLOCK;
	unsigned int length = N_BLOCK + padedLength;
	byte cipher[length];
	this->encrypt(plain, plainLength, cipher, length);
	unsigned int b64Length = (4 * (length + 2) / 3) + 2;
	unsigned char base64[b64Length]; base64[b64Length - 1] = '\0';
	unsigned int base64_length = Base64::encode(cipher, length, base64);
	return String((char*)base64);
}

String AESM::decrypt(byte* base64) {
	unsigned int binary_length = Base64::decode_length(base64);
	// this is because in this implementation we assume that the first 16 bytes of the message
	// are the encryption iv followed by the encrypted data
	if (2 * N_BLOCK <= binary_length) {
		uint16_t cipherLength = binary_length - N_BLOCK;
		unsigned char cipher[binary_length];
		binary_length = Base64::decode(base64, cipher);
		byte plain[cipherLength + 1];
		plain[cipherLength] = '\0';
		this->decrypt(plain, cipher, cipherLength + N_BLOCK);
		byte last = plain[cipherLength - 1];
		// get rid of pkcs7 padding
		if (0 < last && last <= N_BLOCK)
			for (uint16_t i = cipherLength - 1; i >= (cipherLength - last); i--)
				plain[i] = '\0';

		return String((char*) plain);
	} else {
		return "";
	}
}

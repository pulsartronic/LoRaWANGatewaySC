#include <AES.h>
#include <Base64M.h>

#ifndef __AESM__
#define __AESM__

// CBC implementation
class AESM {
	public:
	AES aes;
	byte key[32];

	AESM(byte* key);
	static uint16_t calculateCipherLength(int plainLength);
	static uint16_t calculatePlainLength(int cipherLength);
	static void getRidOfPadding(byte* plain, uint16_t plainLength);
	void encrypt(byte* plain, unsigned int plainLength, byte* cipher, unsigned int cipherLength);
	void decrypt(byte* plain, byte* cipher, unsigned int cipherLength);
	String encrypt(byte* plain, int plainLength);
	String decrypt(byte* base64);
};

#endif

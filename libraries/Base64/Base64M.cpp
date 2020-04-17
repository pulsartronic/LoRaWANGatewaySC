#include <Base64M.h>

unsigned char Base64::from_binary(unsigned char v) {
  // Capital letters - 'A' is ascii 65 and base64 0
  if(v < 26) return v + 'A';
  // Lowercase letters - 'a' is ascii 97 and base64 26
  if(v < 52) return v + 71;
  // Digits - '0' is ascii 48 and base64 52
  if(v < 62) return v - 4;
  // '+' is ascii 43 and base64 62
  if(v == 62) return '+';
  // '/' is ascii 47 and base64 63
  if(v == 63) return '/';
  return 64;
}

unsigned char Base64::to_binary(unsigned char c) {
  // Capital letters - 'A' is ascii 65 and base64 0
  if('A' <= c && c <= 'Z') return c - 'A';
  // Lowercase letters - 'a' is ascii 97 and base64 26
  if('a' <= c && c <= 'z') return c - 71;
  // Digits - '0' is ascii 48 and base64 52
  if('0' <= c && c <= '9') return c + 4;
  // '+' is ascii 43 and base64 62
  if(c == '+') return 62;
  // '/' is ascii 47 and base64 63
  if(c == '/') return 63;
  return 255;
}

unsigned int Base64::encode_length(unsigned int input_length) {
  return (input_length + 2)/3*4;
}

unsigned int Base64::decode_length(unsigned char input[]) {
  unsigned char *start = input;
  
  while(Base64::to_binary(input[0]) < 64) {
    ++input;
  }
  
  unsigned int input_length = input - start;
  
  unsigned int output_length = input_length/4*3;
  
  switch(input_length % 4) {
    default: return output_length;
    case 2: return output_length + 1;
    case 3: return output_length + 2;
  }
}

unsigned int Base64::encode(unsigned char input[], unsigned int input_length, unsigned char output[]) {
  unsigned int full_sets = input_length/3;
  
  // While there are still full sets of 24 bits...
  for(unsigned int i = 0; i < full_sets; ++i) {
    output[0] = Base64::from_binary(                         input[0] >> 2);
    output[1] = Base64::from_binary((input[0] & 0x03) << 4 | input[1] >> 4);
    output[2] = Base64::from_binary((input[1] & 0x0F) << 2 | input[2] >> 6);
    output[3] = Base64::from_binary( input[2] & 0x3F);
    
    input += 3;
    output += 4;
  }
  
  switch(input_length % 3) {
    case 0:
      output[0] = '\0';
      break;
    case 1:
      output[0] = Base64::from_binary(                         input[0] >> 2);
      output[1] = Base64::from_binary((input[0] & 0x03) << 4);
      output[2] = '=';
      output[3] = '=';
      output[4] = '\0';
      break;
    case 2:
      output[0] = Base64::from_binary(                         input[0] >> 2);
      output[1] = Base64::from_binary((input[0] & 0x03) << 4 | input[1] >> 4);
      output[2] = Base64::from_binary((input[1] & 0x0F) << 2);
      output[3] = '=';
      output[4] = '\0';
      break;
  }
  
  return Base64::encode_length(input_length);
}

unsigned int Base64::decode(unsigned char input[], unsigned char output[]) {
  unsigned int output_length = Base64::decode_length(input);
  
  // While there are still full sets of 24 bits...
  for(unsigned int i = 2; i < output_length; i += 3) {
    output[0] = Base64::to_binary(input[0]) << 2 | Base64::to_binary(input[1]) >> 4;
    output[1] = Base64::to_binary(input[1]) << 4 | Base64::to_binary(input[2]) >> 2;
    output[2] = Base64::to_binary(input[2]) << 6 | Base64::to_binary(input[3]);
    
    input += 4;
    output += 3;
  }
  
  switch(output_length % 3) {
    case 1:
      output[0] = Base64::to_binary(input[0]) << 2 | Base64::to_binary(input[1]) >> 4;
      break;
    case 2:
      output[0] = Base64::to_binary(input[0]) << 2 | Base64::to_binary(input[1]) >> 4;
      output[1] = Base64::to_binary(input[1]) << 4 | Base64::to_binary(input[2]) >> 2;
      break;
  }
  
  return output_length;
}


#include <stdio.h>
#include <string.h>
#include "hash.h"
static void run(unsigned char algo, const char*s){
  unsigned char d[32]; hash_compute(algo,(const unsigned char*)s,(unsigned short)strlen(s),d);
  printf("%-8s %-10s ", algo_name(algo), s[0]?s:"(empty)");
  for(int i=0;i<hash_digest_len(algo);i++) printf("%02x",d[i]);
  printf("\n");
}
int main(void){
  run(ALGO_MD5,"abc"); run(ALGO_MD5,"");
  run(ALGO_SHA1,"abc");
  run(ALGO_SHA256,"abc"); run(ALGO_SHA256,"");
  run(ALGO_CRC32,"abc");
  run(ALGO_NTLM,"password"); run(ALGO_NTLM,"abc");
  // longer-than-one-block input to exercise multi-block padding:
  run(ALGO_SHA256,"The quick brown fox jumps over the lazy dog");
  run(ALGO_MD5,"The quick brown fox jumps over the lazy dog");
  return 0;
}

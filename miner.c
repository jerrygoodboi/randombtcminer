#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <openssl/sha.h>
//#define NO 48

    uint32_t version = 0x312cc000;
    const char *prev_block = "000000000000000000014906ea57ed18b76cb826db91bcdb4d0f27537eac80d7";
    const char *merkle_root = "ab660a972ac145a6d98b8e21ef423ce380664767280ac6c8b3a8ccb63335ffb2";
    uint32_t timestamp = 1741712402;
    uint32_t bits = 386040449;
    size_t counter = 0;
    char block_hash[65];
    time_t t1, t2;
    uint8_t block_header[80] = {0};
    uint8_t prev_block_bytes[32];
    uint8_t merkle_root_bytes[32];
    uint8_t hash1[SHA256_DIGEST_LENGTH];
    uint8_t hash2[SHA256_DIGEST_LENGTH];
struct sande{
	int start;
	int end;
};
void *th(void *arg){
    struct sande *args = (struct sande*)arg; 
    uint32_t nonce = args->start;
    uint32_t end = args->end;
    while(nonce <= end) {
        t1 = time(0);
        if (t1 - t2 >= 1) {
            printf("%d %lu H/s\n",pthread_self(), counter);
            t2 = t1;
            counter = 0;
        }

        memcpy(block_header + 76, &nonce, 4);
        SHA256(block_header, 80, hash1);
        SHA256(hash1, SHA256_DIGEST_LENGTH, hash2);

        for (int i = 0; i < 32; i++) {
            sprintf(block_hash + i * 2, "%02x", hash2[31 - i]);
        }
        block_hash[64] = '\0';

        nonce++;
        counter++;
    }
}
int main(int argc, char **argv) {
    short cpu = sysconf(_SC_NPROCESSORS_ONLN);
    t1 = time(0);
    t2 = t1 + 1;
    //uint64_t range = 4294967296 / cpu;
    uint64_t range = 1600000 / cpu;
    struct sande values[cpu];
    for (int i = 0; i < cpu; i++) {
    	values[i].start = i * range;
    	values[i].end = (i + 1) * range;
    }

    for (int i = 0; i < 32; i++) {
        sscanf(prev_block + (31 - i) * 2, "%2hhx", &prev_block_bytes[i]);
        sscanf(merkle_root + (31 - i) * 2, "%2hhx", &merkle_root_bytes[i]);
    }

    memcpy(block_header, &version, 4);
    memcpy(block_header + 4, prev_block_bytes, 32);
    memcpy(block_header + 36, merkle_root_bytes, 32);
    memcpy(block_header + 68, &timestamp, 4);
    memcpy(block_header + 72, &bits, 4);
    int NO = atoi(argv[1]);
    pthread_t t[NO];
    for( int i = 0; i < NO; i++)
    pthread_create(&t[i], 0, th, &values[i]);
    for( int i = 0; i < NO; i++)
    pthread_join(t[i], 0);

}

#ifndef RECORD_H
#define RECORD_H

#include <string>
using namespace std;

struct VaultRecord {
    int id;
    bool is_deleted;
    char category[32];
    char title[64];
    char payload[256];

    void copyText(char destination[], int max_size, const char source[]) {
        int i = 0;
        while (source[i] != '\0' && i < max_size - 1) {
            destination[i] = source[i];
            i++;
        }
        while (i < max_size) {
            destination[i] = '\0';
            i++;
        }
    }

    // NEW: Scrambles (or unscrambles) all 256 bytes of the payload using XOR (^)
    void toggleEncryption() {
        string secret_key = "B500N_VAULT_KEY"; // Your private cipher key
        int key_len = secret_key.length();

        for (int i = 0; i < 256; i++) {
            // i % key_len wraps around the key so it repeats across all 256 bytes
            payload[i] = payload[i] ^ secret_key[i % key_len];
        }
    }

    VaultRecord() {
        id = -1;
        is_deleted = false;
        copyText(category, 32, "");
        copyText(title, 64, "");
        copyText(payload, 256, "");
    }

    VaultRecord(int record_id, const char cat[], const char ttl[], const char data[]) {
        id = record_id;
        is_deleted = false;
        copyText(category, 32, cat);
        copyText(title, 64, ttl);
        copyText(payload, 256, data);
    }
};

#endif
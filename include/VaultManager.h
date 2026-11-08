#ifndef VAULT_MANAGER_H
#define VAULT_MANAGER_H

#include "Record.h"

class VaultManager {
private:
    const char* file_path;
    int getTotalRecords();

public:
    VaultManager(const char* path);

    bool addRecord(const char category[], const char title[], const char payload[]);
    void listRecords();
    bool getRecord(int id);
    bool updateRecord(int id, const char new_payload[]);
    void searchRecords(const char keyword[]);
    bool deleteRecord(int id);
    bool compact();
};

#endif
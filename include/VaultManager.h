#ifndef VAULT_MANAGER_H
#define VAULT_MANAGER_H

#include "Record.h"

// Node for our Linked-List Stack of deleted disk slot IDs
struct FreeSlotNode {
    int deleted_id;
    FreeSlotNode* next;

    FreeSlotNode(int id) {
        deleted_id = id;
        next = nullptr;
    }
};

class VaultManager {
private:
    const char* file_path;

    // Top pointer for our LIFO Free-List Stack in RAM
    FreeSlotNode* free_stack_top;

    int getTotalRecords();

    // Stack operations to track & recycle deleted record slots
    void loadFreeSlotsIntoStack();
    void pushFreeId(int id);
    int popFreeId();
    bool isStackEmpty();
    void clearStack();

public:
    VaultManager(const char* path);
    ~VaultManager(); // Destructor to free dynamic Stack nodes

    bool addRecord(const char category[], const char title[], const char payload[]);
    void listRecords();
    bool getRecord(int id);
    bool updateRecord(int id, const char new_payload[]);
    bool deleteRecord(int id);
    bool compact();
    void searchRecords(const char keyword[]);
};

#endif
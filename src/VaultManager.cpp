#include "VaultManager.h"
#include <iostream>
#include <fstream>
#include <cstdio>
using namespace std;

// Constructor: Opens/creates vault.dat and builds the Free-List Stack in RAM
VaultManager::VaultManager(const char* path) {
    file_path = path;
    free_stack_top = nullptr;

    ifstream check_file(file_path, ios::binary);
    if (!check_file.is_open()) {
        ofstream create_file(file_path, ios::binary);
        create_file.close();
    } else {
        check_file.close();
    }

    // Scan the file and push any deleted record IDs onto our Stack
    loadFreeSlotsIntoStack();
}

// Destructor: Prevents memory leaks by freeing all Stack nodes when program exits
VaultManager::~VaultManager() {
    clearStack();
}

// --- STACK DATA STRUCTURE OPERATIONS ---

void VaultManager::pushFreeId(int id) {
    FreeSlotNode* new_node = new FreeSlotNode(id);
    new_node->next = free_stack_top;
    free_stack_top = new_node;
}

int VaultManager::popFreeId() {
    if (free_stack_top == nullptr) return -1;

    FreeSlotNode* temp = free_stack_top;
    int recycled_id = temp->deleted_id;

    free_stack_top = free_stack_top->next;
    delete temp;

    return recycled_id;
}

bool VaultManager::isStackEmpty() {
    return (free_stack_top == nullptr);
}

void VaultManager::clearStack() {
    while (free_stack_top != nullptr) {
        popFreeId();
    }
}

// Scans vault.dat and pushes all soft-deleted IDs onto the Free-List Stack
void VaultManager::loadFreeSlotsIntoStack() {
    ifstream file(file_path, ios::binary);
    if (!file.is_open()) return;

    VaultRecord rec;
    while (file.read((char*)&rec, sizeof(VaultRecord))) {
        if (rec.is_deleted == true) {
            pushFreeId(rec.id);
        }
    }
    file.close();
}

// Helper: Counts total 360-byte slots on disk
int VaultManager::getTotalRecords() {
    ifstream file(file_path, ios::binary);
    if (!file.is_open()) return 0;

    file.seekg(0, ios::end);
    int total_bytes = file.tellg();
    file.close();

    return total_bytes / sizeof(VaultRecord);
}

// --- CRUD OPERATIONS (UPGRADED WITH STACK SLOT RECYCLING) ---

// 1. CREATE: Recycles a deleted slot from the Stack if available; otherwise appends!
bool VaultManager::addRecord(const char category[], const char title[], const char payload[]) {
    // Check our Stack first!
    if (!isStackEmpty()) {
        int recycled_id = popFreeId();
        VaultRecord rec(recycled_id, category, title, payload);
        rec.toggleEncryption();

        // Open with ios::in | ios::out so we can jump to the middle and overwrite the dead slot
        fstream file(file_path, ios::binary | ios::in | ios::out);
        if (!file.is_open()) {
            cout << "[Error] Could not open vault file!\n";
            return false;
        }

        int byte_position = recycled_id * sizeof(VaultRecord);
        file.seekp(byte_position);
        file.write((char*)&rec, sizeof(VaultRecord));
        file.close();

        cout << "[OK] Recycled deleted slot #" << recycled_id << " from Stack for (" << title << ")\n";
        return true;
    }

    // If Stack is empty (no deleted holes exist), append to the end as usual
    int next_id = getTotalRecords();
    VaultRecord rec(next_id, category, title, payload);
    rec.toggleEncryption();

    ofstream file(file_path, ios::binary | ios::app);
    if (!file.is_open()) {
        cout << "[Error] Could not open vault file!\n";
        return false;
    }

    file.write((char*)&rec, sizeof(VaultRecord));
    file.close();

    cout << "[OK] Added & encrypted record #" << next_id << " (" << title << ")\n";
    return true;
}

// 2. READ ALL
void VaultManager::listRecords() {
    ifstream file(file_path, ios::binary);
    if (!file.is_open()) {
        cout << "[Error] Could not open vault file!\n";
        return;
    }

    VaultRecord rec;
    int active_count = 0;

    cout << "\nID  | Category     | Title\n";
    cout << "----+--------------+--------------------------------\n";

    while (file.read((char*)&rec, sizeof(VaultRecord))) {
        if (rec.is_deleted == false) {
            cout << rec.id << "   | " << rec.category << "          | " << rec.title << "\n";
            active_count++;
        }
    }

    if (active_count == 0) {
        cout << "(Vault is empty)\n";
    }
    cout << "\n";
    file.close();
}

// 3. READ ONE
bool VaultManager::getRecord(int id) {
    int total = getTotalRecords();
    if (id < 0 || id >= total) {
        cout << "[Error] Record ID " << id << " does not exist.\n";
        return false;
    }

    ifstream file(file_path, ios::binary);
    if (!file.is_open()) return false;

    int byte_position = id * sizeof(VaultRecord);
    file.seekg(byte_position);

    VaultRecord rec;
    file.read((char*)&rec, sizeof(VaultRecord));
    file.close();

    if (rec.is_deleted == true) {
        cout << "[Error] Record #" << id << " was deleted.\n";
        return false;
    }

    rec.toggleEncryption();

    cout << "\n=== Record #" << rec.id << " ===\n";
    cout << "Category : " << rec.category << "\n";
    cout << "Title    : " << rec.title << "\n";
    cout << "Payload  : " << rec.payload << "\n\n";
    return true;
}

// 4. UPDATE
bool VaultManager::updateRecord(int id, const char new_payload[]) {
    int total = getTotalRecords();
    if (id < 0 || id >= total) {
        cout << "[Error] Record ID " << id << " does not exist.\n";
        return false;
    }

    fstream file(file_path, ios::binary | ios::in | ios::out);
    if (!file.is_open()) return false;

    int byte_position = id * sizeof(VaultRecord);

    file.seekg(byte_position);
    VaultRecord rec;
    file.read((char*)&rec, sizeof(VaultRecord));

    if (rec.is_deleted == true) {
        cout << "[Error] Cannot update a deleted record.\n";
        file.close();
        return false;
    }

    rec.copyText(rec.payload, 256, new_payload);
    rec.toggleEncryption();

    file.seekp(byte_position);
    file.write((char*)&rec, sizeof(VaultRecord));
    file.close();

    cout << "[OK] Updated & encrypted record #" << id << "\n";
    return true;
}

// 5. DELETE: Marks is_deleted = true AND pushes the ID onto the Free-List Stack
bool VaultManager::deleteRecord(int id) {
    int total = getTotalRecords();
    if (id < 0 || id >= total) {
        cout << "[Error] Record ID " << id << " does not exist.\n";
        return false;
    }

    fstream file(file_path, ios::binary | ios::in | ios::out);
    if (!file.is_open()) return false;

    int byte_position = id * sizeof(VaultRecord);

    file.seekg(byte_position);
    VaultRecord rec;
    file.read((char*)&rec, sizeof(VaultRecord));

    if (rec.is_deleted == true) {
        cout << "[Warn] Record #" << id << " is already deleted.\n";
        file.close();
        return false;
    }

    rec.is_deleted = true;
    file.seekp(byte_position);
    file.write((char*)&rec, sizeof(VaultRecord));
    file.close();

    // Push the newly freed ID onto our Stack!
    pushFreeId(id);

    cout << "[OK] Deleted record #" << id << " (Slot pushed to Free-List Stack)\n";
    return true;
}

// 6. COMPACT: Purges deleted slots and clears the Free-List Stack
bool VaultManager::compact() {
    const char* temp_path = "data/vault_temp.dat";
    ifstream in_file(file_path, ios::binary);
    ofstream out_file(temp_path, ios::binary);

    if (!in_file.is_open() || !out_file.is_open()) return false;

    VaultRecord rec;
    int new_id = 0;
    int removed_count = 0;

    while (in_file.read((char*)&rec, sizeof(VaultRecord))) {
        if (rec.is_deleted == false) {
            rec.id = new_id;
            new_id++;
            out_file.write((char*)&rec, sizeof(VaultRecord));
        } else {
            removed_count++;
        }
    }

    in_file.close();
    out_file.close();

    remove(file_path);
    rename(temp_path, file_path);

    // Since all holes are gone, clear the Free-List Stack
    clearStack();

    cout << "[OK] Cleaned up " << removed_count << " deleted record(s).\n";
    return true;
}

// Helper 1: Converts an uppercase letter to lowercase
char toLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

// Helper 2: Substring search
bool containsText(const char text[], const char keyword[]) {
    if (keyword[0] == '\0') return true;

    int i = 0;
    while (text[i] != '\0') {
        int j = 0;
        while (keyword[j] != '\0' && text[i + j] != '\0' &&
               toLowerChar(text[i + j]) == toLowerChar(keyword[j])) {
            j++;
        }
        if (keyword[j] == '\0') {
            return true;
        }
        i++;
    }
    return false;
}

// 7. SEARCH
void VaultManager::searchRecords(const char keyword[]) {
    ifstream file(file_path, ios::binary);
    if (!file.is_open()) {
        cout << "[Error] Could not open vault file!\n";
        return;
    }

    VaultRecord rec;
    int match_count = 0;

    cout << "\nSearch Results for \"" << keyword << "\":\n";
    cout << "ID  | Category     | Title\n";
    cout << "----+--------------+--------------------------------\n";

    while (file.read((char*)&rec, sizeof(VaultRecord))) {
        if (rec.is_deleted == false) {
            if (containsText(rec.category, keyword) || containsText(rec.title, keyword)) {
                cout << rec.id << "   | " << rec.category << "          | " << rec.title << "\n";
                match_count++;
            }
        }
    }

    if (match_count == 0) {
        cout << "(No matching records found)\n";
    }
    cout << "\n";
    file.close();
}
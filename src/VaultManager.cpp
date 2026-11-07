#include "VaultManager.h"
#include <iostream>
#include <fstream>
#include <cstdio>
using namespace std;

VaultManager::VaultManager(const char* path) {
    file_path = path;

    // Try opening the file to see if it exists
    ifstream check_file(file_path, ios::binary);
    if (!check_file.is_open()) {
        // If it doesn't exist yet, create an empty binary file
        ofstream create_file(file_path, ios::binary);
        create_file.close();
    } else {
        check_file.close();
    }
}

// Helper: Counts how many records are inside vault.dat
int VaultManager::getTotalRecords() {
    ifstream file(file_path, ios::binary);
    if (!file.is_open()) return 0;

    // Move the reading cursor to the very end of the file (ios::end)
    file.seekg(0, ios::end);

    // tellg() tells us what byte number we are at (which equals total file size in bytes)
    int total_bytes = file.tellg();
    file.close();

    // Divide total file bytes by 360 to get the number of records
    return total_bytes / sizeof(VaultRecord);
}

// 1. CREATE: Add a new record to the end of the file
bool VaultManager::addRecord(const char category[], const char title[], const char payload[]) {
    int next_id = getTotalRecords();
    VaultRecord rec(next_id, category, title, payload);

    // Scramble the payload BEFORE writing to disk
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

// 2. READ ALL: Loop through the file from start to finish
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

    // Keep reading 360-byte chunks until we hit the end of the file
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

// 3. READ ONE: Jump straight to Record #id without reading the whole file
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

    // Unscramble the payload in RAM right before printing it
    rec.toggleEncryption();

    cout << "\n=== Record #" << rec.id << " ===\n";
    cout << "Category : " << rec.category << "\n";
    cout << "Title    : " << rec.title << "\n";
    cout << "Payload  : " << rec.payload << "\n\n";
    return true;
}

// 4. UPDATE: Overwrite a specific record right where it sits on disk
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

    // Copy the new plain-text payload, then scramble it before saving to disk
    rec.copyText(rec.payload, 256, new_payload);
    rec.toggleEncryption();

    file.seekp(byte_position);
    file.write((char*)&rec, sizeof(VaultRecord));
    file.close();

    cout << "[OK] Updated & encrypted record #" << id << "\n";
    return true;
}

// 5. DELETE: Mark is_deleted = true on disk
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

    // Flip the boolean flag and write the struct back to the same spot
    rec.is_deleted = true;
    file.seekp(byte_position);
    file.write((char*)&rec, sizeof(VaultRecord));
    file.close();

    cout << "[OK] Deleted record #" << id << "\n";
    return true;
}

// 6. COMPACT: Copy all non-deleted records into a fresh file
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

    // Delete old file and rename temp file to vault.dat
    remove(file_path);
    rename(temp_path, file_path);

    cout << "[OK] Cleaned up " << removed_count << " deleted record(s).\n";
    return true;
}
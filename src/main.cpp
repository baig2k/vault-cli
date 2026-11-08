#include <iostream>
#include <string>
#include "VaultManager.h"
using namespace std;

// Helper 1: Prints the usage manual if the user types an incomplete command
void printHelp() {
    cout << "\n=== Vault CLI - Command Reference ===\n";
    cout << "  add <category> <title> <payload>  : Save a new snippet or secret\n";
    cout << "  list                              : Show all saved records\n";
    cout << "  get <id>                          : View the full payload of record #id\n";
    cout << "  update <id> <new_payload>         : Change the payload of record #id\n";
    cout << "  delete <id>                       : Soft-delete record #id\n";
    cout << "  compact                           : Permanently remove deleted records\n\n";
    cout << "  search <keyword>                  : Find records by category or title\n";
}

// Helper 2: Converts a C-string like "12" into the integer 12 using basic PF ASCII math
int textToInt(const char text[]) {
    if (text[0] == '\0') return -1; // Empty string check

    int result = 0;
    int i = 0;

    while (text[i] != '\0') {
        // Check if the character is actually a digit between '0' and '9'
        if (text[i] < '0' || text[i] > '9') {
            return -1; // Return -1 if the user typed letters instead of a number
        }
        // Shift previous digits left by multiplying by 10, then add the new digit
        result = (result * 10) + (text[i] - '0');
        i++;
    }
    return result;
}

int main(int argc, char* argv[]) {
    // If the user only ran "./build/vault.exe" with no command, show the help menu
    if (argc < 2) {
        printHelp();
        return 0;
    }

    VaultManager vault("data/vault.dat");

    // Store argv[1] in a C++ string so we can compare it easily using ==
    string command = argv[1];

    // 1. CREATE: ./build/vault.exe add <category> <title> <payload> (needs 5 words total)
    if (command == "add") {
        if (argc < 5) {
            cout << "[Error] Missing arguments! Usage: vault add <category> <title> <payload>\n";
            return 1;
        }
        vault.addRecord(argv[2], argv[3], argv[4]);
    }
    // 2. READ ALL: ./build/vault.exe list
    else if (command == "list") {
        vault.listRecords();
    }
    // 3. READ ONE: ./build/vault.exe get <id> (needs 3 words total)
    else if (command == "get") {
        if (argc < 3) {
            cout << "[Error] Missing ID! Usage: vault get <id>\n";
            return 1;
        }
        int id = textToInt(argv[2]);
        vault.getRecord(id);
    }
    // 4. UPDATE: ./build/vault.exe update <id> <new_payload> (needs 4 words total)
    else if (command == "update") {
        if (argc < 4) {
            cout << "[Error] Missing arguments! Usage: vault update <id> <new_payload>\n";
            return 1;
        }
        int id = textToInt(argv[2]);
        vault.updateRecord(id, argv[3]);
    }
    // 5. DELETE: ./build/vault.exe delete <id> (needs 3 words total)
    else if (command == "delete") {
        if (argc < 3) {
            cout << "[Error] Missing ID! Usage: vault delete <id>\n";
            return 1;
        }
        int id = textToInt(argv[2]);
        vault.deleteRecord(id);
    }
    // 6. COMPACT: ./build/vault.exe compact
    else if (command == "compact") {
        vault.compact();
    }
    // 7. SEARCH: ./build/vault.exe search <keyword> (needs 3 words total)
    else if (command == "search") {
    if (argc < 3) {
        cout << "[Error] Missing keyword! Usage: vault search <keyword>\n";
        return 1;
    }
    vault.searchRecords(argv[2]);
}
    // Unknown command typed
    else {
        cout << "[Error] Unknown command: '" << command << "'\n";
        printHelp();
        return 1;
    }

    return 0;
}
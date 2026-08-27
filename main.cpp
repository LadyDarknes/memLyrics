#include <windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

int main() {
    constexpr size_t BUF_SIZE = 64;
    char* mem = (char*)VirtualAlloc((void*)0x0000021A4B00, BUF_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!mem) mem = (char*)VirtualAlloc(nullptr, BUF_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!mem) return 1;

    std::cout << "0x" << std::hex << (uintptr_t)mem << "\n";

    const char* song[] = {
        "Bazen bana gelir gider seni dertler",
        "Seni ruyalarimda hapsetmeler",
        "Yildizlarin hirsizlari mi var?",
        "Tutamam tutamam hep yeni bir gun",
        "Gulumse kaderine..."
    };

    Sleep(20000);
    for (const char* line : song) {
        memset(mem, 0, BUF_SIZE);

        std::istringstream iss(line);
        std::string word, current;

        while (iss >> word) {
            if (!current.empty()) current += " ";
            current += word;

            if (current.size() < BUF_SIZE) {
                memcpy(mem, current.c_str(), current.size() + 1);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }

    std::cin.get();
    VirtualFree(mem, 0, MEM_RELEASE);
    return 0;
}

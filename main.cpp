#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

int main() {
    SetConsoleOutputCP(CP_UTF8);

    constexpr size_t ROW_WIDTH = 32;
    constexpr size_t TOTAL_ROWS = 4;
    constexpr size_t BUFFER_SIZE = ROW_WIDTH * TOTAL_ROWS;

    void* base_addr = reinterpret_cast<void*>(0x0000021A4B00);
    char* mem = static_cast<char*>(VirtualAlloc(base_addr, BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!mem) {
        mem = static_cast<char*>(VirtualAlloc(nullptr, BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    }

    std::memset(mem, ' ', BUFFER_SIZE);

    std::cout << "0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(mem) << "\n";

    struct Line {
        std::string text;
        int ms;
    };

    std::vector<Line> lyrics = 
    {
        {"Bazen bana gelir, gider seni dert", 2500},
        {"Seni ruyalarimda hapsetmeler", 2500},
        {"Yildizlarin hirsizlari mi var?", 2500},
        {"Tutamam, tutamam, hep yeni bir gun", 2500},
        {"Gulumse kaderine...", 3000}
    };

    std::vector<std::string> display(TOTAL_ROWS, "");
    std::this_thread::sleep_for(std::chrono::seconds(10));

    for (const auto& l : lyrics) {
        for (size_t i = 0; i + 1 < TOTAL_ROWS; ++i) {
            display[i] = display[i + 1];
        }
        display[TOTAL_ROWS - 1] = l.text;

        std::memset(mem, ' ', BUFFER_SIZE);
        for (size_t i = 0; i < TOTAL_ROWS; ++i) {
            std::string line = display[i];
            if (line.size() > ROW_WIDTH) {
                line = line.substr(0, ROW_WIDTH);
            } else {
                line.resize(ROW_WIDTH, ' ');
            }
            std::memcpy(mem + (i * ROW_WIDTH), line.data(), ROW_WIDTH);
        }

        std::cout << "> " << l.text << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(l.ms));
    }

    std::cout << "\n[+] done.\n";
    std::cin.get();

    VirtualFree(mem, 0, MEM_RELEASE);
    return 0;
}

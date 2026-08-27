#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

struct LyricLine {
    std::string text;
    int line_pause_ms{1500};
};

int main() {
    constexpr size_t ROW_WIDTH = 16;
    constexpr size_t TOTAL_ROWS = 6;
    constexpr size_t BUFFER_SIZE = ROW_WIDTH * TOTAL_ROWS;
    constexpr int CHAR_DELAY_MS = 60;

    void* base_addr = reinterpret_cast<void*>(0x0000021A4B00);
    char* mem = static_cast<char*>(VirtualAlloc(base_addr, BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!mem) {
        mem = static_cast<char*>(VirtualAlloc(nullptr, BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!mem) return 1;
    }

    std::memset(mem, ' ', BUFFER_SIZE);
    std::cout << "0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(mem) << "\n";

    std::vector<LyricLine> song = {
        {"Bazen bana", 1200},
        {"gelir gider", 1200},
        {"seni dertler", 1500},
        {"Seni ruyamda", 1200},
        {"hapsetmeler", 1600},
        {"Yildizlarin", 1200},
        {"hirsizi mi var?", 1600},
        {"Tutamam tutamam", 1500},
        {"hep yeni bir gun", 1600},
        {"Gulumse kaderine", 2500}
    };

    std::vector<std::string> rows(TOTAL_ROWS, std::string(ROW_WIDTH, ' '));

    auto sync_memory = [&]() {
        for (size_t r = 0; r < TOTAL_ROWS; ++r) {
            std::string row_str = rows[r];
            if (row_str.size() > ROW_WIDTH) {
                row_str = row_str.substr(0, ROW_WIDTH);
            } else {
                row_str.resize(ROW_WIDTH, ' ');
            }
            std::memcpy(mem + (r * ROW_WIDTH), row_str.data(), ROW_WIDTH);
        }
    };

    std::this_thread::sleep_for(std::chrono::seconds(2));

    for (const auto& item : song) {
        for (size_t i = 0; i + 1 < TOTAL_ROWS; ++i) {
            rows[i] = rows[i + 1];
        }
        rows[TOTAL_ROWS - 1] = std::string(ROW_WIDTH, ' ');

        for (char c : item.text) {
            std::string& current = rows[TOTAL_ROWS - 1];
            size_t non_space = current.find_last_not_of(' ');
            size_t next_idx = (non_space == std::string::npos) ? 0 : non_space + 1;

            if (next_idx < ROW_WIDTH) {
                current[next_idx] = c;
                sync_memory();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(CHAR_DELAY_MS));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(item.line_pause_ms));
    }

    std::cin.get();
    VirtualFree(mem, 0, MEM_RELEASE);
    return 0;
}

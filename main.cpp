#include <windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

struct LyricEntry {
    const char* text;
    int start_ms;
    int end_ms;
};

int main() {
    constexpr size_t BUF_SIZE = 64;
    constexpr int TIMING_OFFSET_MS = -900; 

    char* mem = (char*)VirtualAlloc((void*)0x0000021A4B00, BUF_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!mem) mem = (char*)VirtualAlloc(nullptr, BUF_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!mem) return 1;

    std::cout << "0x" << std::hex << (uintptr_t)mem << "\n";

    const LyricEntry song[] = {
        {"Bazen bana gelir, gider seni dert etmeler", 34850, 41700},
        {"Seni ruyalarimda hapsetmeler", 41700, 45800},
        {"Yildizlarin hirsizlari mi var?", 45800, 51100},
        {"Tutamam, tutamam, hep yeni bir gun dogar", 51100, 56000},
        {"Baska bi' evrende, en guzel halinle", 56000, 61550},
        {"Sen hayata karis, ben daha da bitecegim", 61550, 66900},
        {"Kirginim kendime, usuyorum golgende", 66900, 72400},
        {"Henuz bilmesen de belki bir gun gidecegim", 72400, 77900},
        {"Hic gerek yok daha fazlasina", 88750, 93550},
        {"Zamani tutmaya, fezaya ucmaya", 93550, 98800},
        {"Geride kaldilar, geride kaldi o gunler", 98800, 104450},
        {"Sen varken taptigim kasvetli sehirler", 104450, 110600},
        {"Baska bi' evrende, en guzel halinle", 110600, 116000},
        {"Sen hayata karis, ben daha da bitecegim", 116000, 121400},
        {"Kirginim kendime, usuyorum golgende", 121400, 127000},
        {"Henuz bilmesen de belki bir gun gidecegim", 127000, 133800},
        {"Baska bi' evrende, en guzel halinle", 165100, 170500},
        {"Sen hayata karis, ben daha da bitecegim", 170500, 176050},
        {"Kirginim kendime, usuyorum golgende", 176050, 181500},
        {"Henuz bilmesen de belki bir gun gidecegim", 181500, 188490}
    };

    memset(mem, 0, BUF_SIZE);
    auto start_time = std::chrono::steady_clock::now();

    for (const auto& entry : song) {
        int target_start = (std::max)(0, entry.start_ms + TIMING_OFFSET_MS);
        int target_end = (std::max)(0, entry.end_ms + TIMING_OFFSET_MS);

        while (true) {
            auto now = std::chrono::steady_clock::now();
            int elapsed = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
            if (elapsed >= target_start) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        memset(mem, 0, BUF_SIZE);

        std::vector<std::string> words;
        std::istringstream iss(entry.text);
        std::string w;
        int total_weight = 0;

        while (iss >> w) {
            words.push_back(w);
            int weight = (int)w.size();
            if (!w.empty() && (w.back() == ',' || w.back() == '?')) weight += 3;
            total_weight += (std::max)(2, weight);
        }

        int total_duration = target_end - target_start;
        std::string current;

        for (const auto& word : words) {
            if (!current.empty()) current += " ";
            current += word;

            if (current.size() < BUF_SIZE) {
                memcpy(mem, current.c_str(), current.size() + 1);
            }

            int weight = (int)word.size();
            if (!word.empty() && (word.back() == ',' || word.back() == '?')) weight += 3;
            int word_delay = (total_duration * (std::max)(2, weight)) / (std::max)(1, total_weight);

            std::this_thread::sleep_for(std::chrono::milliseconds(word_delay));
        }

        while (true) {
            auto now = std::chrono::steady_clock::now();
            int elapsed = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
            if (elapsed >= target_end) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    std::cin.get();
    VirtualFree(mem, 0, MEM_RELEASE);
    return 0;
}

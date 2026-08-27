#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>

#include "player.hpp"
#include "lyrics.hpp"

int main() {
    SetConsoleOutputCP(CP_UTF8);
    winrt::init_apartment();

    constexpr size_t BUF_SIZE = 64;
    char* mem = static_cast<char*>(VirtualAlloc(
        reinterpret_cast<void*>(0x0000021A4B00),
        BUF_SIZE,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    ));

    if (!mem) {
        mem = static_cast<char*>(VirtualAlloc(nullptr, BUF_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!mem) return 1;
    }

    std::memset(mem, 0, BUF_SIZE);
    std::cout << "0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(mem) << std::dec << "\n";

    PlayerTracker tracker;
    std::string current_song_key;
    std::vector<LyricLine> lyrics;
    std::string last_written;

    while (true) {
        PlayerState state = tracker.update();

        if (state.valid) {
            std::string key = state.artist + " - " + state.title;
            if (key != current_song_key) {
                current_song_key = key;
                lyrics = LyricsService::fetch(state.artist, state.title, state.duration_ms / 1000);
                std::memset(mem, 0, BUF_SIZE);
                last_written = "";
                std::cout << "[+] " << key << " (" << lyrics.size() << " satir soz yuklendi)\n";
            }

            if (state.is_playing && !lyrics.empty()) {
                int cur_ms = state.progress_ms;

                int active_idx = -1;
                for (size_t i = 0; i < lyrics.size(); ++i) {
                    if (cur_ms >= lyrics[i].start_ms && cur_ms < lyrics[i].end_ms) {
                        active_idx = static_cast<int>(i);
                        break;
                    }
                }

                if (active_idx != -1) {
                    const auto& line = lyrics[active_idx];
                    int elapsed = cur_ms - line.start_ms;
                    int duration = (std::max)(1, line.end_ms - line.start_ms);

                    std::string stream_text;
                    if (!line.words.empty()) {
                        size_t num_words = line.words.size();

                        // Kelime kelime kesin anlık zamanlama:
                        // Saniye ilerledikçe sadece o ana kadar söylenmiş kelimeler eklenir
                        size_t visible_words = (std::min)(num_words, static_cast<size_t>((elapsed * num_words) / duration) + 1);

                        for (size_t w = 0; w < visible_words; ++w) {
                            if (!stream_text.empty()) stream_text += " ";
                            stream_text += line.words[w];
                        }
                    }

                    if (stream_text != last_written) {
                        last_written = stream_text;
                        std::memset(mem, 0, BUF_SIZE);
                        if (stream_text.size() >= BUF_SIZE) stream_text = stream_text.substr(0, BUF_SIZE - 1);
                        std::memcpy(mem, stream_text.c_str(), stream_text.size() + 1);
                    }
                } else {
                    if (!last_written.empty()) {
                        last_written = "";
                        std::memset(mem, 0, BUF_SIZE);
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    VirtualFree(mem, 0, MEM_RELEASE);
    return 0;
}

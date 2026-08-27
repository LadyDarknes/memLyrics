#pragma once

#include <string>
#include <vector>

struct LyricLine {
    int start_ms{0};
    int end_ms{0};
    int vocal_duration_ms{0};
    std::string text;
    std::vector<std::string> words;
    std::vector<int> cumulative_weights;
    int total_weight{0};
};

class LyricsService {
public:
    static std::vector<LyricLine> fetch(const std::string& artist, const std::string& title, int duration_sec);

private:
    static std::vector<LyricLine> parse(const std::string& synced_lrc);
};

#include "lyrics.hpp"
#include "utils.hpp"
#include <sstream>
#include <regex>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<LyricLine> LyricsService::parse(const std::string& synced_lrc) {
    std::vector<LyricLine> lines;
    if (synced_lrc.empty()) return lines;

    std::regex re(R"(\[(\d{2}):(\d{2})\.(\d{2,3})\]\s*(.*))");
    std::smatch match;
    std::istringstream iss(synced_lrc);
    std::string line;

    while (std::getline(iss, line)) {
        if (std::regex_match(line, match, re)) {
            int min = std::stoi(match[1].str());
            int sec = std::stoi(match[2].str());
            std::string frac_str = match[3].str();
            int ms = (frac_str.size() == 2) ? std::stoi(frac_str) * 10 : std::stoi(frac_str);

            std::string raw_text = match[4].str();
            std::string text = normalize_to_ascii(raw_text);
            if (text.empty()) continue;

            LyricLine entry;
            entry.start_ms = (min * 60 + sec) * 1000 + ms;
            entry.text = text;

            std::istringstream wiss(text);
            std::string w;
            int current_weight = 0;
            while (wiss >> w) {
                entry.words.push_back(w);
                int weight = (int)w.size();
                if (!w.empty() && (w.back() == ',' || w.back() == '?')) weight += 3;
                weight = (std::max)(2, weight);
                current_weight += weight;
                entry.cumulative_weights.push_back(current_weight);
            }
            entry.total_weight = (std::max)(1, current_weight);
            lines.push_back(entry);
        }
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        if (i + 1 < lines.size()) {
            lines[i].end_ms = lines[i + 1].start_ms;
        } else {
            lines[i].end_ms = lines[i].start_ms + 6000;
        }

        int total_gap = lines[i].end_ms - lines[i].start_ms;
        // Ortalama insan okuma/söyleme hızı: her karakter ~60-80ms + 500ms nefes payı
        int estimated_vocal = lines[i].total_weight * 70 + 400;
        lines[i].vocal_duration_ms = (std::clamp)(estimated_vocal, 1200, (std::max)(1200, total_gap - 250));
    }

    return lines;
}

std::vector<LyricLine> LyricsService::fetch(const std::string& artist, const std::string& title, int duration_sec) {
    std::string clean_title = clean_song_title(title);

    // 1) Doğrudan exact get araması
    std::string url = "https://lrclib.net/api/get?artist_name=" + url_encode(artist) +
                       "&track_name=" + url_encode(clean_title);
    if (duration_sec > 0) {
        url += "&duration=" + std::to_string(duration_sec);
    }

    std::string resp = http_get(url);
    try {
        if (!resp.empty()) {
            auto j = json::parse(resp);
            std::string synced = j.value("syncedLyrics", "");
            if (!synced.empty()) return parse(synced);
        }
    } catch (...) {}

    // 2) Süre filtresiz get araması
    std::string url_no_dur = "https://lrclib.net/api/get?artist_name=" + url_encode(artist) +
                             "&track_name=" + url_encode(clean_title);
    std::string resp_no_dur = http_get(url_no_dur);
    try {
        if (!resp_no_dur.empty()) {
            auto j = json::parse(resp_no_dur);
            std::string synced = j.value("syncedLyrics", "");
            if (!synced.empty()) return parse(synced);
        }
    } catch (...) {}

    // 3) Global Search araması (artist + title)
    std::string search_url = "https://lrclib.net/api/search?q=" + url_encode(artist + " " + clean_title);
    std::string search_resp = http_get(search_url);
    try {
        if (!search_resp.empty()) {
            auto j_arr = json::parse(search_resp);
            if (j_arr.is_array()) {
                for (const auto& item : j_arr) {
                    std::string synced = item.value("syncedLyrics", "");
                    if (!synced.empty()) return parse(synced);
                }
            }
        }
    } catch (...) {}

    return {};
}

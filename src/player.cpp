#include "player.hpp"
#include "utils.hpp"
#include <windows.h>

using namespace winrt;
using namespace Windows::Media::Control;

PlayerTracker::PlayerTracker() {
    try {
        manager_ = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    } catch (...) {}
}

std::string PlayerTracker::to_utf8(const winrt::hstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

PlayerState PlayerTracker::update() {
    PlayerState state;
    if (!manager_) {
        try {
            manager_ = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
        } catch (...) {
            return state;
        }
    }

    try {
        auto sessions = manager_.GetSessions();
        GlobalSystemMediaTransportControlsSession active_session = nullptr;

        for (auto&& s : sessions) {
            auto id = to_utf8(s.SourceAppUserModelId());
            if (id.find("Spotify") != std::string::npos || id.find("spotify") != std::string::npos) {
                active_session = s;
                break;
            }
        }

        if (!active_session) {
            active_session = manager_.GetCurrentSession();
        }

        if (active_session) {
            auto media = active_session.TryGetMediaPropertiesAsync().get();
            auto timeline = active_session.GetTimelineProperties();
            auto playback = active_session.GetPlaybackInfo();

            state.title = normalize_to_ascii(to_utf8(media.Title()));
            state.artist = normalize_to_ascii(to_utf8(media.Artist()));

            bool is_playing = (playback.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing);
            state.is_playing = is_playing;

            int64_t pos_ticks = timeline.Position().count();
            if (is_playing) {
                auto now = winrt::clock::now();
                auto last_updated = timeline.LastUpdatedTime();
                if (now > last_updated) {
                    int64_t diff = (now - last_updated).count();
                    if (diff > 0 && diff < 100000000LL) { // 10 saniyeden kucuk farklar icin kesintisiz enterpolasyon
                        pos_ticks += diff;
                    }
                }
            }

            state.progress_ms = static_cast<int>(pos_ticks / 10000);
            state.duration_ms = static_cast<int>(timeline.EndTime().count() / 10000);
            state.valid = !state.title.empty();
        }
    } catch (...) {}

    return state;
}

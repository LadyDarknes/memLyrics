#pragma once

#include <string>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>

#pragma comment(lib, "windowsapp.lib")

struct PlayerState {
    std::string title;
    std::string artist;
    int progress_ms{0};
    int duration_ms{0};
    bool is_playing{false};
    bool valid{false};
};

class PlayerTracker {
public:
    PlayerTracker();
    PlayerState update();

private:
    winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager manager_{nullptr};
    std::string to_utf8(const winrt::hstring& wstr);
};

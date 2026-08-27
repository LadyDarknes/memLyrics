#pragma once

#include <string>
#include <regex>
#include <windows.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

inline std::string normalize_to_ascii(const std::string& input) {
    std::string out;
    out.reserve(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);

        if (c < 0x80) {
            out += static_cast<char>(c);
            continue;
        }

        if (i + 1 < input.size()) {
            unsigned char next = static_cast<unsigned char>(input[i + 1]);

            if (c == 0xC3) {
                switch (next) {
                    case 0xBC: out += 'u'; ++i; continue;
                    case 0x9C: out += 'U'; ++i; continue;
                    case 0xB6: out += 'o'; ++i; continue;
                    case 0x96: out += 'O'; ++i; continue;
                    case 0xA7: out += 'c'; ++i; continue;
                    case 0x87: out += 'C'; ++i; continue;
                    case 0xA2: out += 'a'; ++i; continue;
                    case 0x82: out += 'A'; ++i; continue;
                    case 0xAE: out += 'i'; ++i; continue;
                    case 0x8E: out += 'I'; ++i; continue;
                    case 0xBB: out += 'u'; ++i; continue;
                    case 0x9B: out += 'U'; ++i; continue;
                }
            } else if (c == 0xC4) {
                switch (next) {
                    case 0x9F: out += 'g'; ++i; continue;
                    case 0x9E: out += 'G'; ++i; continue;
                    case 0xB1: out += 'i'; ++i; continue;
                    case 0xB0: out += 'I'; ++i; continue;
                }
            } else if (c == 0xC5) {
                switch (next) {
                    case 0x9F: out += 's'; ++i; continue;
                    case 0x9E: out += 'S'; ++i; continue;
                }
            }
        }
    }
    return out;
}

inline std::string clean_song_title(const std::string& title) {
    std::string clean = title;
    try {
        clean = std::regex_replace(clean, std::regex(R"(\s*-\s*.*)", std::regex::icase), "");
        clean = std::regex_replace(clean, std::regex(R"(\s*\((feat|ft|with|remastered|live|acoustic|deluxe|bonus|radio).*\))", std::regex::icase), "");
        clean = std::regex_replace(clean, std::regex(R"(\s*\[(feat|ft|with|remastered|live|acoustic|deluxe|bonus|radio).*\])", std::regex::icase), "");
    } catch (...) {}
    return clean.empty() ? title : clean;
}

inline std::string url_encode(const std::string& value) {
    std::string escaped;
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped += c;
        } else if (c == ' ') {
            escaped += '+';
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            escaped += buf;
        }
    }
    return escaped;
}

inline std::string http_get(const std::string& url) {
    HINTERNET hInternet = InternetOpenA("memLyrics/2.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return "";

    URL_COMPONENTSA urlComp{};
    urlComp.dwStructSize = sizeof(urlComp);
    char host[256] = {0};
    char path[2048] = {0};
    urlComp.lpszHostName = host;
    urlComp.dwHostNameLength = sizeof(host);
    urlComp.lpszUrlPath = path;
    urlComp.dwUrlPathLength = sizeof(path);

    if (!InternetCrackUrlA(url.c_str(), 0, 0, &urlComp)) {
        InternetCloseHandle(hInternet);
        return "";
    }

    INTERNET_PORT port = urlComp.nPort ? urlComp.nPort : (urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT);
    HINTERNET hConnect = InternetConnectA(hInternet, host, port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "";
    }

    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTO_REDIRECT;
    if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) flags |= INTERNET_FLAG_SECURE;

    HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", path, NULL, NULL, NULL, flags, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }

    std::string response;
    if (HttpSendRequestA(hRequest, NULL, 0, NULL, 0)) {
        char buffer[8192];
        DWORD bytesRead = 0;
        while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return response;
}

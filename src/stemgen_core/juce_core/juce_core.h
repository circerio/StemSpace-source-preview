#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace juce {
class String {
public:
    String() = default;
    String(const char* s) : s_(s ? s : "") {}
    String(const std::string& s) : s_(s) {}
    String(std::string_view s) : s_(s) {}
    String(int v) : s_(std::to_string(v)) {}
    String(long long v) : s_(std::to_string(v)) {}
    String(unsigned long long v) : s_(std::to_string(v)) {}
    String(const wchar_t* w) { assignWide(w); }

    static String fromUTF8(const char* p, int n) {
        return String(std::string(p ? p : "", p ? static_cast<size_t>(n) : 0));
    }
    void clear() { s_.clear(); wide_.clear(); }
    bool isEmpty() const noexcept { return s_.empty(); }
    bool isNotEmpty() const noexcept { return !s_.empty(); }

    bool equalsIgnoreCase(const String& other) const {
        if (s_.size() != other.s_.size()) return false;
        for (size_t i = 0; i < s_.size(); ++i) {
            const auto a = static_cast<unsigned char>(s_[i]);
            const auto b = static_cast<unsigned char>(other.s_[i]);
            if (std::tolower(a) != std::tolower(b)) return false;
        }
        return true;
    }

    const char* toRawUTF8() const noexcept { return s_.c_str(); }
    const wchar_t* toWideCharPointer() const {
        wide_ = toWideString();
        return wide_.c_str();
    }
    std::wstring toWideString() const {
        if (s_.empty()) return {};
        const int n = MultiByteToWideChar(CP_UTF8, 0, s_.data(),
            static_cast<int>(s_.size()), nullptr, 0);
        std::wstring out(static_cast<size_t>(n), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s_.data(),
            static_cast<int>(s_.size()), out.data(), n);
        return out;
    }
    const std::string& str() const noexcept { return s_; }

    friend String operator+(const String& a, const String& b) {
        return String(a.s_ + b.s_);
    }
    friend String operator+(const String& a, const char* b) {
        return String(a.s_ + (b ? b : ""));
    }
    friend String operator+(const char* a, const String& b) {
        return String((a ? std::string(a) : std::string()) + b.s_);
    }

private:
    void assignWide(const wchar_t* w) {
        if (!w || !*w) return;
        const int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0,
            nullptr, nullptr);
        if (n <= 1) return;
        s_.resize(static_cast<size_t>(n));
        WideCharToMultiByte(CP_UTF8, 0, w, -1, s_.data(), n, nullptr, nullptr);
        if (!s_.empty() && s_.back() == '\0') s_.pop_back();
    }
    std::string s_;
    mutable std::wstring wide_;
};

class File {
public:
    explicit File(const String& path) : path_(path.toWideString()) {}
    bool existsAsFile() const {
        std::error_code ec;
        return std::filesystem::is_regular_file(path_, ec);
    }
    long long getSize() const {
        std::error_code ec;
        const auto n = std::filesystem::file_size(path_, ec);
        return ec ? -1LL : static_cast<long long>(n);
    }
    const std::filesystem::path& path() const noexcept { return path_; }
private:
    std::filesystem::path path_;
};
} // namespace juce

#ifndef DBG
#define DBG(...) do {} while (0)
#endif

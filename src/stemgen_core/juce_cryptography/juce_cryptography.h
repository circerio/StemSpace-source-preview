#pragma once
#include <juce_core/juce_core.h>
#include <windows.h>
#include <wincrypt.h>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace juce {
class SHA256 {
public:
    explicit SHA256(const File& file) { compute(file); }

    String toHexString() const {
        std::ostringstream s;
        s << std::hex << std::setfill('0');
        for (unsigned char b : digest_) s << std::setw(2) << static_cast<unsigned>(b);
        return String(s.str());
    }

private:
    void compute(const File& file) {
        HCRYPTPROV provider = 0;
        HCRYPTHASH hash = 0;
        if (!CryptAcquireContextW(&provider, nullptr, nullptr, PROV_RSA_AES,
                                  CRYPT_VERIFYCONTEXT)) return;
        if (!CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash)) {
            CryptReleaseContext(provider, 0);
            return;
        }
        std::ifstream in(file.path(), std::ios::binary);
        std::array<char, 65536> buffer{};
        while (in) {
            in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const auto n = in.gcount();
            if (n > 0 && !CryptHashData(hash,
                    reinterpret_cast<const BYTE*>(buffer.data()),
                    static_cast<DWORD>(n), 0)) {
                CryptDestroyHash(hash);
                CryptReleaseContext(provider, 0);
                return;
            }
        }
        DWORD bytes = static_cast<DWORD>(digest_.size());
        CryptGetHashParam(hash, HP_HASHVAL, digest_.data(), &bytes, 0);
        CryptDestroyHash(hash);
        CryptReleaseContext(provider, 0);
    }

    std::array<unsigned char, 32> digest_{};
};
} // namespace juce

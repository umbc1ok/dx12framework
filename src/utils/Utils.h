#include <random>
#include <sstream>
#include <Windows.h>
#include "Types.h"

namespace olej_utils
{

    template<typename T>
    class Badge {
        friend T;
        Badge() {}
    };

    template<typename T>
    void swapAndErase(std::vector<T>& vector, T element)
    {
        if (auto const it = std::ranges::find(vector, element); it != vector.end())
        {
            // NOTE: Swap with last and pop to avoid shifting other elements.
            std::swap(vector.at(it - vector.begin()), vector.at(vector.size() - 1));
            vector.pop_back();
        }
    }

    inline unsigned char randomChar()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        return static_cast<unsigned char>(dis(gen));
    }

    inline std::string generateHEX(unsigned int const length)
    {
        std::stringstream ss;
        for (unsigned int i = 0; i < length; ++i)
        {
            auto const random_character = randomChar();
            std::stringstream hexstream;
            hexstream << std::hex << static_cast<int>(random_character);
            auto hex = hexstream.str();
            ss << (hex.length() < 2 ? '0' + hex : hex);
        }
        return ss.str();
    }

    inline std::string generateGUID()
    {
        std::string result;

        unsigned int constexpr guid_lengths[] = { 8, 4, 4, 4, 12 };

        for (unsigned int i = 0; i < 5; ++i)
        {
            result += generateHEX(guid_lengths[i]);
        }

        return result;
    }

    inline u32 murmurHash(u8 const* key, size_t const len, u32 const seed)
    {
        u32 h = seed;
        if (len > 3)
        {
            u32 const* key_x4 = reinterpret_cast<u32 const*>(key);
            size_t i = len >> 2;
            do
            {
                u32 k = *key_x4++;
                k *= 0xcc9e2d51;
                k = (k << 15) | (k >> 17);
                k *= 0x1b873593;
                h ^= k;
                h = (h << 13) | (h >> 19);
                h = h * 5 + 0xe6546b64;
            } while (--i);
            key = reinterpret_cast<u8 const*>(key_x4);
        }
        if (len & 3)
        {
            size_t i = len & 3;
            u32 k = 0;
            key = &key[i - 1];
            do
            {
                k <<= 8;
                k |= *key--;
            } while (--i);
            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;
            h ^= k;
        }
        h ^= len;
        h ^= h >> 16;
        h *= 0x85ebca6b;
        h ^= h >> 13;
        h *= 0xc2b2ae35;
        h ^= h >> 16;
        return h;
    }

    inline LPCWSTR const stringToLPCWSTR(std::string const& s)
    {
        std::wstring wsTmp(s.begin(), s.end());
        LPCWSTR result = wsTmp.c_str();
        return result;
    }

    // probably this can be replaced with some std function
    inline std::string const wstringToString(std::wstring const& s)
    {
        std::string wsTmp(s.begin(), s.end());
        return wsTmp;
    }

    inline uint32_t packTriangle(uint8_t x, uint8_t y, uint8_t z)
    {
        uint32_t packed_triangle =
            (static_cast<uint32_t>(x) << 0)
            | (static_cast<uint32_t>(y) << 8)
            | (static_cast<uint32_t>(z) << 16);
        return packed_triangle;
    }

}

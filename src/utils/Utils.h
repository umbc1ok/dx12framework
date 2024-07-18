#include <random>
#include <sstream>

#include "Types.h"

namespace olej_utils
{
    template<typename T>
    void swap_and_erase(std::vector<T>& vector, T element)
    {
        if (auto const it = std::ranges::find(vector, element); it != vector.end())
        {
            // NOTE: Swap with last and pop to avoid shifting other elements.
            std::swap(vector.at(it - vector.begin()), vector.at(vector.size() - 1));
            vector.pop_back();
        }
    }

    inline unsigned char random_char()
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        return static_cast<unsigned char>(dis(gen));
    }

    inline std::string generate_hex(unsigned int const length)
    {
        std::stringstream ss;
        for (unsigned int i = 0; i < length; ++i)
        {
            auto const random_character = random_char();
            std::stringstream hexstream;
            hexstream << std::hex << static_cast<int>(random_character);
            auto hex = hexstream.str();
            ss << (hex.length() < 2 ? '0' + hex : hex);
        }
        return ss.str();
    }

    inline std::string generate_guid()
    {
        std::string result;

        unsigned int constexpr guid_lengths[] = { 8, 4, 4, 4, 12 };

        for (unsigned int i = 0; i < 5; ++i)
        {
            result += generate_hex(guid_lengths[i]);
        }

        return result;
    }

    inline u32 murmur_hash(u8 const* key, size_t const len, u32 const seed)
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
}

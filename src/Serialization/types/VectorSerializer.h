
#include <fstream>
#include <vector>

namespace serializers
{
    template <typename T>
    void serializeVector(std::ofstream& out, const std::vector<T>& vec)
    {
        size_t size = vec.size();
        out.write(reinterpret_cast<const char*>(&size), sizeof(size));
        out.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
    }

    template <typename T>
    void deserializeVector(std::ifstream& in, std::vector<T>& vec) {
        size_t size;
        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        vec.resize(size);
        in.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
    }

    template <typename T>
    void serializeObject(std::ofstream& out, const T& obj) {
        out.write(reinterpret_cast<const char*>(&obj), sizeof(T));
    }
    template <typename T>
    void deserializeObject(std::ifstream& in, T& obj) {
        in.read(reinterpret_cast<char*>(&obj), sizeof(T));
    }

}

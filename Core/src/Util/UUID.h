#ifndef UUID_H
#define UUID_H

#include <cstdint>
#include <functional>
#include <random>
#include <ostream>

// -------------------------------------------------------------------------
// UUID — a 64-bit unique identifier.
// Value 0 is reserved as the null/invalid sentinel.
// -------------------------------------------------------------------------
class CoreUUID
{
public:
    CoreUUID() : m_Value(0) {}
    explicit CoreUUID(uint64_t value) : m_Value(value) {}

    static CoreUUID Generate()
    {
        static std::mt19937_64                        rng(std::random_device{}());
        static std::uniform_int_distribution<uint64_t> dist(1, UINT64_MAX);
        return CoreUUID(dist(rng));
    }

    bool     IsValid() const { return m_Value != 0; }
    uint64_t Value()   const { return m_Value; }

    bool operator==(const CoreUUID&) const = default;
private:
    uint64_t m_Value;
};

template<>
struct std::hash<CoreUUID>
{
    size_t operator()(const CoreUUID& id) const noexcept
    {
        return std::hash<uint64_t>{}(id.Value());
    }
};

inline std::ostream& operator<<(std::ostream& os, const CoreUUID& id)
{
    return os << id.Value();
}

#endif // UUID_H

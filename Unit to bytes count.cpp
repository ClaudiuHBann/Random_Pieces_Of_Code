#include <iostream>

using namespace std;

// B -> KB -> MB -> GB -> TB -> ...
// 0     1     2     3     4    ...
template <uint8_t TypeUnit = 0> [[nodiscard]] constexpr uint64_t to_unit(const uint64_t aValue) noexcept
{
    return !TypeUnit ? aValue : 1024 * to_unit<TypeUnit - 1>(aValue);
}

[[nodiscard]] constexpr uint64_t operator"" _B(const uint64_t aValue) noexcept
{
    return to_unit<0>(aValue);
}

[[nodiscard]] constexpr uint64_t operator"" _KB(const uint64_t aValue) noexcept
{
    return to_unit<1>(aValue);
}

[[nodiscard]] constexpr uint64_t operator"" _MB(const uint64_t aValue) noexcept
{
    return to_unit<2>(aValue);
}

[[nodiscard]] constexpr uint64_t operator"" _GB(const uint64_t aValue) noexcept
{
    return to_unit<3>(aValue);
}

[[nodiscard]] constexpr uint64_t operator"" _TB(const uint64_t aValue) noexcept
{
    return to_unit<4>(aValue);
}

int main()
{
    cout << "324_B -> " << 324_B << endl;
    cout << "22_KB -> " << 22_KB << endl;
    cout << "10_MB -> " << 10_MB << endl;
    cout << "58_GB -> " << 58_GB << endl;
    cout << "2_TB -> " << 2_TB << endl;

    return 0;
}

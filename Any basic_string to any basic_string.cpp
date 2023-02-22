#include <string>

using namespace std;

namespace impl {
    template<typename T>
    inline constexpr bool is_basic_string_v = false;

    template<typename... T>
    inline constexpr bool is_basic_string_v<basic_string<T...>> = true;
}

template<typename T>
inline constexpr bool is_basic_string_v = impl::is_basic_string_v<T>;

template<typename StringType, typename StringFrom>
inline auto ToBasicString(const StringFrom& strFrom) {
    static_assert(is_basic_string_v<StringFrom>, "The StringFrom is not a basic_string!");
    static_assert(is_arithmetic_v<StringType>, "The StringType is not a built-in type!");

    if constexpr (is_same_v<typename StringFrom::value_type, StringType>) {
        return strFrom;
    } else {
        return basic_string<StringType, char_traits<StringType>, allocator<StringType>>(strFrom.begin(), strFrom.end());
    }
}

int main() {
    wstring wstr(L"gajamkalamakajigugumuculugu");
    string str = ToBasicString<char>(wstr);
    basic_string<float, char_traits<float>, allocator<float>> wtf = ToBasicString<float>(str);
	auto wstrBack = ToBasicString<wchar_t>(wstr);

    return 0;
}

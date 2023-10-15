#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#elif defined(__linux__)
#error "Not implemented yet!"
#else
#error "Unknown OS"
#endif

#include <span>
#include <string>

class Converter
{
  public:
    [[nodiscard]] static inline std::string ToUTF8(const std::wstring &aString) noexcept
    {
        const auto requiredSize =
            WideCharToMultiByte(CP_UTF8, 0, aString.c_str(), (int)aString.size(), nullptr, 0, nullptr, nullptr);
        if (!requiredSize)
        {
            return {};
        }

        std::string stringUTF8(requiredSize, 0);
        if (!WideCharToMultiByte(CP_UTF8, 0, aString.c_str(), (int)aString.size(), stringUTF8.data(), requiredSize,
                                 nullptr, nullptr))
        {
            return {};
        }

        return stringUTF8;
    }

    [[nodiscard]] static inline std::wstring FromUTF8(const std::span<const char> &aStringUTF8) noexcept
    {
        const auto requiredSize =
            MultiByteToWideChar(CP_UTF8, 0, aStringUTF8.data(), (int)aStringUTF8.size(), nullptr, 0);
        if (!requiredSize)
        {
            return {};
        }

        std::wstring string(requiredSize, 0);
        if (!MultiByteToWideChar(CP_UTF8, 0, aStringUTF8.data(), (int)aStringUTF8.size(), string.data(), requiredSize))
        {
            return {};
        }

        return string;
    }

    [[nodiscard]] static inline auto FromUTF8(const std::string &aStringUTF8) noexcept
    {
        return FromUTF8(std::span<const char>{aStringUTF8});
    }
};

#include <iostream>

int main()
{
    std::wstring stringStart = L"Salut!";
    const auto stringEnd = Converter::FromUTF8(Converter::ToUTF8(stringStart));

    std::cout << (stringStart == stringEnd);

    return 0;
}

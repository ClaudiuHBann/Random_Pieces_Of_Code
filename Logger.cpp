// win32
#include <Windows.h>

// std
#include <functional>
#include <iostream>
#include <sstream>

#define TRACE_LOCATION_PROCESS_THREAD_ID                                                                               \
    hbann::String(TEXT("PID: ") + hbann::ToString(::GetCurrentProcessId()) + TEXT(" TID: ") +                          \
                  hbann::ToString(::GetCurrentThreadId()))
#define TRACE_LOCATION_FILE_LINE hbann::ClampFileNameLength(TEXT(__FILE__), hbann::ToString(__LINE__), 50)
#define TRACE_LOCATION                                                                                                 \
    hbann::String(TRACE_LOCATION_PROCESS_THREAD_ID + TEXT(" ") + TRACE_LOCATION_FILE_LINE + TEXT("\t"))
#define TRACE(strn) hbann::Print<const ::TCHAR *>((TRACE_LOCATION + (hbann::StringStream() << strn).str()).c_str())

namespace hbann
{
namespace detail
{
template <typename Type> constexpr bool is_basic_string_v = false;

template <typename... Type> constexpr bool is_basic_string_v<std::basic_string<Type...>> = true;
} // namespace detail

template <typename Type> constexpr bool is_basic_string_v = hbann::detail::is_basic_string_v<Type>;

template <typename Type> using remove_decayed_cvptr_t = std::remove_const_t<std::remove_pointer_t<std::decay_t<Type>>>;

#if defined(_UNICODE) || defined(UNICODE)
using String = std::wstring;
using StringStream = std::wstringstream;
#else
using String = std::string;
using StringStream = std::stringstream;
#endif // defined(_UNICODE) || defined(UNICODE)

template <typename String> [[nodiscard]] static constexpr hbann::String ToString(const String &aString) noexcept
{
    if constexpr (std::is_same_v<hbann::remove_decayed_cvptr_t<String>, ::TCHAR> ||
                  is_basic_string_v<hbann::remove_decayed_cvptr_t<String>>)
    {
        return aString;
    }
    else
    {
#if defined(_UNICODE) || defined(UNICODE)
        return std::to_wstring(aString);
#else
        return std::to_string(aString);
#endif // defined(_UNICODE) || defined(UNICODE)
    }
}

[[nodiscard]] static inline auto ClampFileNameLength(hbann::String aFile, hbann::String aLine,
                                                     const size_t aLengthMaxFileName = 69,
                                                     const size_t aLengthMaxLine = 4,
                                                     const hbann::String &aPadding = TEXT("..."))
{
    aLine += hbann::String(aLengthMaxLine - aLine.length(), TEXT(' '));
    hbann::String lineStr(TEXT(":") + aLine);

    auto fileLength = aFile.length();
    if (fileLength <= aLengthMaxFileName)
    {
        aFile += lineStr;
        aFile += hbann::String(aLengthMaxFileName - fileLength, TEXT(' '));
    }
    else
    {
        auto start = fileLength - aLengthMaxFileName + aPadding.length();
        aFile = aPadding + aFile.substr(start);
        aFile += lineStr;
    }

    return aFile;
}

static inline void OutputDebugStringForced(const ::TCHAR *aString) noexcept
{
    if (::IsDebuggerPresent())
    {
        ::OutputDebugString(aString);
    }

    if (::GetConsoleWindow())
    {
#if defined(_UNICODE) || defined(UNICODE)
        std::wcout << aString;
#else
        std::cout << aString;
#endif // defined(_UNICODE) || defined(UNICODE)
    }

    // TO DO: add a log file
}

template <typename Object, typename Range>
static constexpr void Print(
    const Range &aRange, const hbann::String &aSeparatorDimensions = TEXT("\n"),
    const std::function<void(const Object &)> &aFuncPrintElem = [](const auto &aObject) {
        OutputDebugStringForced(hbann::ToString(aObject).c_str());
        OutputDebugStringForced(TEXT(" "));
    }) noexcept
{
    if constexpr (std::ranges::range<Range>)
    {
        OutputDebugStringForced(aSeparatorDimensions.c_str());
        for (const auto &object : aRange)
        {
            Print(object, aSeparatorDimensions, aFuncPrintElem);
        }
    }
    else
    {
        aFuncPrintElem(aRange);
        OutputDebugStringForced(aSeparatorDimensions.c_str());
    }
}
} // namespace hbann

int main() {
	TRACE("wow" << TEXT(" this ") << 44);

	return 0;
}

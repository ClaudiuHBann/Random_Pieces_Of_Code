#include <Windows.h>

#include <functional>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <ranges>

namespace Utility {
	using namespace std;

	namespace impl {
		template<typename T>
		constexpr bool is_basic_string_v = false;

		template<typename... T>
		constexpr bool is_basic_string_v<basic_string<T...>> = true;
	}

	template<typename T>
	constexpr bool is_basic_string_v = impl::is_basic_string_v<T>;

#if defined(_UNICODE) || defined(UNICODE)
	using String = wstring;
	using StringStream = wstringstream;
#else
	using String = string;
	using StringStream = stringstream;
#endif // defined(_UNICODE) || defined(UNICODE)

	template <typename T>
	static inline String ToString(const T& t) {
		if constexpr (is_same_v<remove_const_t<remove_pointer_t<decay_t<T>>>, ::TCHAR> ||
					  is_same_v<remove_const_t<remove_reference_t<decay_t<T>>>, String>) {
			return t;
		} else {
#if defined(_UNICODE) || defined(UNICODE)
			return to_wstring(t);
#else
			return to_string(t);
#endif // defined(_UNICODE) || defined(UNICODE)
		}
	}

	template<typename StringType, typename StringFrom>
	static inline auto ToStringType(const StringFrom& strTo) {
		static_assert(is_basic_string_v<StringFrom>, "The StringFrom is not a basic_string!");
		static_assert(is_arithmetic_v<StringType>, "The StringType is not a built-in type!");

		if constexpr (is_same_v<typename StringFrom::value_type, StringType>) {
			return strTo;
		} else {
#pragma warning(suppress: 4244)
			return basic_string<StringType, char_traits<StringType>, allocator<StringType>>(strTo.begin(), strTo.end());
		}
	}

	static inline String ClampFileNameLength(
		String file,
		String line,
		const size_t lengthMaxFileName = 69,
		const size_t lengthMaxLine = 4,
		const String& padding = TEXT("...")
	) {
		line += String(lengthMaxLine - line.length(), TEXT(' '));
		String lineStr(TEXT(":") + line);

		auto fileLength = file.length();
		if (fileLength <= lengthMaxFileName) {
			file += lineStr;
			file += String(lengthMaxFileName - fileLength, TEXT(' '));
		} else {
			auto start = fileLength - lengthMaxFileName + padding.length();
			file = padding + file.substr(start);
			file += lineStr;
		}

		return file;
	}

	static inline void OutputDebugStringForced(const ::TCHAR* str) {
		if (::IsDebuggerPresent()) {
			::OutputDebugString(str);
		}

		if (GetConsoleWindow()) {
#if defined(_UNICODE) || defined(UNICODE)
			wcout << str;
#else
			cout << str;
#endif // defined(_UNICODE) || defined(UNICODE)
		}

		// TO DO: add a log file
	}

	template<typename Object, typename Iterable>
	static void Print(
		const Iterable& iterable,
		const String& separatorDimensions = TEXT("\n"),
		const function<void(const Object&)>& funcPrintElem = [] (const auto& obj) {
			if constexpr (is_arithmetic_v<decay_t<Object>>) {
				OutputDebugStringForced(ToString(obj).c_str());
			} else if constexpr (is_same_v<remove_const_t<remove_reference_t<decay_t<Object>>>, String>) {
				OutputDebugStringForced(obj.c_str());
			} else if constexpr (is_same_v<remove_const_t<remove_pointer_t<decay_t<Object>>>, ::TCHAR>) {
				OutputDebugStringForced(obj);
			} else {
				static_assert(false, R"(The object from the innermost range is not a built-in/(c)string type, please provide a valid print element function.)");
			}

			OutputDebugStringForced(TEXT(" "));
		}
	) {
		if constexpr (ranges::range<Iterable>) {
			OutputDebugStringForced(separatorDimensions.c_str());
			ranges::for_each(iterable, [&] (const auto& it) { Print(it, separatorDimensions, funcPrintElem); });
		} else {
			funcPrintElem(iterable);
			OutputDebugStringForced(separatorDimensions.c_str());
		}
	}
}

#define TRACE_LOCATION_PROCESS_THREAD_ID Utility::String(TEXT("PID: ") + Utility::ToString(::GetCurrentProcessId()) + TEXT(" TID: ") + Utility::ToString(::GetCurrentThreadId()))
#define TRACE_LOCATION_FILE_LINE Utility::ClampFileNameLength(TEXT(__FILE__), Utility::ToString(__LINE__), 50)
#define TRACE_LOCATION Utility::String(TRACE_LOCATION_PROCESS_THREAD_ID + TEXT(" ") + TRACE_LOCATION_FILE_LINE + TEXT("\t"))
#define TRACE(strn) Utility::Print<const ::TCHAR*>((TRACE_LOCATION + (Utility::StringStream() << strn).str()).c_str())

int main() {
	TRACE("wow" << TEXT(" this ") << 44);

	return 0;
}

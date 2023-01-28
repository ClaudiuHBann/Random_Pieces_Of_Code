#include <functional>
#include <algorithm>
#include <iostream>

using namespace std;

template<typename Object, typename Iterable, typename Stream = ostream>
void Print(
    const Iterable& iterable,
    const string& separator = "\n",
    const function<void(const Object&)>& funcPrintElem = [] (const Object& obj) {
        static_assert(is_arithmetic_v<Object>, R"(The object from the innermost range is not a built-in type, please provide a valid function.)");
        cout << obj << " ";
    },
    const Stream& stream = cout
) {
    if constexpr (ranges::range<Iterable>) {
        ranges::for_each(iterable, [&] (const auto& it) { Print(it, separator, funcPrintElem, stream); });
        cout << separator;
    } else {
        funcPrintElem(iterable);
    }
}

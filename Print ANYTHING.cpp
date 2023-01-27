#include <functional>
#include <algorithm>
#include <iostream>

using namespace std;

template<typename Object, typename Iterable>
void Print(
    const Iterable& iterable,
    const string& separatorDimension = "\n",
    const function<void(const Object&)>& funcPrintElem = [] (const auto& obj) {
        static_assert(is_arithmetic_v<Object>, R"(The object from the innermost range is not a built-in type, please provide a valid function.)");
        cout << obj << " ";
    }
) {
    if constexpr (ranges::range<Iterable>) {
        ranges::for_each(iterable, [&] (const auto& it) { Print(it, funcPrintElem); });
        cout << separatorDimension;
    } else {
        funcPrintElem(iterable);
    }
}

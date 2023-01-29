#include <functional>
#include <algorithm>
#include <iostream>

using namespace std;

template<typename Object, typename Iterable>
void Print(
    const Iterable& iterable,
    const string& separatorDimensions = "\n",
    const function<void(const Object&)>& funcPrintElem = [] (const Object& obj) {
        static_assert(
            is_arithmetic_v<Object> || is_same_v<remove_const_t<remove_pointer_t<Object>>, char>,
            R"(The object from the innermost range is not a built-in/c-string type, please provide a valid print element function.)"
            );
        cout << obj << ' ';
    }
) {
    if constexpr (ranges::range<Iterable>) {
        ranges::for_each(iterable, [&] (const auto& it) { Print(it, separatorDimensions, funcPrintElem); });
        cout << separatorDimensions;
    } else {
        funcPrintElem(iterable);
    }
}

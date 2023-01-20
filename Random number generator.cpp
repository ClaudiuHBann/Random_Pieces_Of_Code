/*
    Python inspired numpy lib number/vector/matrix one liners like:
    r = np.random.randint(0, 100)
    vector = np.random.randint(0, 100, 10)
    matrix = np.random.randint(0, 100, (10, 10))

    Recreated by https://github.com/ClaudiuHBann in C++20:
*/

#include <iostream>

#include <ranges>
#include <random>
#include <vector>

using namespace std;
using namespace ranges;
using namespace views;

class Random {
public:
    Random() : mGenerator(mSeeder()) {}

    template <typename Type = int>
    inline auto GetInt(
        const Type min = numeric_limits<Type>::min(),
        const Type max = numeric_limits<Type>::max()
    ) {
        return uniform_int_distribution<Type>(min, max)(mGenerator);
    }

    template <typename Type = float>
    inline auto GetReal(
        const Type min = numeric_limits<Type>::min(),
        const Type max = numeric_limits<Type>::max()
    ) {
        return uniform_real_distribution<Type>(min, max)(mGenerator);
    }

    // https://stackoverflow.com/questions/48199813/how-to-use-condition-to-check-if-typename-t-is-integer-type-of-float-type-in-c second
    template <typename Type>
    inline auto Get(
        const Type min = numeric_limits<Type>::min(),
        const Type max = numeric_limits<Type>::max()
    ) {
        if constexpr (is_integral_v<Type>) {
            return GetInt(min, max);
        } else if constexpr (is_floating_point_v<Type>) {
            return GetReal(min, max);
        } else {
            return {};
        }
    }

    template <typename Type>
    inline auto GetVector(
        const size_t size,
        const Type min = numeric_limits<Type>::min(),
        const Type max = numeric_limits<Type>::max()
    ) {
        return iota(0U, size) |
            views::transform([=] (auto _) { return Get(min, max); });
    }

    template <typename Type>
    inline auto GetMatrix(
        const size_t rows,
        const size_t columns = 1U,
        const Type min = numeric_limits<Type>::min(),
        const Type max = numeric_limits<Type>::max()
    ) {
        return iota(0U, rows) |
            views::transform([=] (auto _) { return GetVector(columns, min, max); });
    }

    //protected:
        // https://isocpp.org/wiki/faq/pointers-to-members
        // can be used with any method that generates random numbers
        // method's definition must be T func(T min, T max)
    template <typename Type, typename Func>
    inline auto GetVectorS(
        Type(Func::* func)(const Type, const Type),
        const size_t size,
        const Type min = numeric_limits<Type>::min(),
        const Type max = numeric_limits<Type>::max()
    ) {
        return views::iota(0U, size) |
            views::transform(
                [=] (auto _) {
                    return (this->*func)(min, max);
                }
        );
    }

    // can be used with any method that generates random numbers
    // method's definition must be T func(T min, T max)
    template <typename Type, typename Func>
    inline auto GetMatrixS(
        Func func,
        const size_t rows,
        const size_t columns = 1U,
        const Type min = numeric_limits<Type>::min(),
        const Type max = numeric_limits<Type>::max()
    ) {
        return views::iota(0U, rows) |
            views::transform([=] (auto _) { return GetVectorS(func, columns, min, max); });
    }

private:
    random_device mSeeder;
    mt19937 mGenerator;
};

int main() {
    Random random;

    {
        auto integer = random.GetInt();
        auto real = random.GetReal(0.f, 69.f);
        auto vector = random.GetVector(2, 0, 100);
        auto matrix = random.GetMatrix(2, 2, 0.f, 100.f);

        cout << "GetInt() = " << integer << endl;
        cout << "GetReal(0.f, 69.f) = " << real << endl << endl;

        cout << "GetVector(2, 0, 100):" << endl;
        for (const auto& item : vector) {
            cout << item << " ";
        }
        cout << endl << endl;

        cout << "GetMatrix(2, 2, 0.f, 100.f):" << endl;
        for (const auto& row : matrix) {
            for (const auto& column : row) {
                cout << column << " ";
            }
            cout << endl;
        }
        cout << endl << endl;
    }

    {
        auto integer = random.Get(-100, 100);
        auto real = random.Get<double>();
        auto vector = random.GetVectorS(&Random::GetReal<float>, 2, 0.f, 100.f);
        auto matrix = random.GetMatrixS(&Random::GetInt<int>, 2, 2, 0, 100);

        cout << "Get<int>(-100, 100) = " << integer << endl;
        cout << "Get<float>() = " << real << endl << endl;

        cout << "GetVectorS(&Random::GetReal<float>, 2, 0.f, 100.f):" << endl;
        for (const auto& item : vector) {
            cout << item << " ";
        }
        cout << endl << endl;

        cout << "GetMatrixS(&Random::GetInt<int>, 2, 2, 0, 100):" << endl;
        for (const auto& row : matrix) {
            for (const auto& column : row) {
                cout << column << " ";
            }
            cout << endl;
        }
    }

    return 0;
}

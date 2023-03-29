/*
    Python inspired numpy lib number/vector/matrix one liners like:
    r = np.random.randint(0, 100)
    vector = np.random.randint(0, 100, 10)
    matrix = np.random.randint(0, 100, (10, 10))

    Recreated by https://github.com/ClaudiuHBann in C++20:
*/

#include <iostream>

#include <random>
#include <ranges>
#include <vector>

using namespace std;
using namespace ranges;
using namespace views;

template <typename Type>
concept Arithmetical = is_arithmetic_v<Type>;

class Random
{
  public:
    Random() : mGenerator(mSeeder())
    {
    }

    template <Arithmetical Type,
              typename UniformDistribution =
                  conditional_t<is_integral_v<Type>, uniform_int_distribution<Type>, uniform_real_distribution<Type>>>
    inline auto Get(const Type min = numeric_limits<Type>::min(), const Type max = numeric_limits<Type>::max())
    {
        return UniformDistribution(min, max)(mGenerator);
    }

    template <Arithmetical Type>
    inline auto GetVector(const size_t size, const Type min = numeric_limits<Type>::min(),
                          const Type max = numeric_limits<Type>::max())
    {
        return iota(0U, size) | views::transform([=](auto _) { return Get(min, max); });
    }

    template <Arithmetical Type>
    inline auto GetMatrix(const size_t rows, const size_t columns = 1U, const Type min = numeric_limits<Type>::min(),
                          const Type max = numeric_limits<Type>::max())
    {
        return iota(0U, rows) | views::transform([=](auto _) { return GetVector(columns, min, max); });
    }

    template <Arithmetical Type, typename Func>
    inline auto GetVectorS(Type (Func::*func)(const Type, const Type), const size_t size,
                           const Type min = numeric_limits<Type>::min(), const Type max = numeric_limits<Type>::max())
    {
        return views::iota(0U, size) | views::transform([=](auto _) { return (this->*func)(min, max); });
    }

    template <Arithmetical Type, typename Func>
    inline auto GetMatrixS(Func func, const size_t rows, const size_t columns = 1U,
                           const Type min = numeric_limits<Type>::min(), const Type max = numeric_limits<Type>::max())
    {
        return views::iota(0U, rows) | views::transform([=](auto _) { return GetVectorS(func, columns, min, max); });
    }

  private:
    random_device mSeeder;
    mt19937 mGenerator;
};

int main()
{
    Random random;

    {
        auto integer = random.Get<int>();
        auto real = random.Get<float>(0.f, 69.f);
        auto vector = random.GetVector(2, 0, 100);
        auto matrix = random.GetMatrix(2, 2, 0.f, 100.f);

        cout << "Get<int>() = " << integer << endl;
        cout << "Get<float>(0.f, 69.f) = " << real << endl << endl;

        cout << "GetVector(2, 0, 100):" << endl;
        for (const auto &item : vector)
        {
            cout << item << " ";
        }
        cout << endl << endl;

        cout << "GetMatrix(2, 2, 0.f, 100.f):" << endl;
        for (const auto &row : matrix)
        {
            for (const auto &column : row)
            {
                cout << column << " ";
            }
            cout << endl;
        }
        cout << endl << endl;
    }

    {
        auto integer = random.Get(-100, 100);
        auto real = random.Get<double>();
        auto vector = random.GetVectorS(&Random::Get<float>, 2, 0.f, 100.f);
        auto matrix = random.GetMatrixS(&Random::Get<int>, 2, 2, 0, 100);

        cout << "Get<int>(-100, 100) = " << integer << endl;
        cout << "Get<float>() = " << real << endl << endl;

        cout << "GetVectorS(&Random::Get<float>, 2, 0.f, 100.f):" << endl;
        for (const auto &item : vector)
        {
            cout << item << " ";
        }
        cout << endl << endl;

        cout << "GetMatrixS(&Random::Get<int>, 2, 2, 0, 100):" << endl;
        for (const auto &row : matrix)
        {
            for (const auto &column : row)
            {
                cout << column << " ";
            }
            cout << endl;
        }
    }

    return 0;
}

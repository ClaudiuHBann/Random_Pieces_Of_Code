#include <Windows.h>

#include <vector>
#include <functional>

using namespace std;

class Accelerator
{
  public:
    inline void Register(const vector<uint8_t> &keys, const function<void()> &callback)
    {
        mAccelerators.push_back({keys, callback, false});
    }

    void Check()
    {
        for (auto &[keys, callback, state] : mAccelerators)
        {
            bool fire = true;
            for (const auto &key : keys)
            {
                if (GetAsyncKeyState(key) >= 0)
                {
                    fire = false;
                    break;
                }
            }

            if (!state && fire)
            {
                callback();
            }
            state = fire;
        }
    }

  private:
    vector<tuple<vector<uint8_t>, function<void()>, bool>> mAccelerators{};
};

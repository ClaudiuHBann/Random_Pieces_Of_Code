#include <Windows.h>

#include <vector>
#include <functional>

using namespace std;

class Accelerator {
public:
	inline void Register(const vector<uint8_t>& keys, const function<void()>& callback) {
		mAccelerators.push_back({ keys, callback, false });
	}

	void Check() {
		for (auto& accelerator : mAccelerators) {
			bool fire = true;
			for (const auto& key : get<0>(accelerator)) {
				if (GetAsyncKeyState(key) >= 0) {
					fire = false;
					break;
				}
			}

			if (!get<2>(accelerator) && fire) {
				get<1>(accelerator)();
			}
			get<2>(accelerator) = fire;
		}
	}

private:
	vector<tuple<vector<uint8_t>, function<void()>, bool>> mAccelerators {};
};

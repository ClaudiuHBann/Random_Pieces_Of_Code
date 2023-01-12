#include <map>
#include <string>

#include <chrono>

using namespace std;
using namespace chrono;

class Stopwatch {
public:
    inline Stopwatch() = default;

    inline Stopwatch(const string& name, const bool init = false) {
        Create(name, init);
    }

    inline uint64_t Size() const {
        return mStopwatches.size();
    }

    inline bool Create(const string& name, const bool init = false, const bool overwrite = false) {
        if (!name.empty() && ((mStopwatches.find(name) != mStopwatches.end() && overwrite) || mStopwatches.find(name) == mStopwatches.end())) {
            mStopwatches[name] = init ? steady_clock::now() : steady_clock::time_point();
        }

        return !name.empty();
    }

    inline bool Remove(const string& name) {
        return mStopwatches.erase(name);
    }

    inline bool Reset(const string& name) {
        if (mStopwatches.find(name) != mStopwatches.end()) {
            mStopwatches[name] = steady_clock::now();
            return true;
        }

        return false;
    }

    inline duration<long long, nano> GetTimeElapsed(const string& name) {
        duration<long long, nano> timePoint {};

        if (mStopwatches.find(name) != mStopwatches.end()) {
            timePoint = steady_clock::now() - mStopwatches[name];
        }

        return timePoint;
    }

protected:
    map<string, steady_clock::time_point> mStopwatches;
};

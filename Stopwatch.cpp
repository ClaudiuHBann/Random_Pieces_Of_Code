#include <iostream>
#include <chrono>
#include <string>
#include <mutex>
#include <map>

using namespace std;
using namespace chrono;

class Stopwatch {
public:
    inline Stopwatch() = default;

    inline Stopwatch(const string& name, const bool start = false) {
        Create(name, start);
    }

    inline size_t Size() {
        scoped_lock guard(mMutex);
        return mStopwatches.size();
    }

    inline bool Create(const string& name, const bool start = false, const bool overwrite = false) {
        if (name.empty()) {
            return false;
        }

        scoped_lock guard(mMutex);
        if (mStopwatches.find(name) != mStopwatches.end() && !overwrite) {
            return false;
        }

        mStopwatches[name] = { start ? steady_clock::now() : steady_clock::time_point(), start };
        return true;
    }

    inline bool Pause(const string& name) {
        scoped_lock guard(mMutex);
        if (mStopwatches.find(name) != mStopwatches.end()) {
            // a paused stopwatch will contain the elapsed time
            mStopwatches[name] = { steady_clock::time_point(GetTimeElapsed(name)), false };
            return true;
        }

        return false;
    }

    inline bool Resume(const string& name) {
        scoped_lock guard(mMutex);
        if (mStopwatches.find(name) != mStopwatches.end() && !mStopwatches[name].second) {
            // a paused stopwatch contains the elapsed time
            // so we need to take the current time and subtract from current moment
            steady_clock::time_point nowBackwards(steady_clock::now() - mStopwatches[name].first);
            mStopwatches[name] = { nowBackwards, true };
            return true;
        }

        return false;
    }

    inline bool Reset(const string& name, const bool start = true) {
        return Create(name, start, true);
    }

    inline bool Remove(const string& name) {
        scoped_lock guard(mMutex);
        return mStopwatches.erase(name);
    }

    inline nanoseconds GetTimeElapsed(const string& name) {
        scoped_lock guard(mMutex);
        if (mStopwatches.find(name) != mStopwatches.end()) {
            if (mStopwatches[name].second) {
                return steady_clock::now() - mStopwatches[name].first;
            } else {
                // a paused stopwatch already contains the elapsed time
                return mStopwatches[name].first.time_since_epoch();
            }
        }

        return {};
    }

protected:
    map<string, pair<steady_clock::time_point, bool>> mStopwatches;
    //  name         time now/elapsed          running

private:
    mutex mMutex;
};

int main() {
    Stopwatch stopwatch("all", true);
    cout << R"(Created stopwatch "all"...)" << endl;

    stopwatch.Create("test", true);
    cout << R"(Created stopwatch "test"...)" << endl;
    cout << "Waiting..." << endl;
    cout << R"(Stopwatch "test" time elapsed is )" << stopwatch.GetTimeElapsed("test") << endl;

    cout << R"(Stopwatch "all" time elapsed is )" << stopwatch.GetTimeElapsed("all") << endl;

    return 0;
}

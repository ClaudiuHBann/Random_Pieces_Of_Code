#include <iostream>

#include <any>
#include <functional>
#include <mutex>
#include <thread>

class Thread
{
  public:
    enum class Priority : uint8_t
    {
        LOW,    // last
        MEDIUM, // middle
        HIGH    // first
    };

    // TODO: I think any deep copies the data, maybe void*
    struct Task
    {
        //              result, context
        std::function<std::any(std::any)> work;

        //                   result, context
        std::function<void(std::any, std::any)> callback;

        std::any context;
    };

    Thread(const bool aStart = false)
    {
        if (aStart)
        {
            Start();
        }
    }

    bool Add(Task &&aTask, const Priority aPriority = Priority::LOW)
    {
        std::scoped_lock lock(mMutexTasks);

        switch (aPriority)
        {
        case Priority::LOW:
            mTasks.emplace_back(std::move(aTask));
            return true;

        case Priority::MEDIUM: {
            const auto index = static_cast<ptrdiff_t>(mTasks.size() / 2.);
            const auto it = std::next(mTasks.begin(), index);

            mTasks.emplace(it, std::move(aTask));
            return true;
        }

        case Priority::HIGH:
            mTasks.emplace_front(std::move(aTask));
            return true;

        default:
            return false;
        }
    }

    size_t TaskCount()
    {
        std::scoped_lock lock(mMutexTasks);
        return mTasks.size();
    }

    bool HasWork()
    {
        std::scoped_lock lock(mMutexTasks);
        return TaskCount();
    }

    void Start()
    {
        mRunning = true;

        mThread = std::thread(std::bind(&Thread::Run, this));
    }

    void Stop()
    {
        mRunning = false;

        if (mThread.joinable())
        {
            mThread.join();
        }
    }

    ~Thread()
    {
        Stop();
    }

  private:
    std::recursive_mutex mMutexTasks{};
    std::list<Task> mTasks{};

    std::atomic_bool mRunning{};
    std::thread mThread{};

    void Run()
    {
        while (mRunning)
        {
            // wait for work
            while (mRunning && !HasWork())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }

            // lock and check again
            std::unique_lock lock(mMutexTasks);
            if (!mRunning || !HasWork())
            {
                // no more work, go wait again
                lock.unlock();
                continue;
            }

            // get the task and unlock
            Task task(std::move(mTasks.front()));
            mTasks.pop_front();

            lock.unlock();

            // work
            task.callback(task.work(task.context), task.context);
        }
    }
};

class ThreadPool
{
  public:
    struct Options
    {
        Thread::Priority priority = Thread::Priority::LOW;
        uint8_t threadIndex = 0; // 0 == most free thread
    };

    ThreadPool(const bool aStart = false, const uint8_t aThreadsCount = 2) : mThreads(aThreadsCount)
    {
        if (aStart)
        {
            Start();
        }
    }

    bool Add(Thread::Task &&aTask, const Options &aOptions = {})
    {
        if (aOptions.threadIndex > mThreads.size())
        {
            return false;
        }

        if (aOptions.threadIndex)
        {
            return mThreads[aOptions.threadIndex - 1].Add(std::move(aTask), aOptions.priority);
        }
        else
        {
            return ChooseThread().Add(std::move(aTask), aOptions.priority);
        }
    }

    void Start()
    {
        for (auto &thread : mThreads)
        {
            thread.Start();
        }
    }

    void Stop()
    {
        for (auto &thread : mThreads)
        {
            thread.Stop();
        }
    }

    ~ThreadPool()
    {
        Stop();
    }

  private:
    std::vector<Thread> mThreads{};

    Thread &ChooseThread()
    {
        size_t index{};
        for (size_t i = 1; i < mThreads.size(); i++)
        {
            if (mThreads[index].TaskCount() > mThreads[i].TaskCount())
            {
                index = i;
            }
        }

        return mThreads[index];
    }
};

std::any Work(std::any aContext)
{
    std::cout << "start task no. " << std::any_cast<size_t>(aContext) << " on thread " << std::this_thread::get_id()
              << std::endl;
    return true;
}

void Callback(std::any aResult, std::any aContext)
{
    std::cout << "end task no. " << std::any_cast<size_t>(aContext) << " on thread " << std::this_thread::get_id()
              << " with " << std::any_cast<bool>(aResult) << std::endl;
}

int main()
{
    ThreadPool tp(true);

    for (size_t i = 0; i < 100; i++)
    {
        tp.Add({std::bind(&Work, i), Callback, i}, {Thread::Priority::MEDIUM, 0});
    }

    std::cin.get();

    return 0;
}

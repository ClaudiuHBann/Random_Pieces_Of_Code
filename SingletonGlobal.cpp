#include <Windows.h>

#include <cassert>
#include <functional>
#include <mutex>

class UnMoveable
{
  protected:
    constexpr UnMoveable() noexcept = default;
    constexpr virtual ~UnMoveable() noexcept = default;

  private:
    constexpr UnMoveable(UnMoveable &&) noexcept = delete;
    constexpr UnMoveable &operator=(UnMoveable &&) noexcept = delete;
};

class UnCopyable
{
  protected:
    constexpr UnCopyable() noexcept = default;
    constexpr virtual ~UnCopyable() noexcept = default;

  private:
    constexpr UnCopyable(const UnCopyable &) noexcept = delete;
    constexpr UnCopyable &operator=(const UnCopyable &) noexcept = delete;
};

template <typename Type> class ContainerLazyPtrRaw final : public UnCopyable, public UnMoveable
{
    static_assert(!std::is_pointer_v<Type>, "The Type cannot be a raw pointer itself!");

  public:
    constexpr ContainerLazyPtrRaw() noexcept = default;

    ~ContainerLazyPtrRaw()
    {
        if (mCallbackUninitialize)
        {
            mCallbackUninitialize();
        }
    }

    void SetUninitialize(std::move_only_function<void()> aCallback) noexcept
    {
        mCallbackUninitialize = std::move(aCallback);
    }

    void SetPtr(Type *aPtr) noexcept
    {
        mPtr = aPtr;
    }

    Type *GetPtr() const noexcept
    {
        return mPtr;
    }

  private:
    Type *mPtr{};

    std::move_only_function<void()> mCallbackUninitialize;
};

class SharedMemory final
{
  public:
    constexpr SharedMemory() noexcept = default;
    constexpr ~SharedMemory() noexcept = default;

    // If the name matches an existing mapping object, the function returns that mapped object
    void *Create(const uint32_t aSize, std::string_view aName)
    {
        if (mHandle)
        {
            return mMemory;
        }

        mHandle = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, aSize, aName.data());
        if (!mHandle)
        {
            throw std::runtime_error("mHandle cannot be null!");
        }

        mMemory = MapViewOfFile(mHandle, SECTION_MAP_WRITE | SECTION_MAP_READ, 0, 0, 0);
        if (!mMemory)
        {
            throw std::runtime_error("mMemory cannot be null!");
        }

        return mMemory;
    }

    template <typename Type> Type GetMemory(const size_t aOffset = 0) const noexcept
    {
        assert(mMemory);
        uint8_t *memoryAsBytes = reinterpret_cast<uint8_t *>(mMemory);
        return reinterpret_cast<Type>(memoryAsBytes + aOffset);
    }

    void Delete() noexcept
    {
        if (mMemory)
        {
            UnmapViewOfFile(mMemory);
            mMemory = nullptr;
        }

        if (mHandle)
        {
            CloseHandle(mHandle);
            mHandle = nullptr;
        }
    }

  private:
    void *mMemory{};
    void *mHandle{};
};

class SharedMutex final
{
  public:
    constexpr SharedMutex() noexcept = default;
    constexpr ~SharedMutex() noexcept = default;

    // If the name matches an existing mutex, the function returns that mutex
    void *Create(std::string_view aName)
    {
        if (mMutex)
        {
            return mMutex;
        }

        mMutex = CreateMutexA(nullptr, FALSE, aName.data());
        if (!mMutex)
        {
            throw std::runtime_error("mMutex cannot be null!");
        }

        return mMutex;
    }

    void lock()
    {
        if (!mMutex)
        {
            throw std::runtime_error("mMutex cannot be null!");
        }

        if (mLocked)
        {
            throw std::runtime_error("Could not lock mutex twice!");
        }

        mLocked = WaitForSingleObject(mMutex, INFINITE) == WAIT_OBJECT_0;
        if (!mLocked)
        {
            throw std::runtime_error("Could not lock mutex!");
        }
    }

    void unlock() noexcept
    {
        if (!mMutex)
        {
            return;
        }

        assert(mLocked);
        mLocked = !ReleaseMutex(mMutex);
    }

    void Delete() noexcept
    {
        if (mMutex)
        {
            CloseHandle(mMutex);
            mMutex = nullptr;
        }
    }

  private:
    void *mMutex{};
    bool mLocked{};
};

// A Per Process Singleton that uses shared memory
//
// The shared memory layout is:
//  2 bytes  +  the size of the object
// ref count      the object itself
//
// The File Mapping has the name format: SingletonGlobal<typeid(Type).hash_code()>
template <typename Type> class SingletonGlobal : public UnCopyable, public UnMoveable
{
    using RefCountType = uint16_t;

  public:
    constexpr SingletonGlobal() noexcept = default;
    constexpr virtual ~SingletonGlobal() noexcept = default;

    static Type &GetInstance()
    {
        const auto sharedMutexName = std::format("SingletonGlobal<{}>::Mutex", typeid(Type).hash_code());
        mSharedMutex.Create(sharedMutexName);

        return CreateInstance();
    }

  private:
    static inline ContainerLazyPtrRaw<Type> mInstance;
    static inline SharedMutex mSharedMutex;

    static inline SharedMemory mSharedMemory;

    static RefCountType &GetCount() noexcept
    {
        return *mSharedMemory.GetMemory<RefCountType *>();
    }

    static Type *GetObjectPtr() noexcept
    {
        return mSharedMemory.GetMemory<Type *>(sizeof(RefCountType));
    }

    static Type &CreateInstance()
    {
        std::scoped_lock lock(mSharedMutex);

        if (mInstance.GetPtr())
        {
            return *mInstance.GetPtr();
        }

        return Initialize();
    }

    static void DeleteInstance()
    {
        std::scoped_lock lock(mSharedMutex);

        if (!mInstance.GetPtr())
        {
            return;
        }

        Uninitialize();
    }

    static Type &Initialize()
    {
        // contains the instance count and the object itself
        const uint32_t sharedMemorySize = sizeof(RefCountType) + sizeof(Type);
        const auto sharedMemoryName = std::format("SingletonGlobal<{}>::Memory", typeid(Type).hash_code());

        mSharedMemory.Create(sharedMemorySize, sharedMemoryName);

        if (GetCount()++)
        {
            // the object was already created
            mInstance.SetPtr(GetObjectPtr());
        }
        else
        {
            // use the placement new operator to create our object
            // inside the shared memory we allocated
            mInstance.SetPtr(new (GetObjectPtr()) Type());
        }

        // when this instance of the class will deallocate
        // it will call this callback to delete the instance
        mInstance.SetUninitialize(DeleteInstance);

        return *mInstance.GetPtr();
    }

    static void Uninitialize()
    {
        if (--GetCount())
        {
            return;
        }

        // we cannot call the delete operator on this object
        // in conjunction with the placement new operator
        // therefore we call it's dtor explicitly
        mInstance.GetPtr()->~Type();

        mSharedMemory.Delete();
        mSharedMutex.Delete();
    }
};

int main()
{
    return 0;
}

#include "pch.h"

#ifdef WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include <Windows.h>

#else
#error "Not implemented!"
#endif // WIN32

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

    constexpr ~SharedMemory() noexcept
    {
        Delete();
    }

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

    template <typename Type> Type GetMemory(const size_t aOffset = 0) const noexcept
    {
        assert(mMemory);
        uint8_t *memoryAsBytes = reinterpret_cast<uint8_t *>(mMemory);
        return reinterpret_cast<Type>(memoryAsBytes + aOffset);
    }

  private:
    void *mMemory{};
    void *mHandle{};
};

class SharedMutex final
{
  public:
    constexpr SharedMutex() noexcept = default;

    constexpr ~SharedMutex() noexcept
    {
        Delete();
    }

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

    void Delete() noexcept
    {
        if (mMutex)
        {
            CloseHandle(mMutex);
            mMutex = nullptr;
        }
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

  private:
    void *mMutex{};
    bool mLocked{};
};

// A Global/Per-Process Singleton that uses shared memory and named mutexes
//
// !!! WARNING !!!
//
// Per-Process SingletonGlobalLazy does not support shared properties by default
// To share properties between processes they MUST be on "stack" so
// SingletonGlobalLazy will actually allocate them on the shared memory
//
// !!! INFORMATION !!!
//
// The shared memory layout is:
//  2 bytes  +  the size of the object
// ref count      the object itself
//
// The File Mapping has the name format: SingletonGlobalLazy<Type Hash>([PID])::Memory
// The Named Mutex has the name format: SingletonGlobalLazy<Type Hash>([PID])::Mutex
template <typename Type, bool global = false> class SingletonGlobalLazy : public UnCopyable, public UnMoveable
{
    using RefCountType = int16_t;

  public:
    constexpr SingletonGlobalLazy() noexcept = default;
    constexpr virtual ~SingletonGlobalLazy() noexcept = default;

    [[nodiscard]] static Type &GetInstance()
    {
        mSharedMutex.Create(MakeNameForMutex());
        return CreateInstance();
    }

    // Event that is called when the first instance is created
    virtual void OnInitialize([[maybe_unused]] Type &instance)
    {
    }

    // Event that is called for every instance BUT the last one when they are destroyed
    virtual void OnPerInstanceUninitialize([[maybe_unused]] Type &instance)
    {
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

  private:
    static inline SharedMutex mSharedMutex;
    static inline SharedMemory mSharedMemory;

    static inline ContainerLazyPtrRaw<Type> mInstance;

    static RefCountType &GetCount() noexcept
    {
        return *mSharedMemory.GetMemory<RefCountType *>();
    }

    static Type *GetObjectPtr() noexcept
    {
        return mSharedMemory.GetMemory<Type *>(sizeof(RefCountType));
    }

    [[nodiscard]] static Type &CreateInstance()
    {
        std::scoped_lock lock(mSharedMutex);

        if (mInstance.GetPtr())
        {
            return *mInstance.GetPtr();
        }

        return Initialize();
    }

    [[nodiscard]] static Type &Initialize()
    {
        // contains the instance count and the object itself
        const uint32_t sharedMemorySize = sizeof(RefCountType) + sizeof(Type);

        mSharedMemory.Create(sharedMemorySize, MakeNameForMemory());

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
            mInstance.GetPtr()->OnInitialize(*GetObjectPtr());
        }

        // when this instance of the class will deallocate
        // it will call this callback to delete the instance
        mInstance.SetUninitialize(DeleteInstance);

        return *mInstance.GetPtr();
    }

    static void Uninitialize()
    {
        const auto currentCount = --GetCount();
        if (currentCount < 0)
        {
            throw std::runtime_error("Ref count can't be negative!");
        }

        // we just DeleteInstance so clear callback for DeleteInstance
        mInstance.SetUninitialize(nullptr);

        if (currentCount)
        {
            mInstance.GetPtr()->OnPerInstanceUninitialize(*GetObjectPtr());
            mInstance.SetPtr(nullptr);

            return;
        }

        // we cannot call the delete operator on this object
        // in conjunction with the placement new operator
        // therefore we call it's dtor explicitly
        mInstance.GetPtr()->~Type();
        mInstance.SetPtr(nullptr);

        mSharedMemory.Delete();
        mSharedMutex.Delete();
    }

    [[nodiscard]] static std::string MakeNameForMemory() noexcept
    {
        if constexpr (global)
        {
            return std::format("SingletonGlobal<{}>()::Memory", typeid(Type).hash_code());
        }
        else
        {
            return std::format("SingletonGlobal<{}>({})::Memory", typeid(Type).hash_code(), _getpid());
        }
    }

    [[nodiscard]] static std::string MakeNameForMutex() noexcept
    {
        if constexpr (global)
        {
            return std::format("SingletonGlobal<{}>()::Mutex", typeid(Type).hash_code());
        }
        else
        {
            return std::format("SingletonGlobal<{}>({})::Mutex", typeid(Type).hash_code(), _getpid());
        }
    }
};

int main()
{
    return 0;
}

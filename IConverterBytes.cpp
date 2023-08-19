#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
using namespace filesystem;

using bytes = vector<uint8_t>;

#ifndef GUID_DEFINED
#define GUID_DEFINED
#if defined(__midl)
typedef struct
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    byte Data4[8];
} GUID;
#else
typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
#endif
#endif

namespace impl
{
template <typename Type> constexpr auto is_basic_string_v = false;

template <typename... Type> constexpr auto is_basic_string_v<basic_string<Type...>> = true;
} // namespace impl

template <typename Type> constexpr auto is_basic_string_v = impl::is_basic_string_v<Type>;

class IConverterBytes
{
    // composed by data size + data and repeat
    // ex: 16 (as bytes) + GUID (as bytes) + ...
    // the size will always be a uint32_t
    shared_ptr<bytes> mBytes{};

  public:
    using type_size = uint32_t;
    using type = decltype(mBytes);
    using type_rvalue = add_rvalue_reference_t<type>;
    using type_element = type::element_type;
    using type_value = type_element::value_type;

    constexpr IConverterBytes() noexcept = default; // in use with ToStream
    constexpr virtual ~IConverterBytes() noexcept = default;

    // Derived class should use Read... -> Clear in this constructor
    IConverterBytes(type_element &&aBytes) : mBytes(make_shared<type_element>(move(aBytes)))
    {
    }

    // Derived class should use Create -> Write -> Release in this method
    virtual type_rvalue ToStream() = 0;

  protected:
    template <typename... Types> void Create(const Types &...aObjs)
    {
        _Create(GetObjsSizeInBytes(aObjs...), sizeof...(aObjs));
    }

    template <typename Type = void *> void Write(const Type &aObj, const type_size aSize = 0)
    {
        if constexpr (is_basic_string_v<Type>)
        {
            _Write(aObj.data(), type_size(aObj.size() * sizeof(Type::value_type)));
        }
        else if constexpr (is_same_v<Type, path>)
        {
            Write(aObj.wstring());
        }
        else if constexpr (is_arithmetic_v<Type> || is_enum_v<Type> || is_standard_layout_v<Type>)
        {
            _Write(&aObj, sizeof(Type));
        }
        else
        {
            if (!aSize)
            {
                throw runtime_error("Tried to write 0 bytes!");
            }

            _Write(&aObj, aSize);
        }
    }

    template <typename Type, typename... Types> void WriteAll(const Type &aObj, const Types &...aObjs)
    {
        Write(aObj);

        if constexpr (sizeof...(aObjs))
        {
            WriteAll(aObjs...);
        }
    }

    template <typename Type = span<type_value>> decltype(auto) Read()
    {
        if constexpr (is_basic_string_v<Type>)
        {
            const auto &sp = Read<>();
            const auto ptrType = reinterpret_cast<Type::value_type *>(sp.data());
            const auto ptrSize = sp.size() / sizeof(Type::value_type);

            return Type(ptrType, ptrSize);
        }
        else if constexpr (is_same_v<Type, path>)
        {
            const auto &sp = Read<>();
            const auto ptrType = reinterpret_cast<path::value_type *>(sp.data());
            const auto ptrSize = sp.size() / sizeof(Type::value_type);

            return wstring(ptrType, ptrSize);
        }
        else if constexpr (is_arithmetic_v<Type> || is_enum_v<Type> ||
                           // is_standard_layout_v is true for span but we want the last branch for spans so:
                           (is_standard_layout_v<Type> && !is_same_v<Type, span<type_value>>))
        {
            return *reinterpret_cast<Type *>(Read().data());
        }
        else
        {
            return Read();
        }
    }

    template <typename Type, typename... Types> void ReadAll(Type &aObj, Types &...aObjs)
    {
        aObj = Read<Type>();

        if constexpr (sizeof...(aObjs))
        {
            ReadAll(aObjs...);
        }
    }

    constexpr decltype(auto) Release() noexcept
    {
        return move(mBytes);
    }

    void Clear(const bool aExplicit = true) noexcept
    {
        if (aExplicit)
        {
            mBytes->clear();
        }

        mBytes.reset();
    }

  private:
    size_t mIndex{};

    // for GetObjsSizeInBytes(aObjs...) where the pack expansion is null
    size_t GetObjsSizeInBytes()
    {
        return 0;
    }

    template <typename Type, typename... Types> size_t GetObjsSizeInBytes(const Type &aObj, const Types &...aObjs)
    {
        if constexpr (is_basic_string_v<Type>)
        {
            return aObj.size() * sizeof(Type::value_type) + GetObjsSizeInBytes(aObjs...);
        }
        else if constexpr (is_same_v<Type, path>)
        {
            return aObj.wstring().size() * sizeof(Type::value_type) + GetObjsSizeInBytes(aObjs...);
        }
        else if constexpr (is_arithmetic_v<Type> || is_enum_v<Type> || is_standard_layout_v<Type>)
        {
            return sizeof(Type) + GetObjsSizeInBytes(aObjs...);
        }
        else
        {
            // static_assert doesn't work here
            throw runtime_error("Could not calculate the size in bytes of this type!");
        }
    }

    void _Create(const size_t aSize, const size_t aCount)
    {
        mBytes = make_shared<type_element>(aSize + aCount * sizeof(type_size));
        mIndex = {};
    }

    void _Write(const void *aBytes, const type_size aSize)
    {
        if (!mBytes)
        {
            throw runtime_error("The stream was released or wasn't created!");
        }

        if (mIndex + sizeof(type_size) > mBytes->size() || mIndex + sizeof(type_size) + aSize > mBytes->size())
        {
            throw runtime_error("Tried to write bytes outside the stream!");
        }

        memcpy(mBytes->data() + mIndex, &aSize, sizeof(type_size)); // write the size of aBytes
        mIndex += sizeof(type_size);
        memcpy(mBytes->data() + mIndex, aBytes, aSize); // write aBytes after
        mIndex += aSize;
    }

    type_size ReadSize()
    {
        if (!mBytes)
        {
            throw runtime_error("The stream was released or wasn't created!");
        }

        if (mIndex + sizeof(type_size) > mBytes->size())
        {
            throw runtime_error("Tried to read bytes outside the stream!");
        }

        auto size = *reinterpret_cast<type_size *>(mBytes->data() + mIndex);
        mIndex += sizeof(type_size);
        return size;
    }

    span<type_value> Read()
    {
        auto size = ReadSize();
        if (mIndex + size > mBytes->size())
        {
            throw runtime_error("Tried to read bytes outside the stream!");
        }

        auto sp = span<type_value>(mBytes->data() + mIndex, size);
        mIndex += size;
        return sp;
    }
};

class Person : public IConverterBytes
{
  public:
    enum class Type : uint8_t
    {
        UNKNOWN,
        MALE,
        FEMALE
    };

    constexpr Person(const Type aType, const GUID &aID, const string &aNickname, const path &aPath,
                     const wstring &aName, const size_t aAge) noexcept
        : mType(aType), mID(aID), mNickname(aNickname), mPath(aPath), mName(aName), mAge(aAge)
    {
    }

    Person(bytes &&aBytes) : IConverterBytes(move(aBytes))
    {
        // in the order of writing
        ReadAll(mType, mID, mNickname, mPath, mName, mAge);
        Clear();
    }

    virtual type_rvalue ToStream() override
    {
        Create(mType, mID, mNickname, mPath, mName, mAge);
        WriteAll(mType, mID, mNickname, mPath, mName, mAge);

        return Release();
    }

    void Print() const noexcept
    {
        wchar_t buffer[40]{};
        swprintf_s(buffer, L"{%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx}", mID.Data1,
                   mID.Data2, mID.Data3, mID.Data4[0], mID.Data4[1], mID.Data4[2], mID.Data4[3], mID.Data4[4],
                   mID.Data4[5], mID.Data4[6], mID.Data4[7]);

        wcout << vformat(L"mType = {}, mID = {}, mNickname = {}, mPath = {}, mName = {}, mAge = {}",
                         make_wformat_args((uint8_t)mType, buffer, wstring(mNickname.begin(), mNickname.end()),
                                           mPath.wstring(), mName, mAge))
              << endl;
    }

  private:
    Type mType = Type::UNKNOWN;
    GUID mID{};
    string mNickname{};
    path mPath{};
    wstring mName{};
    size_t mAge{};
};

int main()
{
    Person contextStart(Person::Type::MALE, {1, 0, 0, 'c', 'l', 'a', 'u', 'h', 'b', 'a', 'n'}, "HBann",
                        LR"(some\path.idk)", L"Claudiu", 21);
    contextStart.Print();

    Person contextEnd(move(*contextStart.ToStream()));
    contextEnd.Print();

    return 0;
}

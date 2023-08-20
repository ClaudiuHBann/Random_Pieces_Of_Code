#include <cassert>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

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

template <typename... Type> constexpr auto is_basic_string_v<std::basic_string<Type...>> = true;
} // namespace impl

template <typename Type> constexpr auto is_basic_string_v = impl::is_basic_string_v<Type>;

class IStream
{
    /*
        Format: [4 bytes +] any data + repeat...

            [4 bytes +] -> optional bytes that are available if *any data* is not a known size object
               any data -> any continuous stream of bytes


        ex.:
                We have a *type* that is a uint8_t and an *ID* that is string, the *mBytes* will contain:
                    - the *type* as a stream of bytes because it's a known size object
                    - the size in bytes of *ID* as bytes because it's not a known size object followed by the *ID* as a
                    stream of bytes


                uint8_t type = 24;
                string ID = "ea025860-02bd-47cb-b053-b82ca250e4ba";

               1 byte   +  4 bytes  +     36 bytes
                0x18    +   0x24    +   *ID* as bytes
    */
    std::vector<uint8_t> mStream{};

  public:
    using type_size_sub_stream = uint32_t; // the size type of a stream inside the internal stream
    using type_stream = decltype(mStream);
    using type_stream_value = type_stream::value_type;
    using type_rvalue_stream = std::add_rvalue_reference_t<type_stream>;

    /**
     * @brief Clears the stream
     */
    virtual ~IStream() noexcept
    {
        Clear();
    }

    /**
     * @brief Converts the object to a stream
     * @note Flow: Serialize / (Init -> Read(All)... -> Clear)
     * @return the object as a rvalue stream
     */
    virtual type_rvalue_stream ToStream() = 0;

    /**
     * @brief Converts the object to a stream
     * @note Flow: Deserialize / (Init -> Read(All)... -> Clear)
     * @param aStream the object as a rvalue stream
     */
    virtual void FromStream(type_rvalue_stream aStream) = 0;

    /**
     * @brief Getter/Setter for the internal stream
     * @param aSelf C++23 magic
     * @return (const) &(&) of the internal stream
     */
    template <class Self> constexpr auto &&GetStream(this Self &&aSelf)
    {
        return forward<Self>(aSelf).mStream;
    }

    // C++20 magic
    auto operator<=>(const IStream &) const = default;

  protected:
    /**
     * @brief Serializes the object
     * @return the object as a rvalue stream
     */
    template <typename... Types> decltype(auto) Serialize(const Types &...aObjects)
    {
        Create(aObjects...);
        WriteAll(aObjects...);
        return Release();
    }

    /**
     * @brief Deserializes the object
     * @tparam ...Types the objects's type
     * @param aStream the object as a rvalue stream
     * @param ...aObjects the object be read
     */
    template <typename... Types> void Deserialize(type_rvalue_stream aStream, Types &...aObjects)
    {
        Init(move(aStream));
        ReadAll(aObjects...);
        Clear();
    }

    /**
     * @brief Initializes the stream
     * @param aStream the object as a rvalue stream
     */
    constexpr void Init(type_rvalue_stream aStream) noexcept
    {
        mStream = move(aStream);
        mIndex = {};
    }

    /**
     * @brief Creates the fixed size stream
     * @tparam ...Types object's types
     * @param ...aObjects object
     */
    template <typename... Types> void Create(const Types &...aObjects)
    {
        mStream = type_stream(GetObjectsSize(aObjects...));
        mIndex = {};
    }

    /**
     * @brief Writes the object in the stream
     * @tparam Type the object's type
     * @param aObject the object
     * @param aSize the object's size in bytes
     */
    template <typename Type = void *> void Write(const Type &aObject, const type_size_sub_stream aSize = 0)
    {
        if constexpr (is_basic_string_v<Type>)
        {
            auto size = type_size_sub_stream(aObject.size() * sizeof(Type::value_type));
            WriteObject(aObject.data(), size);
        }
        else if constexpr (std::is_same_v<Type, std::filesystem::path>)
        {
            Write(aObject.wstring());
        }
        else if constexpr (std::is_arithmetic_v<Type> || std::is_enum_v<Type> || std::is_standard_layout_v<Type>)
        {
            WriteObjectOfKnownSize(&aObject, sizeof(Type));
        }
        else
        {
            WriteObject(&aObject, aSize);
        }
    }

    /**
     * @brief Writes all the object in the stream
     * @tparam Type the current object's type
     * @tparam ...Types the rest of object's types
     * @param aObject the current object
     * @param ...aObjects the rest of object
     */
    template <typename Type, typename... Types> void WriteAll(const Type &aObject, const Types &...aObjects)
    {
        Write(aObject);

        if constexpr (sizeof...(aObjects))
        {
            WriteAll(aObjects...);
        }
    }

    /**
     * @brief Reads the object from the stream
     * @tparam Type the object's type
     * @return the object
     */
    template <typename Type = std::span<type_stream_value>> decltype(auto) Read()
    {
        if constexpr (is_basic_string_v<Type>)
        {
            const auto [ptr, size] = ReadStream<Type>();
            return Type(ptr, size);
        }
        else if constexpr (std::is_same_v<Type, std::filesystem::path>)
        {
            const auto [ptr, size] = ReadStream<Type>();
            return basic_string<std::filesystem::path::value_type>(ptr, size);
        }
        else if constexpr (std::is_arithmetic_v<Type> || std::is_enum_v<Type> ||
                           // std::is_standard_layout_v is true for span but we want the last branch for spans so:
                           (std::is_standard_layout_v<Type> && !std::is_same_v<Type, std::span<type_stream_value>>))
        {
            return ReadObjectOfKnownSize<Type>();
        }
        else
        {
            return ReadObject();
        }
    }

    /**
     * @brief Reads all the object from the stream
     * @tparam Type the current object's type
     * @tparam ...Types the rest of object's types
     * @param aObject the current object
     * @param ...aObjects the rest of object
     */
    template <typename Type, typename... Types> void ReadAll(Type &aObject, Types &...aObjects)
    {
        aObject = Read<Type>();

        if constexpr (sizeof...(aObjects))
        {
            ReadAll(aObjects...);
        }
    }

    /**
     * @brief Releases the stream
     * @return the stream
     */
    constexpr decltype(auto) Release() noexcept
    {
        return move(mStream);
    }

    /**
     * @brief Clears the stream
     */
    constexpr void Clear() noexcept
    {
        mStream.clear();
    }

  private:
    size_t mIndex{};

    /**
     * @brief Used by the recursive variadic templated GetObjectsSize when there is nothing to unfold
     * @return 0
     */
    constexpr size_t GetObjectsSize() const noexcept
    {
        return 0;
    }

    /**
     * @brief Calculates the required size in bytes to store the object in the stream
     * @tparam Type the current object's type
     * @tparam ...Types the rest of object's types
     * @param aObject the current object
     * @param ...aObjects the rest of object
     * @return the required size in bytes to store the object in the stream
     */
    template <typename Type, typename... Types>
    constexpr decltype(auto) GetObjectsSize(const Type &aObject, const Types &...aObjects) const noexcept
    {
        if constexpr (is_basic_string_v<Type>)
        {
            const auto sizeInBytesOfStr = aObject.size() * sizeof(Type::value_type);
            // not a known size object so add the size in bytes of it's leading size in bytes
            return sizeof(type_size_sub_stream) + sizeInBytesOfStr + GetObjectsSize(aObjects...);
        }
        else if constexpr (std::is_same_v<Type, std::filesystem::path>)
        {
            const auto sizeInBytesOfPath = aObject.wstring().size() * sizeof(Type::value_type);
            // not a known size object so add the size in bytes of it's leading size in bytes
            return sizeof(type_size_sub_stream) + sizeInBytesOfPath + GetObjectsSize(aObjects...);
        }
        else if constexpr (std::is_arithmetic_v<Type> || std::is_enum_v<Type> || std::is_standard_layout_v<Type>)
        {
            return sizeof(Type) + GetObjectsSize(aObjects...);
        }
        else
        {
            assert(false && "Could not calculate the size in bytes of current type!");
        }
    }

    /**
     * @brief Writes a number of bytes from stream
     * @param aStream the stream
     * @param aSize the number of bytes to write
     */
    void WriteObject(const void *aStream, const type_size_sub_stream aSize)
    {
        assert(aSize);
        assert(mIndex + sizeof(type_size_sub_stream) <= mStream.size());

        // write the stream's size as bytes
        std::memcpy(mStream.data() + mIndex, &aSize, sizeof(type_size_sub_stream));
        mIndex += sizeof(type_size_sub_stream);

        assert(mIndex + aSize <= mStream.size());

        // write the stream
        std::memcpy(mStream.data() + mIndex, aStream, aSize);
        mIndex += aSize;
    }

    /**
     * @brief Writes a number of bytes from stream
     * @note the object's type must be a known size type
     * @param aStream the stream
     * @param aSize the number of bytes to write
     */
    void WriteObjectOfKnownSize(const void *aStream, const type_size_sub_stream aSize)
    {
        assert(mIndex + aSize <= mStream.size());

        std::memcpy(mStream.data() + mIndex, aStream, aSize);
        mIndex += aSize;
    }

    /**
     * @brief Reads the size of the current sub stream inside the stream
     * @return the current sub stream size
     */
    decltype(auto) ReadSize() noexcept
    {
        assert(mIndex + sizeof(type_size_sub_stream) <= mStream.size());

        const auto sizePtr = reinterpret_cast<type_size_sub_stream *>(mStream.data() + mIndex);
        mIndex += sizeof(type_size_sub_stream);
        return *sizePtr;
    }

    /**
     * @brief Reads an object from the stream
     * @tparam Type the stream's type
     * @return a pair containing the stream start and it's size
     */
    template <typename Type> decltype(auto) ReadStream() noexcept
    {
        const auto spen(ReadObject());
        const auto streamType = reinterpret_cast<Type::value_type *>(spen.data());
        const auto streamSize = spen.size_bytes() / sizeof(Type::value_type);

        return std::pair{streamType, streamSize};
    }

    /**
     * @brief Reads an object from the stream
     * @return a span containing the object as a stream
     */
    decltype(auto) ReadObject() noexcept
    {
        const auto size = ReadSize();
        assert(mIndex + size <= mStream.size());

        std::span<type_stream_value> spen(mStream.data() + mIndex, size);
        mIndex += size;
        return spen;
    }

    /**
     * @brief Reads an object from stream
     * @note the object's type must be a known size type
     * @return the object
     */
    template <typename Type> constexpr decltype(auto) ReadObjectOfKnownSize() noexcept
    {
        assert(mIndex + sizeof(Type) <= mStream.size());

        const auto objectPtr = reinterpret_cast<Type *>(mStream.data() + mIndex);
        mIndex += sizeof(Type);
        return *objectPtr;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////

#include <format>
#include <iostream>

using namespace std;
using namespace filesystem;

wstring to_wstring(const GUID &aGUID)
{
    wchar_t buffer[40]{};
    swprintf_s(buffer, L"{%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx}", aGUID.Data1,
               aGUID.Data2, aGUID.Data3, aGUID.Data4[0], aGUID.Data4[1], aGUID.Data4[2], aGUID.Data4[3], aGUID.Data4[4],
               aGUID.Data4[5], aGUID.Data4[6], aGUID.Data4[7]);
    return buffer;
}

class Person : public IStream
{
  public:
    enum class Type : uint8_t
    {
        UNKNOWN,
        MALE,
        FEMALE
    };

    constexpr Person() noexcept = default;

    constexpr Person(const Type aType, const GUID &aID, const string &aNickname, const path &aPath,
                     const wstring &aName, const size_t aAge) noexcept
        : mType(aType), mID(aID), mNickname(aNickname), mPath(aPath), mName(aName), mAge(aAge)
    {
    }

    void FromStream(type_rvalue_stream aStream) override
    {
        // in the order of writing
        Deserialize(move(aStream), mType, mID, mNickname, mPath, mName, mAge);
    }

    type_rvalue_stream ToStream() override
    {
        return Serialize(mType, mID, mNickname, mPath, mName, mAge);
    }

    void Print() const noexcept
    {

        wcout << vformat(L"mType = {}, mID = {}, mNickname = {}, mPath = {}, mName = {}, mAge = {}",
                         make_wformat_args((uint8_t)mType, to_wstring(mID), wstring(mNickname.begin(), mNickname.end()),
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

    Person contextEnd;
    contextEnd.FromStream(move(contextStart.ToStream()));
    contextEnd.Print();

    return 0;
}

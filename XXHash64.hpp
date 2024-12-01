#include <array>
#include <bit>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <span>
#include <string_view>
#include <vector>

class XXHash64
{
  public:
    static constexpr uint64_t DigestData(const std::span<const uint8_t> aData) noexcept
    {
        XXHash64 hasher;
        hasher.Update(aData);
        return hasher.Digest();
    }

    static constexpr uint64_t DigestString(const std::string_view aString) noexcept
    {
        return DigestData({std::bit_cast<const uint8_t *>(aString.data()), aString.size()});
    }

    static uint64_t DigestFile(const std::filesystem::path &aFile, const uint64_t aChunkSize = 64 * 1024) noexcept
    {
        std::ifstream ifs(aFile, std::ios::binary);
        if (!ifs)
        {
            return {};
        }

        XXHash64 hasher;
        std::vector<uint8_t> buffer(aChunkSize);

        while (ifs.read(reinterpret_cast<char *>(buffer.data()), buffer.size()))
        {
            hasher.Update(buffer);
        }

        const auto fileSizeTotal = std::filesystem::file_size(aFile);
        hasher.Update({buffer.data(), fileSizeTotal % aChunkSize});

        return hasher.Digest();
    }

  public:
    constexpr explicit XXHash64(const uint64_t aSeed = {}) noexcept
        : mState({aSeed + PRIME_1 + PRIME_2, aSeed + PRIME_2, aSeed, aSeed - PRIME_1})
    {
    }

    constexpr bool Update(const std::span<const uint8_t> aData) noexcept
    {
        if (!aData.size())
        {
            return false;
        }

        mTotalSize += aData.size();

        // if the data is small enough, fill the buffer with it
        if (mBufferOffset + aData.size() < mBuffer.size())
        {
            AppendToBuffer(aData);
            return true;
        }

        int64_t dataOffset{};
        const int64_t dataEnd = aData.size() - mBuffer.size();

        // if the buffer has some data, fill it and process the chunk
        if (mBufferOffset > 0)
        {
            dataOffset += AppendToBuffer(aData.subspan(0, mBuffer.size() - mBufferOffset));
            ProcessChunk(mBuffer, mState);
        }

        while (dataOffset <= dataEnd)
        {
            ProcessChunk(aData.subspan(dataOffset, mBuffer.size()), mState);
            dataOffset += mBuffer.size();
        }

        // fill the buffer with the remaining data
        mBufferOffset = 0;
        AppendToBuffer(aData.subspan(dataOffset));

        return true;
    }

    constexpr uint64_t Digest() const noexcept
    {
        uint64_t result{};
        if (mTotalSize >= mBuffer.size())
        {
            result = FoldChunkIntoBlock();
        }
        else
        {
            // take original seed
            result = mState[2] + PRIME_5;
        }
        result += mTotalSize;

        uint64_t dataOffset{};

        // process one block at a time
        for (; dataOffset + sizeof(uint64_t) <= mBufferOffset; dataOffset += sizeof(uint64_t))
        {
            const auto block = *std::bit_cast<const uint64_t *>(mBuffer.data() + dataOffset);
            result = RotateLeft(result ^ ProcessBlock(0, block), 27) * PRIME_1 + PRIME_4;
        }

        // if half a block left, process it
        if (dataOffset + sizeof(uint32_t) <= mBufferOffset)
        {
            const auto block = *std::bit_cast<const uint32_t *>(mBuffer.data() + dataOffset);
            result = RotateLeft(result ^ block * PRIME_1, 23) * PRIME_2 + PRIME_3;
            dataOffset += sizeof(uint32_t);
        }

        // process the remaining bytes
        while (dataOffset != mBufferOffset)
        {
            result = RotateLeft(result ^ mBuffer[dataOffset++] * PRIME_5, 11) * PRIME_1;
        }

        // mix
        result ^= result >> 33;
        result *= PRIME_2;
        result ^= result >> 29;
        result *= PRIME_3;
        result ^= result >> 32;

        return result;
    }

  private:
    constexpr uint64_t FoldChunkIntoBlock() const noexcept
    {
        uint64_t result{};

        constexpr std::array<uint8_t, BLOCKS_PER_CHUNK> bits{1, 7, 12, 18};
        for (size_t i = 0; i < BLOCKS_PER_CHUNK; i++)
        {
            result += RotateLeft(mState[i], bits[i]);
        }

        for (size_t i = 0; i < BLOCKS_PER_CHUNK; i++)
        {
            result = (result ^ ProcessBlock(0, mState[i])) * PRIME_1 + PRIME_4;
        }

        return result;
    }

    static constexpr uint64_t RotateLeft(uint64_t aData, unsigned char aBits) noexcept
    {
        return (aData << aBits) | (aData >> (std::numeric_limits<uint64_t>::digits - aBits));
    }

    static constexpr uint64_t ProcessBlock(const uint64_t aPrevious, const uint64_t aInput) noexcept
    {
        return RotateLeft(aPrevious + aInput * PRIME_2, std::numeric_limits<uint32_t>::digits - 1) * PRIME_1;
    }

    static constexpr void ProcessChunk(const std::span<const uint8_t> aData, const std::span<uint64_t> aStates) noexcept
    {
        const std::span<const uint64_t> dataBlock{std::bit_cast<const uint64_t *>(aData.data()), BLOCKS_PER_CHUNK};
        for (size_t i = 0; i < BLOCKS_PER_CHUNK; i++)
        {
            aStates[i] = ProcessBlock(aStates[i], dataBlock[i]);
        }
    }

  private:
    uint64_t AppendToBuffer(const std::span<const uint8_t> aData) noexcept
    {
        assert(mBufferOffset + aData.size() <= mBuffer.size());
        std::memcpy(mBuffer.data() + mBufferOffset, aData.data(), aData.size());

        mBufferOffset += aData.size();
        return aData.size();
    }

  private:
    static constexpr uint64_t PRIME_1 = 11400714785074694791;
    static constexpr uint64_t PRIME_2 = 14029467366897019727;
    static constexpr uint64_t PRIME_3 = 1609587929392839161;
    static constexpr uint64_t PRIME_4 = 9650029242287828579;
    static constexpr uint64_t PRIME_5 = 2870177450012600261;

    static constexpr uint8_t BLOCKS_PER_CHUNK = 4;

    std::array<uint64_t, BLOCKS_PER_CHUNK> mState{};
    std::array<uint8_t, 32> mBuffer{};

    uint64_t mBufferOffset{};
    uint64_t mTotalSize{};
};

int main()
{
    return 0;
}

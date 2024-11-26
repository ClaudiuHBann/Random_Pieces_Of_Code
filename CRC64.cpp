#include <array>
#include <filesystem>
#include <fstream>
#include <print>
#include <span>
#include <string>
#include <vector>

class CRC64
{
  public:
    enum Poly : uint64_t
    {
        ECMA182 = 0x42F0E1EBA9EA3693
    };

    constexpr CRC64(const Poly aPoly) noexcept : mTable(GenerateTable(aPoly))
    {
    }

    constexpr uint64_t DigestData(const std::span<const uint8_t> aData) const noexcept
    {
        return Update(aData, 0);
    }

    constexpr uint64_t DigestString(const std::string_view aString) const noexcept
    {
        return Update({reinterpret_cast<const uint8_t *>(aString.data()), aString.size()}, 0);
    }

    uint64_t DigestFile(const std::filesystem::path &aFile, const uint64_t aChunkSize = 64 * 1024) const noexcept
    {
        std::ifstream ifs(aFile, std::ios::binary);
        if (!ifs)
        {
            return {};
        }

        std::vector<uint8_t> buffer(aChunkSize);

        uint64_t crc{};
        while (ifs.read(reinterpret_cast<char *>(buffer.data()), buffer.size()))
        {
            crc = Update(buffer, crc);
        }

        const auto fileSizeTotal = std::filesystem::file_size(aFile);
        return Update({buffer.data(), fileSizeTotal % aChunkSize}, crc);
    }

  private:
    static constexpr std::array<uint64_t, 256> GenerateTable(const Poly aPoly) noexcept
    {
        std::array<uint64_t, 256> table{};
        for (uint64_t i = 0; i < table.size(); i++)
        {
            uint64_t crc{};
            uint64_t c = i << 56;
            for (uint64_t j = 0; j < 8; j++)
            {
                const auto xorPoly = (crc ^ c) & (std::numeric_limits<int64_t>::max() + 1);

                crc <<= 1;
                if (xorPoly)
                {
                    crc ^= aPoly;
                }
                c <<= 1;
            }

            table[i] = crc;
        }

        return table;
    }

  private:
    constexpr uint64_t Update(const std::span<const uint8_t> aData, uint64_t aCRC) const noexcept
    {
        for (uint64_t i = 0; i < aData.size(); i++)
        {
            const uint64_t t = ((aCRC >> 56) ^ aData[i]) & std::numeric_limits<uint8_t>::max();
            aCRC = mTable[t] ^ (aCRC << 8);
        }

        return aCRC;
    }

  private:
    const std::array<uint64_t, 256> mTable;
};

int main()
{
    const CRC64 crc64(CRC64::Poly::ECMA182);

    std::println("{:x}", crc64.DigestFile(__FILE__));
    std::println("{:x}", crc64.DigestString("Caricioplan"));
    const std::vector<uint8_t> bytes{0x48, 0x42, 0x61, 0x6E, 0x6E};
    std::println("{:x}", crc64.DigestData(bytes));

    return 0;
}

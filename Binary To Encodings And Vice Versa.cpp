#define BSWAP16(value) (unsigned short)(((value) << 8) | (((value) >> 8) & 0xFF))
#define BSWAP32(value) (unsigned int)((((value) & 0xFF000000) >> 24) | (((value) & 0x00FF0000) >> 8) | (((value) & 0x0000FF00) << 8) | (((value) & 0x000000FF) << 24))

#include <bitset>

std::string ASCIIToBinary(const std::string& text, const unsigned int blockSize = 8, const std::string& separator = " ")
{
    std::string binaryText, block;
    std::bitset<8U> asciiCharToBinary;

    for (char i : text)
    {
        asciiCharToBinary = i;
        block += asciiCharToBinary.to_string();

        while (block.length() > blockSize)
        {
            binaryText += block.substr(0, blockSize);
            binaryText += separator;

            block = block.substr(blockSize, block.length() - blockSize);
        }
    }

    if (block.length() != 0)
    {
        binaryText += block;
    }

    return binaryText;
}

std::string Unicode16ToBinary(const std::u16string& text, const unsigned int blockSize = 16, const std::string& separator = " ", const bool littleEndian = false)
{
    std::string binaryText, block;
    std::bitset<16U> unicode16CharToBinary;

    for (char16_t i : text)
    {
        unicode16CharToBinary = (littleEndian) ? BSWAP16(i) : i;
        block += unicode16CharToBinary.to_string();

        while (block.length() > blockSize)
        {
            binaryText += block.substr(0, blockSize);
            binaryText += separator;

            block = block.substr(blockSize, block.length() - blockSize);
        }
    }

    if (block.length() != 0)
    {
        binaryText += block;
    }

    return binaryText;
}

std::string Unicode32ToBinary(const std::u32string& text, const unsigned int blockSize = 32, const std::string& separator = " ", const bool littleEndian = false)
{
    std::string binaryText, block;
    std::bitset<32U> unicode32CharToBinary;

    for (char32_t i : text)
    {
        unicode32CharToBinary = (littleEndian) ? BSWAP32(i) : i;
        block += unicode32CharToBinary.to_string();

        while (block.length() > blockSize)
        {
            binaryText += block.substr(0, blockSize);
            binaryText += separator;

            block = block.substr(blockSize, block.length() - blockSize);
        }
    }

    if (block.length() != 0)
    {
        binaryText += block;
    }

    return binaryText;
}

std::string BinaryToASCII(const std::string& binaryText, const unsigned int blockSize = 8, const unsigned int separatorLength = 1)
{
    std::string text, block;
    unsigned int separatorBlockStartIndex = blockSize;

    for (unsigned int i = 0; i < binaryText.length(); i++)
    {
        if (i == separatorBlockStartIndex)
        {
            i += separatorLength;
            separatorBlockStartIndex += blockSize + separatorLength;
        }

        block += binaryText[i];

        if (block.length() == 8)
        {
            char binaryTextToASCIIChar = 0;
            for (unsigned int j = 0; j < block.length(); j++)
            {
                if (block[j] == '1')
                {
                    binaryTextToASCIIChar += (char)pow(2, block.length() - j - 1);
                }
            }

            block.clear();
            text += binaryTextToASCIIChar;
        }
    }

    return text;
}

std::u16string BinaryToUnicode16(const std::string& binaryText, const unsigned int blockSize = 8, const unsigned int separatorLength = 1, const bool littleEndian = false)
{
    std::u16string text, block;
    unsigned int separatorBlockStartIndex = blockSize;

    for (unsigned int i = 0; i < binaryText.length(); i++)
    {
        if (i == separatorBlockStartIndex)
        {
            i += separatorLength;
            separatorBlockStartIndex += blockSize + separatorLength;
        }

        block += binaryText[i];

        if (block.length() == 16)
        {
            unsigned short binaryTextToASCIIChar = 0;
            for (unsigned int j = 0; j < block.length(); j++)
            {
                if (block[j] == '1')
                {
                    binaryTextToASCIIChar += (unsigned short)pow(2, block.length() - j - 1);
                }
            }

            block.clear();
            text += (littleEndian) ? BSWAP16(binaryTextToASCIIChar) : binaryTextToASCIIChar;
        }
    }

    return text;
}

std::u32string BinaryToASCII(const std::string& binaryText, const unsigned int blockSize = 8, const unsigned int separatorLength = 1, const bool littleEndian = false)
{
    std::u32string text, block;
    unsigned int separatorBlockStartIndex = blockSize;

    for (unsigned int i = 0; i < binaryText.length(); i++)
    {
        if (i == separatorBlockStartIndex)
        {
            i += separatorLength;
            separatorBlockStartIndex += blockSize + separatorLength;
        }

        block += binaryText[i];

        if (block.length() == 32)
        {
            unsigned int binaryTextToASCIIChar = 0;
            for (unsigned int j = 0; j < block.length(); j++)
            {
                if (block[j] == '1')
                {
                    binaryTextToASCIIChar += (unsigned int)pow(2, block.length() - j - 1);
                }
            }

            block.clear();
            text += (littleEndian) ? BSWAP32(binaryTextToASCIIChar) : binaryTextToASCIIChar;
        }
    }

    return text;
}

#include <iostream>

#include <fstream>
#include <map>
#include <string>

class IniException final : public std::exception
{
  public:
    explicit IniException(const std::string &aMessage) noexcept : mMessage(aMessage)
    {
    }

    const char *what() const noexcept override
    {
        return mMessage.c_str();
    }

  private:
    std::string mMessage;
};

typedef struct IniContext_s
{
    char sectionStart = '[';
    char sectionEnd = ']';

    char pairSeparator = '=';

    std::string commentsStart = ";#";

    std::string spaces = " \f\t\v"; // no line feed or carriage return
} IniContext;

class IniHelper final
{
  public:
    IniHelper(const IniContext &aContext) noexcept : mContext(aContext)
    {
    }

    std::string Trim(const std::string &aString) const noexcept
    {
        const auto first = aString.find_first_not_of(mContext.spaces);
        if (first == std::string::npos)
        {
            return {};
        }

        const auto last = aString.find_last_not_of(mContext.spaces);
        return aString.substr(first, last - first + 1);
    }

  private:
    IniContext mContext;
};

class IniLexer final
{
  public:
    enum class Token : uint8_t
    {
        NONE,
        COMMENT,
        SECTION,
        PAIR
    };

    explicit IniLexer(const IniContext &aContext = {}) noexcept : mContext(aContext), mHelper(aContext)
    {
    }

    std::string ParseSection(std::string aLine) const
    {
        if (!IsSection(aLine))
        {
            throw IniException("Invalid section!");
        }

        aLine = mHelper.Trim(aLine);
        const auto section(aLine.substr(1, aLine.size() - 2));
        return mHelper.Trim(section);
    }

    std::pair<std::string, std::string> ParsePair(std::string aLine) const
    {
        if (!IsPair(aLine))
        {
            throw IniException("Invalid pair!");
        }

        aLine = mHelper.Trim(aLine);

        const auto pos = aLine.find(mContext.pairSeparator);

        const auto key = aLine.substr(0, pos);
        const auto value = aLine.substr(pos + 1);

        return {mHelper.Trim(key), mHelper.Trim(value)};
    }

    std::string FormatSection(const std::string &aSection) const noexcept
    {
        return mContext.sectionStart + aSection + mContext.sectionEnd;
    }

    std::string FormatPair(const std::string &aKey, const std::string &aValue) const noexcept
    {
        return aKey + mContext.pairSeparator + aValue;
    }

    Token FindToken(const std::string &aLine) const noexcept
    {
        if (IsComment(aLine))
        {
            return Token::COMMENT;
        }

        if (IsSection(aLine))
        {
            return Token::SECTION;
        }

        if (IsPair(aLine))
        {
            return Token::PAIR;
        }

        return Token::NONE;
    }

  private:
    IniContext mContext;
    IniHelper mHelper;

    bool IsSection(std::string aLine) const noexcept
    {
        aLine = mHelper.Trim(aLine);
        return aLine.front() == mContext.sectionStart && aLine.back() == mContext.sectionEnd;
    }

    bool IsPair(std::string aLine) const noexcept
    {
        aLine = mHelper.Trim(aLine);
        return aLine.find(mContext.pairSeparator) != std::string::npos;
    }

    bool IsComment(const std::string &aLine) const noexcept
    {
        for (const auto commentStart : mContext.commentsStart)
        {
            if (aLine.front() == commentStart)
            {
                return true;
            }
        }

        return false;
    }
};

class IniParser final
{
  public:
    using Section = std::map<std::string, std::string>;

    explicit IniParser(const IniContext &aContext = IniContext()) noexcept
        : mContext(aContext), mHelper(mContext), mLexer(mContext)
    {
    }

    void Deserialize(std::ifstream &aStream)
    {
        std::string sectionLast;

        std::string line;
        while (std::getline(aStream, line))
        {
            line = mHelper.Trim(line);
            if (line.empty())
            {
                continue;
            }

            switch (mLexer.FindToken(line))
            {
            case IniLexer::Token::SECTION: {
                sectionLast = mLexer.ParseSection(line);
            }
            break;

            case IniLexer::Token::PAIR: {
                const auto keyAndValue(mLexer.ParsePair(line));
                mSections[sectionLast][keyAndValue.first] = keyAndValue.second;
            }
            break;

            case IniLexer::Token::COMMENT:
            case IniLexer::Token::NONE:
            default:
                break;
            }
        }
    }

    void Serialize(std::ofstream &aStream) const
    {
        const auto itLast = std::prev(mSections.cend(), 1);
        for (auto it = mSections.begin(); it != mSections.end(); it++)
        {
            aStream << mLexer.FormatSection(it->first) << '\n';

            for (const auto &keyAndValue : it->second)
            {
                aStream << mLexer.FormatPair(keyAndValue.first, keyAndValue.second) << '\n';
            }

            if (it != itLast)
            {
                aStream << '\n';
            }
        }
    }

    const std::string &Get(const std::string &aSection, const std::string &aKey) const
    {
        return mSections.at(aSection).at(aKey);
    }

  private:
    std::map<std::string, Section> mSections;

    IniContext mContext;
    IniHelper mHelper;
    IniLexer mLexer;
};

int main()
{
    IniParser parser;

    std::ifstream ifs("input.ini");
    parser.Deserialize(ifs);

    std::cout << parser.Get("Install", "AppFolder") << std::endl;

    std::ofstream ofs("output.ini");
    parser.Serialize(ofs);

    return 0;
}

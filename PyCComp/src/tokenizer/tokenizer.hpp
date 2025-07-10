#pragma once
#include <cstdint>
#include <string_view>
#include <array>
#include <span>
#include <string>
#include <vector>

class SourceReader;

class Tokenizer
{
public:
    struct Token
    {
        enum Type: uint8_t
        {
            KEYWORD,
            IDENTIFIER,
            STRING,
            NUMBER,
            BOOLEAN,
            NONE,
            OPERATOR,
            DELIMITER,
            INDENT,
            DEDENT,
            NEWLINE,
            COMMENT,
            END
        };
        
        Type type;
        std::string value;
        uint32_t line;
        uint32_t column;
    };

    explicit Tokenizer(std::string_view p_Contents);

    [[nodiscard]] const std::vector<Token>& getTokens() const { return m_Tokens; }

private:
    std::string m_Source;
    std::vector<Token> m_Tokens;

    friend class TokenizerTool;
private:
    enum FoundStatus: uint8_t {NOT_FOUND, FOUND, UNUSED};

    
    template<size_t S1, size_t S2>
    static FoundStatus findInLists(const std::array<const char*, S1>& p_elems, const std::array<const char*, S2>& p_Unused, std::string_view p_Elem);

    static FoundStatus isKeyword(std::string_view p_Identifier);
    static FoundStatus isOperator(std::string_view p_Operator);
    static FoundStatus isDelimiter(std::string_view p_Delimiter);
    static bool isOpeningDelimiter(std::string_view p_Delimiter);
    static bool isClosingDelimiter(std::string_view p_Delimiter);

    static constexpr std::array<const char*, 17> c_Keywords{
        "if", "else", "elif", "while", "for", "def", "return", "class",
        "import", "from", "as", "pass", "break", "continue", /*OPERATORS*/ "and", "or", "not"
    };
    static constexpr std::array<const char*, 13> c_UnusedKeywords{
        "try", "except", "finally", "with", "yield", "lambda", "global", "nonlocal",
        "assert", "raise", "del", "in", /*OPERATORS*/ "is"
    };

    static constexpr std::array<const char*, 33> c_Operators{
        "+", "-", "*", "/", "//", "%", "**",
        "==", "!=", "<", "<=", ">", ">=",
        "&", "|", "^", "~", ">>", "<<",
        "=", "+=", "-=", "*=", "/=", "//=", "%=", "**=", "&=", "|=", "^=",
        "->"
    };

    static constexpr std::array<const char*, 0> c_UnusedOperators{
        
    };

    static constexpr std::array<const char*, 4> c_OperatorKeywords{
        "and", "or", "not", "is"
    };

    static constexpr std::array<char, 14> c_OperatorCharacters{
        '+', '-', '*', '/', '%', '=', '<', '>', '!', '&', '|', '^', '~'
    };

    static constexpr std::array<const char*, 10> c_Delimiters{
        "(", ")", "[", "]", "{", "}", ",", ":", ";", "."
    };

    static constexpr std::array<const char*, 1> c_UnusedDelimiters{
        "@"
    };

    static constexpr std::array<const char*, 3> c_OpeningDelimiters{
        "(", "[", "{"
    };

    static constexpr std::array<const char*, 3> c_ClosingDelimiters{
        ")", "]", "}"
    };
};


#include "tokenizer.hpp"

#include <iostream>
#include <stdexcept>

struct TokenizerTool
{
    uint32_t column = 0;
    uint32_t line = 1;

    std::string currentToken{};
    enum : uint8_t {WORD, OPERATOR, SINGLE_STRING, MULTI_STRING, COMMENT, NONE} currentTokenType = NONE;
    uint32_t delimiterLevel = 0;
    uint32_t currentScope = 0;
    uint32_t spaceStack = 0;
    uint32_t localScope = 0;
    bool lineStart = true;
    uint32_t stringBuffer = 0;
    char strTokenStart = '\0';
    Tokenizer& tokenizer;

    struct Error
    {
        std::string message;
        uint32_t line;
        uint32_t column;
    };
    std::vector<Error> errors;

    explicit TokenizerTool(Tokenizer& p_Tokenizer) : tokenizer(p_Tokenizer) { }

    void advanceColumnCount()
    {
        column++;
    }

    void advanceLineCount()
    {
        line++;
        column = 0;
        localScope = 0;
        lineStart = true;
    }
    void finishLineStart(const bool p_Indent = true)
    {
        if (!lineStart)
        {
            return;
        }
        lineStart = false;
        if (!p_Indent)
        {
            return;
        }
        if (spaceStack != 0)
        {
            errors.push_back({ .message = "Inconsistent indent at the beginning of the line", .line = line, .column = column });
        }
        if (localScope != currentScope)
        {
            if (localScope > currentScope)
            {
                if (localScope - currentScope > 1)
                {
                    errors.push_back({ .message = "Invalid indentation level", .line = line, .column = column });
                    return;
                }
                tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::INDENT, .line = line, .column = column });
                currentScope = localScope;
            }
            else
            {
                while (currentScope > localScope)
                {
                    tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::DEDENT, .line = line, .column = column });
                    currentScope--;
                }
            }
        }
    }

    void pushNewline()
    {
        finishToken();
        if (delimiterLevel == 0 && !lineStart)
        {
            tokenizer.m_Tokens.push_back({.type = Tokenizer::Token::Type::NEWLINE, .line = line, .column = column });
        }
        advanceLineCount();
    }

    void finishToken()
    {
        if (currentTokenType == NONE)
        {
            return;
        }
        if (currentTokenType == WORD)
        {
            finishAlnumToken();
        }
        else if (currentTokenType == OPERATOR)
        {
            const Tokenizer::FoundStatus l_Status = Tokenizer::isOperator(currentToken);
            if (l_Status == Tokenizer::UNUSED)
            {
                errors.push_back({ .message = "Operator " + currentToken + " is not implemented", .line = line, .column = column });
            }
            else if (l_Status == Tokenizer::NOT_FOUND)
            {
                errors.push_back({ .message = "Invalid operator: " + currentToken, .line = line, .column = column });
            }
            tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::OPERATOR, .value = currentToken, .line = line, .column = column });
        }
        else if (currentTokenType == SINGLE_STRING || currentTokenType == MULTI_STRING)
        {
            tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::STRING, .value = '"' + currentToken + '"', .line = line, .column = column });
            strTokenStart = '\0';
            stringBuffer = 0;
        }
        else if (currentTokenType == COMMENT)
        {
            tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::COMMENT, .value = currentToken, .line = line, .column = column });
        }
        currentTokenType = NONE;
        currentToken = "";
    }

    void finishAlnumToken()
    {
        if (currentToken == "None")
        {
            tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::NONE, .line = line, .column = column });
            return;
        }

        if (currentToken == "True" || currentToken == "False")
        {
            tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::BOOLEAN, .value = currentToken, .line = line, .column = column });
            return;
        }

        {
            const Tokenizer::FoundStatus l_Status = Tokenizer::isKeyword(currentToken);
            bool l_Operator = false;
            if (l_Status != Tokenizer::NOT_FOUND)
            {
                l_Operator = Tokenizer::findInLists<0>({}, Tokenizer::c_OperatorKeywords, currentToken);
            }
            if (l_Status == Tokenizer::FOUND)
            {
                const Tokenizer::Token::Type l_OperatorType = l_Operator ? Tokenizer::Token::Type::OPERATOR : Tokenizer::Token::Type::KEYWORD;
                tokenizer.m_Tokens.push_back({ .type = l_OperatorType, .value = currentToken, .line = line, .column = column });
                currentToken.clear();
                return;
            }
            if (l_Status == Tokenizer::UNUSED)
            {
                errors.push_back({ .message = (l_Operator ? "keyword " : "operator ") + currentToken + " is not implemented", .line = line, .column = column });
                currentToken.clear();
                return;
            }
        }

        try
        {
            float l_Num = std::stof(currentToken);
            tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::NUMBER, .value = currentToken, .line = line, .column = column });
            return;
        }
        catch (const std::invalid_argument&) {}

        if (!std::isalpha(currentToken[0]) && currentToken[0] != '_')
        {
            errors.push_back({ .message = "Invalid identifier: " + currentToken, .line = line, .column = column });
            return;
        }
        tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::IDENTIFIER, .value = currentToken, .line = line, .column = column });
    }

    void digestIndent()
    {
        if (lineStart && delimiterLevel == 0)
        {
            localScope += 1;
        }
        else
        {
            digestSpace();
        }
    }

    void digestAlphanumeric(const char p_Char)
    {
        if (currentTokenType != WORD && currentTokenType != NONE)
        {
            finishToken();
        }
        currentTokenType = WORD;
        currentToken += p_Char;
        finishLineStart();
    }

    void digestSpace()
    {
        if (lineStart && delimiterLevel == 0)
        {
            spaceStack++;
            if (spaceStack == 4)
            {
                localScope += 1;
                spaceStack = 0;
            }
        }
        else
        {
            finishToken();
        }
    }

    bool checkStringDelimiter(const char p_Char)
    {
        if (currentTokenType == COMMENT)
        {
            return false;
        }

        if ((p_Char == '"' || p_Char == '\'') && strTokenStart == '\0')
        {
            finishToken();
            strTokenStart = p_Char;
        }
        if (p_Char == strTokenStart)
        {
            if (currentTokenType == SINGLE_STRING || (currentTokenType == MULTI_STRING && stringBuffer == 2))
            {
                finishToken();
            }
            else
            {
                stringBuffer += 1;
            }
            return true;
        }
        if (currentTokenType == SINGLE_STRING || currentTokenType == MULTI_STRING)
        {
            if (p_Char == '\n' && currentTokenType == SINGLE_STRING)
            {
                errors.push_back({ .message = "Unclosed single-quoted string at line " + std::to_string(line) + ", column " + std::to_string(column), .line = line, .column = column });
                finishToken();
                return false;
            }
            currentToken += p_Char;
            return true;
        }
        if (stringBuffer == 1 || stringBuffer == 3)
        {
            currentTokenType = stringBuffer == 1 ? SINGLE_STRING : MULTI_STRING;
            stringBuffer = 0;
            currentToken += p_Char;
            return true;
        }
        if (stringBuffer == 2 || stringBuffer == 6)
        {
            currentTokenType = SINGLE_STRING;
            finishToken();
        }
        return false;
    }

    bool checkOperator(const char p_Char)
    {
        for (const char l_Operator : Tokenizer::c_OperatorCharacters)
        {
            if (l_Operator == p_Char)
            {
                if (currentTokenType != OPERATOR && currentTokenType != NONE)
                {
                    finishToken();
                }
                currentToken += p_Char;
                currentTokenType = OPERATOR;
                return true;
            }
        }
        return false;
    }

    bool checkComment(const char p_Char)
    {
        if (currentTokenType == COMMENT)
        {
            if (p_Char == '\n')
            {
                return false;
            }
            currentToken += p_Char;
            return true;
        }
        if (p_Char == '#')
        {
            if (currentTokenType != NONE)
            {
                finishToken();
            }
            currentTokenType = COMMENT;
            currentToken += p_Char;
            return true;
        }
        return false;
    }

    bool checkDelimiter(const char l_Char)
    {
        const Tokenizer::FoundStatus l_Status = Tokenizer::isDelimiter(std::string_view(&l_Char, 1));
        if (l_Status == Tokenizer::NOT_FOUND)
        {
            return false;
        }
        if (l_Status == Tokenizer::UNUSED)
        {
            errors.push_back({ .message = "Delimiter " + std::string(1, l_Char) + " is not implemented", .line = line, .column = column });
        }
        if (currentTokenType == WORD && l_Char == '.')
        {
            try
            {
                int elem = std::stoi(currentToken);
                currentToken += l_Char;
                return true;
            }
            catch (const std::invalid_argument&) {}
        }
        finishToken();
        tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::DELIMITER, .value = std::string(1, l_Char), .line = line, .column = column });
        if (Tokenizer::isOpeningDelimiter(std::string_view(&l_Char, 1)))
        {
            delimiterLevel++;
        }
        else if (Tokenizer::isClosingDelimiter(std::string_view(&l_Char, 1)))
        {
            if (delimiterLevel == 0)
            {
                errors.push_back({ .message = "Unmatched closing delimiter: " + std::string(1, l_Char), .line = line, .column = column });
            }
            else
            {
                delimiterLevel--;
            }
        }
        return true;
    }

    void finish()
    {
        if (currentTokenType == SINGLE_STRING || currentTokenType == MULTI_STRING)
        {
            errors.push_back({ .message = "Unclosed string at EOF", .line = line, .column = column });
        }
        if (delimiterLevel > 0)
        {
            errors.push_back({ .message = "Unclosed delimiter at EOF", .line = line, .column = column });
        }
        if (currentTokenType != NONE)
        {
            finishAlnumToken();
        }
        tokenizer.m_Tokens.push_back({ .type = Tokenizer::Token::Type::END, .line = line, .column = column });
    }
};

Tokenizer::Tokenizer(const std::string_view p_Contents)
    : m_Source(p_Contents)
{
    TokenizerTool l_Tool{*this};
    for (const char l_Char : m_Source)
    {
        l_Tool.advanceColumnCount();
        if (l_Tool.checkStringDelimiter(l_Char))
        {
            l_Tool.finishLineStart(false);
            continue;
        }
        if (l_Tool.checkComment(l_Char))
        {
            continue;
        }
        if (l_Tool.checkOperator(l_Char))
        {
            l_Tool.finishLineStart();
            continue;
        }
        if (l_Char == '\n')
        {
            l_Tool.pushNewline();
            continue;
        }
        if (l_Char == '\t')
        {
            l_Tool.digestIndent();
            continue;
        }
        if (std::isalnum(l_Char) || l_Char == '_')
        {
            l_Tool.digestAlphanumeric(l_Char);
            continue;
        }
        if (l_Char == ' ')
        {
            l_Tool.digestSpace();
            continue;
        }
        if (l_Tool.checkDelimiter(l_Char))
        {
            l_Tool.finishLineStart();
            continue;
        }
        l_Tool.errors.push_back({ .message = "Invalid character: " + std::string(1, l_Char), .line = l_Tool.line, .column = l_Tool.column });
    }
    l_Tool.finish();

    // Print errors if any
    for (const auto& error : l_Tool.errors)
    {
        std::cerr << "Error at line "  << error.line << ", column "  << error.column << ": " << error.message << '\n';
    }
}

template<size_t S1, size_t S2>
Tokenizer::FoundStatus Tokenizer::findInLists(const std::array<const char*, S1>& p_elems, const std::array<const char*, S2>& p_Unused, const std::string_view p_Elem)
{
    for (const char* elem : p_elems)
    {
        if (p_Elem == elem)
        {
            return FOUND;
        }
    }
    for (const auto& unusedElem : p_Unused)
    {
        if (p_Elem == unusedElem)
        {
            return UNUSED;
        }
    }
    return NOT_FOUND;
}

Tokenizer::FoundStatus Tokenizer::isKeyword(const std::string_view p_Identifier)
{
    return findInLists(c_Keywords, c_UnusedKeywords, p_Identifier);
}

Tokenizer::FoundStatus Tokenizer::isOperator(const std::string_view p_Operator)
{
    return findInLists(c_Operators, c_UnusedOperators, p_Operator);
}

Tokenizer::FoundStatus Tokenizer::isDelimiter(const std::string_view p_Delimiter)
{
    return findInLists(c_Delimiters, c_UnusedDelimiters, p_Delimiter);
}

bool Tokenizer::isOpeningDelimiter(const std::string_view p_Delimiter)
{
    constexpr std::array<const char*, 0> unused;
    return findInLists(c_OpeningDelimiters, unused, p_Delimiter) == FOUND;
}

bool Tokenizer::isClosingDelimiter(const std::string_view p_Delimiter)
{
    constexpr std::array<const char*, 0> unused;
    return findInLists(c_ClosingDelimiters, unused, p_Delimiter) == FOUND;
}

#include <iostream>

#include "source_file/source_reader.hpp"
#include "tokenizer/tokenizer.hpp"

int main(const uint32_t argc, char *argv[]) {
    // Arguments <output file> <input file> [working dir]
    if (argc < 3) {
        std::cerr << "Arguments: <output file> <input file> [working dir]\n";
        return 1;
    }
    const std::string l_OutputFile = argv[1];
    const std::string l_InputFile = argv[2];
    const std::string l_WorkingDir = argc > 3 ? argv[3] : "";

    const SourceReader l_Reader{ l_InputFile, l_WorkingDir };

    std::vector<Tokenizer> l_Tokenizers;
    l_Tokenizers.reserve(l_Reader.getModuleCount());
    for (uint32_t l_Index = 0; l_Index < l_Reader.getModuleCount(); ++l_Index) 
    {
        l_Tokenizers.emplace_back(l_Reader.getModule(l_Index)->fileContent);
        std::cout << "Module: " << l_Reader.getModule(l_Index)->fileName.string() << "\n";
        std::stringstream l_Stream;
        for (Tokenizer::Token l_Token : l_Tokenizers.back().getTokens())
        {
            // Format per line: l<4 spaces line number> | c<4 spaces column number> | type[(value) if there is value)
            switch (l_Token.type)
            {
            case Tokenizer::Token::Type::KEYWORD:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | keyword(" << l_Token.value << ")\n";
                break;
            case Tokenizer::Token::Type::IDENTIFIER:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | identifier(" << l_Token.value << ")\n";
                break;
            case Tokenizer::Token::Type::STRING:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | string(" << l_Token.value << ")\n";
                break;
            case Tokenizer::Token::Type::NUMBER:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | number(" << l_Token.value << ")\n";
                break;
            case Tokenizer::Token::Type::BOOLEAN:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | boolean(" << l_Token.value << ")\n";
                break;
            case Tokenizer::Token::Type::NONE:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | none\n";
                break;
            case Tokenizer::Token::Type::OPERATOR:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | operator(" << l_Token.value << ")\n";
                break;
            case Tokenizer::Token::Type::DELIMITER:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | delimiter(" << l_Token.value << ")\n";
                break;
            case Tokenizer::Token::Type::INDENT:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | indent\n";
                break;
            case Tokenizer::Token::Type::DEDENT:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | dedent\n";
                break;
            case Tokenizer::Token::Type::NEWLINE:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | newline\n";
                break;
            case Tokenizer::Token::Type::COMMENT:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | comment(" << l_Token.value << ")\n";
                break;
            case Tokenizer::Token::Type::END:
                l_Stream << "l" << std::setw(4) << l_Token.line << " | c" << std::setw(4) << l_Token.column << " | end\n";
                break;
            }
        }
        std::cout << l_Stream.str();
        std::cout << "***********************************************\n\n";
    }

    return 0;
}

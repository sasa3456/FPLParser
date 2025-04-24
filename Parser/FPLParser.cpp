#include "FPLParser.hpp"

#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace CE_Kernel
{
    namespace Aid
    {
        namespace FPLParser
        {
            enum class TokenType
            {
                BlockStart,
                Identifier,
                String,
                Number,
                Boolean,
                LeftBrace,
                RightBrace,
                LeftBracket,
                RightBracket,
                Colon,
                Comma,
                Eof,
                Error
            };

            struct Token
            {
                TokenType type_;
                std::string value_;
            };

            class Lexer
            {
            public:
                Lexer(std::istream& input_a)
                    : input_(input_a)
                {
                    Advance();
                }

                Token NextToken()
                {
                    SkipWhitespaceAndComments();
                    if (current_char_ == EOF)
                        return {TokenType::Eof, ""};

                    switch (current_char_) {
                    case '@':
                        Advance();
                        return {TokenType::BlockStart, "@"};
                    case '{':
                        Advance();
                        return {TokenType::LeftBrace, "{"};
                    case '}':
                        Advance();
                        return {TokenType::RightBrace, "}"};
                    case '[':
                        Advance();
                        return {TokenType::LeftBracket, "["};
                    case ']':
                        Advance();
                        return {TokenType::RightBracket, "]"};
                    case ':':
                        Advance();
                        return {TokenType::Colon, ":"};
                    case ',':
                        Advance();
                        return {TokenType::Comma, ","};
                    case '"':
                        return ReadString();

                    default:
                        if (std::isdigit(current_char_) || current_char_ == '.')
                            return ReadNumber();

                        if (std::isalpha(current_char_))
                            return ReadIdentifierOrBoolean();

                        return {TokenType::Error, std::string(1, current_char_)};
                    }
                }

            private:
                std::istream& input_;
                char current_char_;

                void Advance() 
				{
					int tmp_ = input_.get();
					if (tmp_ != EOF) 
					{
						current_char_ = static_cast<char>(tmp_);
					} 
					
					else 
					{
						current_char_ = '\0';
					}
				}

                void SkipWhitespaceAndComments()
                {
                    while (true) {
                        while (std::isspace(current_char_))
                            Advance();
                        if (current_char_ == '#')
                            while (current_char_ != '\n' && current_char_ != EOF)
                                Advance();
                        else
                            break;
                    }
                }

                Token ReadString()
                {
                    Advance();
                    std::string value_;
                    while (current_char_ != '"' && current_char_ != EOF) {
                        value_ += current_char_;
                        Advance();
                    }

                    if (current_char_ != '"')
                        throw std::runtime_error("Unterminated string");
                    Advance();
                    return {TokenType::String, value_};
                }

                Token ReadNumber()
                {
                    std::string value_;
                    bool has_dot_ = false;
                    while (std::isdigit(current_char_) || current_char_ == '.') {
                        if (current_char_ == '.') {
                            if (has_dot_)
                                return {TokenType::Error, value_ + current_char_};
                            has_dot_ = true;
                        }

                        value_ += current_char_;
                        Advance();
                    }

                    return {TokenType::Number, value_};
                }

                Token ReadIdentifierOrBoolean()
                {
                    std::string value_;
                    while (std::isalnum(current_char_) || current_char_ == '_') {
                        value_ += current_char_;
                        Advance();
                    }

                    if (value_ == "true" || value_ == "false")
                        return {TokenType::Boolean, value_};

                    return {TokenType::Identifier, value_};
                }
            };

            class Parser
            {
            public:
                Parser(Lexer& lexer_a)
                    : lexer_(lexer_a)
                {
                    current_token_ = lexer_.NextToken();
                }

                std::map<std::string, std::map<std::string, Value>> Parse()
                {
                    std::map<std::string, std::map<std::string, Value>> blocks_;
                    while (current_token_.type_ != TokenType::Eof) {
                        if (current_token_.type_ == TokenType::BlockStart) {
                            auto block_ = ParseBlock();
                            blocks_[block_.first] = block_.second;
                        }

                        else {
                            throw std::runtime_error("Expected block start");
                        }
                    }

                    return blocks_;
                }

            private:
                Lexer& lexer_;
                Token current_token_;

                void Eat(TokenType type_a)
                {
                    if (current_token_.type_ == type_a) {
                        current_token_ = lexer_.NextToken();
                    }

                    else {
                        throw std::runtime_error("Unexpected token: " + current_token_.value_);
                    }
                }

                std::pair<std::string, std::map<std::string, Value>> ParseBlock()
                {
                    Eat(TokenType::BlockStart);
                    std::string name_ = current_token_.value_;
                    Eat(TokenType::Identifier);
                    Eat(TokenType::LeftBrace);
                    auto props_ = ParseProperties();
                    Eat(TokenType::RightBrace);

                    return {name_, props_};
                }

                std::map<std::string, Value> ParseProperties()
                {
                    std::map<std::string, Value> props_;
                    while (current_token_.type_ != TokenType::RightBrace) {
                        std::string key_;
                        if (current_token_.type_ == TokenType::Identifier || current_token_.type_ == TokenType::String) {
                            key_ = current_token_.value_;
                            Eat(current_token_.type_);
                        }

                        else {
                            throw std::runtime_error("Expected key (identifier or string)");
                        }

                        Eat(TokenType::Colon);
                        props_[key_] = ParseValue();
                        if (current_token_.type_ == TokenType::Comma)
                            Eat(TokenType::Comma);
                    }

                    return props_;
                }

                Value ParseValue()
                {
                    Token token_ = current_token_;
                    switch (token_.type_) {
                    case TokenType::String:
                        Eat(TokenType::String);
                        return Value(token_.value_);

                    case TokenType::Number:
                        Eat(TokenType::Number);
                        return Value(std::stod(token_.value_));

                    case TokenType::Boolean:
                        Eat(TokenType::Boolean);
                        return Value(token_.value_ == "true");

                    case TokenType::LeftBracket: {
                        Eat(TokenType::LeftBracket);
                        std::vector<Value> arr_;
                        while (current_token_.type_ != TokenType::RightBracket) {
                            arr_.push_back(ParseValue());
                            if (current_token_.type_ == TokenType::Comma)
                                Eat(TokenType::Comma);
                        }

                        Eat(TokenType::RightBracket);
                        return Value(arr_);
                    }

                    case TokenType::LeftBrace: {
                        Eat(TokenType::LeftBrace);
                        std::map<std::string, Value> obj_;
                        while (current_token_.type_ != TokenType::RightBrace) {
                            std::string key_ = current_token_.value_;
                            Eat(TokenType::Identifier);
                            Eat(TokenType::Colon);
                            obj_[key_] = ParseValue();

                            if (current_token_.type_ == TokenType::Comma)
                                Eat(TokenType::Comma);
                        }

                        Eat(TokenType::RightBrace);
                        return Value(obj_);
                    }

                    default:
                        throw std::runtime_error("Unexpected value type");
                    }
                }
            };

            std::map<std::string, std::map<std::string, Value>> FPLParser::Parse(std::istream& input_a)
            {
                Lexer lexer_(input_a);
                Parser parser_(lexer_);
                return parser_.Parse();
            }
        } // namespace FPLParser
    } // namespace Aid
} // namespace CE_Kernel

// Copyright (c) 2025 Alexander Starov

#pragma once
#include <istream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace FPLParser
{
    struct Value
    {
        std::variant<
            std::string,
            double,
            bool,
            std::vector<Value>,
            std::map<std::string, Value>>
            data_;

        Value() = default;
        Value(std::string s_a)
            : data_(s_a)
        {
        }
        Value(double d_a)
            : data_(d_a)
        {
        }
        Value(bool b_a)
            : data_(b_a)
        {
        }
        Value(std::vector<Value> arr_a)
            : data_(arr_a)
        {
        }
        Value(std::map<std::string, Value> obj_a)
            : data_(obj_a)
        {
        }
    };

    class FPLParser
    {
    public:
        std::map<std::string, std::map<std::string, Value>> Parse(std::istream& input_a);
    };
} // namespace FPLParser

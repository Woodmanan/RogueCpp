#pragma once
#include <functional>
#include <utility>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>
#include <iostream>

//Licensesd under CC0 1.0
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

template<typename ... Args>
void string_format_print(const std::string& format, Args ... args)
{
    std::cout << string_format(format, args ...) << std::endl;
}

template<typename ... Args>
void string_format_print_error(const std::string& format, Args ... args)
{
    std::cerr << string_format(format, args ...) << std::endl;
}

std::vector<std::string> string_split(const std::string& string, const std::string& splitOn);
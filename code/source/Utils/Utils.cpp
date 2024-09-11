#include "Utils.h"

std::vector<std::string> string_split(const std::string& string, const std::string& splitOn)
{
    std::vector<std::string> tokens;
    size_t position = 0;
    std::string value;

    while (position != std::string::npos)
    {
        size_t next = string.find(splitOn, position);
        if (next == std::string::npos) { break; }

        tokens.push_back(string.substr(position, (next - position)));
        position = next + splitOn.length();
    }

    tokens.push_back(string.substr(position, string.size() - position));
    return tokens;
}

float string_to_float(const std::string& string)
{
    if (string.empty()) { return 0; }
    return strtof(string.c_str(), nullptr);
}
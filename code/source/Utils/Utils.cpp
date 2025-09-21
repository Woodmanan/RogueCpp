#include "Utils.h"
#include <string_view>

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

float optional_string_to_float(std::vector<std::string>& tokens, int index, float defaultValue)
{
    if (index >= 0 && index < tokens.size() && !tokens[index].empty())
    {
        return strtof(tokens[index].c_str(), nullptr);
    }

    return defaultValue;
}

int HexToInt(char hex)
{
	switch (hex)
	{
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	case 'a':
	case 'A':
		return 10;
	case 'b':
	case 'B':
		return 11;
	case 'c':
	case 'C':
		return 12;
	case 'd':
	case 'D':
		return 13;
	case 'e':
	case 'E':
		return 14;
	case 'f':
	case 'F':
		return 15;
	default:
		return -1;
	}
}

bool HexToInt(char hexOne, char hexTwo, int& value)
{
	int upperVal = HexToInt(hexOne);
	int lowerVal = HexToInt(hexTwo);

	if (lowerVal == -1 || upperVal == -1)
	{
		return false;
	}

	value = (upperVal << 4) | lowerVal;
	return true;
}

Color string_to_color(const std::string& string)
{
    std::string_view view = string;
    if (view.length() == 7 && view[0] == '#')
    {
        view = view.substr(1, 6);
    }

    if (view.length() != 6)
    {
        return Color(255, 255, 0);
    }

	int r, g, b;
	if (HexToInt(view[0], view[1], r) &&
		HexToInt(view[2], view[3], g) &&
		HexToInt(view[4], view[5], b))
	{
		return Color((uchar)r, (uchar)g, (uchar)b);
	}

	return Color(255, 255, 0);
}
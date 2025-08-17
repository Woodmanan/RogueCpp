#pragma once
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>

inline void SkipLine(std::ifstream& stream)
{
	std::string line;
	std::getline(stream, line);
}
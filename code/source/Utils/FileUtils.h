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

inline std::filesystem::path GetExecutableFolder()
{
	//TODO: Check that this works on windows as well!
	return std::filesystem::canonical("/proc/self/exe").parent_path();
}

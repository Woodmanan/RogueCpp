#pragma once
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>

void SkipLine(ifstream& stream)
{
	std::string line;
	std::getline(stream, line);
}

std::vector<std::string> GetLineTokens(ifstream& stream)
{

}
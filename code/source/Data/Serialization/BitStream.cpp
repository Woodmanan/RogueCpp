#include "BitStream.h"
#include "Debug/Debug.h"
#include <charconv>
#include <stdlib.h>
#include <iostream>

void PackedStream::Write(const char* ptr, size_t length)
{
	data.insert(data.end(), ptr, ptr + length);
}

void PackedStream::Read(char* ptr, size_t length)
{
	ASSERT(readPos + length <= data.size());
	memcpy(ptr, data.data() + readPos, length);
	readPos += length;
}

PackedFileStream::PackedFileStream(std::filesystem::path path, bool in)
{
	if (in)
	{
		stream = std::fstream(path, std::ios::in | std::ios::binary);
	}
	else
	{
		stream = std::fstream(path, std::ios::out | std::ios::binary);
	}
}

void PackedFileStream::Close()
{
	stream.close();
}

void PackedFileStream::Write(const char* ptr, size_t length)
{
	stream.write(ptr, length);
}

void PackedFileStream::Read(char* ptr, size_t length)
{
	stream.read(ptr, length);
}

void JSONStream::BeginWrite(const char* name)
{
	WriteSpacing();
	Write(name, strlen(name));
	Write(": ", 2);
}

void JSONStream::FinishWrite()
{
	Write(",\n", 2);
}

void JSONStream::OpenWriteScope()
{
	Write("{\n", 2);
	AddSpacing();
}

void JSONStream::CloseWriteScope()
{
	RemoveSpacing();
	WriteSpacing();
	Write("}", 1);
}

void JSONStream::WriteSpacing()
{
	for (int i = 0; i < tabs; i++)
	{
#ifdef UseSpacesNotTabs
		Write("    ", 4);
#else
		Write('\t');
#endif
	}
}

void JSONStream::WriteListSeperator()
{
	Write(",\n", 2);
}

void JSONStream::BeginRead(const char* name)
{
	ReadSpacing();
	Skip(strlen(name));
	Skip(2); //Read in the ': '
}

void JSONStream::FinishRead()
{
	Skip(2);
}

void JSONStream::OpenReadScope()
{
	Skip(2);
	AddSpacing();
}

void JSONStream::CloseReadScope()
{
	RemoveSpacing();
	ReadSpacing();
	Skip(1);
}

void JSONStream::ReadSpacing()
{
#ifdef UseSpacesNotTabs
	Skip(tabs * 4);
#else
	Skip(tabs);
#endif
}

void JSONStream::ReadListSeperator()
{
	Skip(2);
}

void JSONStream::AddSpacing()
{
	tabs++;
}

void JSONStream::RemoveSpacing()
{
	ASSERT(tabs > 0);
	tabs--;
}

void JSONStream::Skip(int characters)
{
	readPos += characters;
}

//Write
void JSONStream::Write(const char* ptr, size_t length)
{
	data.insert(data.end(), ptr, ptr + length);
}

void JSONStream::WriteRawBytes(const char* ptr, size_t length)
{
	for (size_t i = 0; i < length; i++)
	{
		if (i % 10 == 0) WriteSpacing();

		char asChar = ptr[i];
		unsigned char uChar = asChar;
		int val = (int)uChar;
		ASSERT(val >= 0 && val <= 255);
		Write(IntToHex(val >> 4));
		Write(IntToHex(val & 0xF));

		if ((i + 1) == length || ((i + 1) % 10) == 0)
		{
			WriteListSeperator();
		}
		else
		{
			Write(' ');
		}
	}
}

void JSONStream::Write(bool value)
{
	if (value)
	{
		Write("true", 4);
	}
	else
	{
		Write("false", 5);
	}
}

void JSONStream::Write(char value)
{
	data.push_back(value);
}

void JSONStream::Write(unsigned char value)
{
	data.push_back(value);
}

void JSONStream::Write(short value)
{
	int asInt = value;
	Write(asInt);
}

void JSONStream::Write(int value)
{
	char buffer[16];
	std::fill_n(buffer, 16, '\0');
	std::to_chars(buffer, buffer + 16, value);
	Write(buffer, strlen(buffer));
}

void JSONStream::Write(unsigned int value)
{
	char buffer[16];
	std::fill_n(buffer, 16, '\0');
	std::to_chars(buffer, buffer + 16, value);
	Write(buffer, strlen(buffer));
}

void JSONStream::Write(float value)
{
	char buffer[32];
	std::fill_n(buffer, 32, '\0');
	std::to_chars(buffer, buffer + 32, value);
	Write(buffer, strlen(buffer));
}

void JSONStream::Write(size_t value)
{
	char buffer[32];
	std::fill_n(buffer, 32, '\0');
	std::to_chars(buffer, buffer + 32, value);
	Write(buffer, strlen(buffer));
}

void JSONStream::Write(std::string& value)
{
	char buffer[128];
	std::fill_n(buffer, 128, '\0');
	std::to_chars(buffer, buffer + 128, value.size());
	int pos = strlen(buffer);
	buffer[pos] = ':';
	buffer[pos + 1] = ' ';
	strncpy(buffer + pos + 2, value.c_str(), 126 - pos);
	Write(buffer, strlen(buffer));
}

//Reads
void JSONStream::Read(char* ptr, size_t length)
{
	ASSERT(readPos + length <= data.size());
	memcpy(ptr, data.data() + readPos, length);
	readPos += length;
}

void JSONStream::ReadRawBytes(char* ptr, size_t length)
{
	char str[3] = { 0 };
	for (size_t i = 0; i < length; i++)
	{
		if (i % 10 == 0) ReadSpacing();
		Read(str, 2);
		int val = (HexToInt(str[0]) << 4) | HexToInt(str[1]);
		ASSERT(val >= 0 && val <= 255);
		unsigned char uChar = (unsigned char)val;
		ptr[i] = uChar;

		if ((i + 1) == length || ((i + 1) % 10) == 0)
		{
			ReadListSeperator();
		}
		else
		{
			Skip(1);
		}
	}
}

void JSONStream::Read(bool& value)
{
	char buffer[8];
	ReadNextWordIntoBuffer(buffer, 8);

	if (strncmp(buffer, "true", 8) == 0)
	{
		value = true;
	}
	else
	{
		value = false;
	}
}

void JSONStream::Read(char& value)
{
	value = data[readPos];
	readPos++;
}

void JSONStream::Read(unsigned char& value)
{
	value = data[readPos];
	readPos++;
}

void JSONStream::Read(short& value)
{
	int asInt = 0;
	Read(asInt);

	value = (short) asInt;
}

void JSONStream::Read(int& value)
{
	char buffer[16];
	ReadNextWordIntoBuffer(buffer, 16);
	value = atoi(buffer);
}

void JSONStream::Read(unsigned int& value)
{
	char buffer[16];
	ReadNextWordIntoBuffer(buffer, 16);
	ASSERT(atoi(buffer) >= 0); //If this triggers, you need to rewrite this function
	value = strtoul(buffer, nullptr, 0);
}

void JSONStream::Read(float& value)
{
	char buffer[32];
	ReadNextWordIntoBuffer(buffer, 32);
	value = atof(buffer);
}

void JSONStream::Read(size_t& value)
{
	char buffer[32];
	ReadNextWordIntoBuffer(buffer, 32);
	value = strtoull(buffer, nullptr, 0);
}

void JSONStream::Read(std::string& value)
{
	int strSize = 0;
	Read(strSize);
	Skip(2);
	char buffer[128];
	ReadIntoBuffer(buffer, 128, strSize);
	buffer[strSize] = '\0';
	value = std::string(buffer, strSize);
}

void JSONStream::ReadNextWordIntoBuffer(char* buffer, int bufSize)
{
	int nextPos = 0;
	while (true)
	{
		if (nextPos >= bufSize - 1) { HALT(); } //Ended up reading more than is safe - needs a bigger buffer!

		if (readPos + nextPos >= data.size())
		{
			break;
		}

		char nextChar = data[readPos + nextPos];
		if (nextChar == ',' || nextChar == ':')
		{
			break;
		}

		nextPos++;
	}

	ReadIntoBuffer(buffer, bufSize, nextPos);
	buffer[nextPos] = '\0';
}

void JSONStream::ReadIntoBuffer(char* buffer, int bufSize, int numCharacters)
{
	ASSERT(numCharacters <= bufSize);
	memcpy(buffer, &data[readPos], numCharacters);
	readPos += numCharacters;
}

int JSONStream::HexToInt(char hex)
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
		return 10;
	case 'b':
		return 11;
	case 'c':
		return 12;
	case 'd':
		return 13;
	case 'e':
		return 14;
	case 'f':
		return 15;
	}

	HALT();
	return -1;
}

char JSONStream::IntToHex(int val)
{
	ASSERT(val >= 0 && val < 16);
	switch (val)
	{
	case 0:
		return '0';
	case 1:
		return '1';
	case 2:
		return '2';
	case 3:
		return '3';
	case 4:
		return '4';
	case 5:
		return '5';
	case 6:
		return '6';
	case 7:
		return '7';
	case 8:
		return '8';
	case 9:
		return '9';
	case 10:
		return 'a';
	case 11:
		return 'b';
	case 12:
		return 'c';
	case 13:
		return 'd';
	case 14:
		return 'e';
	case 15:
		return 'f';
	}

	HALT();
	return 'X';
}
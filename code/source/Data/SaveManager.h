#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include "Debug/Debug.h"

#ifdef _DEBUG
#define JSON
#endif

namespace RogueSaveManager {
	extern std::ofstream outStream;
	extern std::ifstream inStream;
	static int offset;
	const char* const tabString = "    ";
	extern char buffer[]; //Buffer for storing garbage while we read files

	const short version = 3;
	const char* const header = "RSFL";


#ifdef JSON
	const bool debug = true;
#else
	const bool debug = false;
#endif

	template <typename T>
	void Serialize(T& value)
	{
#ifdef JSON
		outStream << value;
#else
		ASSERT(outStream.is_open());
		char* bytePtr = (char*)&value;
		outStream.write(bytePtr, sizeof(T));
#endif
	}

	template <typename T>
	void Deserialize(T& value)
	{
#ifdef JSON
		inStream >> value;
#else
		ASSERT(inStream.is_open());
		char* bytePtr = (char*)&value;
		inStream.read(bytePtr, sizeof(T));
#endif
	}

	void WriteTabs();
	void ReadTabs();
	void WriteNewline();
	void ReadNewline();
	void AddOffset();
	void RemoveOffset();
	void AddListSeparator(bool isLast);
	void RemoveListSeparator(bool isLast);

	int HexToInt(char hex);
	char IntToHex(int val);

	template <typename T>
	static void Write(const char* name, T value)
	{
		ASSERT(outStream.is_open());
#ifdef JSON
		WriteTabs();
		outStream.write(name, strlen(name));
		outStream << " : ";
#endif // JSON

		Serialize(value);

#ifdef JSON
		outStream << ",\n";
#endif // JSON
	}

	template <typename T>
	static void Write(const char* name, std::vector<T> values)
	{
		ASSERT(outStream.is_open());
#ifdef JSON
		WriteTabs();
		outStream.write(name, strlen(name));
		outStream << " : ";
#endif // JSON
		AddOffset();
		Write("Count", values.size());
		WriteTabs();
		AddOffset();
		for (int i = 0; i < values.size(); i++)
		{
			WriteTabs();
			Serialize(values[i]);
			AddListSeparator(i == values.size() - 1);
		}
		RemoveOffset();
		WriteNewline();
		RemoveOffset();
#ifdef JSON
		outStream << ",\n";
#endif // JSON
	}

	template <typename T>
	static void Read(const char* name, T& value)
	{
		ASSERT(inStream.is_open());

#ifdef JSON
		ReadTabs();
		inStream.read(buffer, strlen(name));
		inStream.read(buffer, 3);
#endif // JSON

		Deserialize(value);

#ifdef JSON
		inStream.read(buffer, 2);
#endif // JSON
	}

	template <typename T>
	static void Read(const char* name, std::vector<T>& values)
	{
		ASSERT(inStream.is_open());

#ifdef JSON
		ReadTabs();
		inStream.read(buffer, strlen(name));
		inStream.read(buffer, 3);
#endif // JSON
		AddOffset();
		size_t size;
		Read("Count", size);
		values.resize(size);
		ReadTabs();
		AddOffset();
		for (int i = 0; i < size; i++)
		{
			ReadTabs();
			Deserialize(values[i]);
			RemoveListSeparator(i == values.size() - 1);
		}
		RemoveOffset();
		ReadNewline();
		RemoveOffset();

#ifdef JSON
		inStream.read(buffer, 2);
#endif // JSON
	}

	template <typename T>
	static T Read(const char* name)
	{
		ASSERT(inStream.is_open());

		T hold;
		Read(name, hold);
		return hold;
	}

	static void Write(const char* name, std::string& str)
	{
		ASSERT(outStream.is_open());

#ifdef JSON
		WriteTabs();
		outStream.write(name, strlen(name));
		outStream << " : ";
#endif // JSON

		//Serialize size
		size_t size = str.size();
#ifdef JSON
		outStream << size;
		outStream << ": ";
#else
		char* bytePtr = (char*)&size;
		outStream.write(bytePtr, sizeof(size_t));
#endif

		outStream.write(str.c_str(), str.size());

#ifdef JSON
		outStream << ",\n";
#endif // JSON
	}

	static void Read(const char* name, std::string& str)
	{
		ASSERT(inStream.is_open());

#ifdef JSON
		ReadTabs();
		inStream.read(buffer, strlen(name));
		inStream.read(buffer, 3);
#endif // JSON

		//Serialize Size
		size_t size;
#ifdef JSON
		inStream >> size;
		inStream.read(buffer, 2);
#else
		char* bytePtr = (char*)&size;
		inStream.read(bytePtr, sizeof(size_t));
#endif

		inStream.read(buffer, size);
		str = std::string(buffer, size);

#ifdef JSON
		inStream.read(buffer, 2);
#endif
	}

	static void WriteRawBuffer(char* data, size_t len)
	{
#ifdef JSON
		for (size_t i = 0; i < len; i++)
		{
			if (i % 10 == 0) WriteTabs();

			char asChar = data[i];
			unsigned char uChar = asChar;
			int val = (int)uChar;
			ASSERT(val >= 0 && val <= 255);
			outStream << IntToHex(val >> 4) << IntToHex(val & 0xF);

			if ((i + 1) == len || ((i + 1) % 10) == 0)
			{
				WriteNewline();
			}
			else
			{
				outStream << ' ';
			}
		}
#else
		outStream.write(data, len);
#endif
	}

	template <typename T>
	static void WriteAsBuffer(const char* name, std::vector<T> values)
	{
		ASSERT(outStream.is_open());
#ifdef JSON
		WriteTabs();
		outStream.write(name, strlen(name));
		outStream << " : ";
#endif // JSON
		AddOffset();
		Write("Size", values.size());
		WriteTabs();
#ifdef JSON
		outStream.write("Contents", strlen("Contents"));
		outStream << " : ";
#endif // JSON
		AddOffset();
		WriteRawBuffer((char*)values.data(), values.size() * sizeof(T));
		RemoveOffset();
		WriteNewline();
		RemoveOffset();
#ifdef JSON
		outStream << ",\n";
#endif // JSON
	}

	static void ReadRawBuffer(char* data, size_t len)
	{
#ifdef JSON
		char str[3] = { 0 };
		for (size_t i = 0; i < len; i++)
		{
			if (i % 10 == 0) ReadTabs();
			inStream.read(str, 2);
			int val = (HexToInt(str[0]) << 4) | HexToInt(str[1]);
			ASSERT(val >= 0 && val <= 255);
			unsigned char uChar = (unsigned char)val;
			data[i] = uChar;

			if ((i + 1) == len || ((i + 1) % 10) == 0)
			{
				ReadNewline();
			}
			else
			{
				inStream.read(str, 1);
			}
		}
#else
		inStream.read(data, len);
#endif
	}

	template <typename T>
	static void ReadAsBuffer(const char* name, std::vector<T>& values)
	{
		ASSERT(inStream.is_open());

#ifdef JSON
		ReadTabs();
		inStream.read(buffer, strlen(name));
		inStream.read(buffer, 3);
#endif // JSON
		AddOffset();
		size_t size;
		Read("Size", size);
		values.resize(size);
		ReadTabs();
#ifdef JSON
		inStream.read(buffer, strlen("Contents"));
		inStream.read(buffer, 3);
#endif // JSON
		AddOffset();
		ReadRawBuffer((char*) values.data(), size * sizeof(T));
		RemoveOffset();
		ReadNewline();
		RemoveOffset();

#ifdef JSON
		inStream.read(buffer, 2);
#endif // JSON
	}

	static void OpenWriteSaveFileByPath(const std::filesystem::path path)
	{
		ASSERT(!inStream.is_open());
		offset = 0;
#ifdef JSON
		outStream.open(path);
#else
		outStream.open(path, std::ios::binary);
#endif
		ASSERT(outStream.is_open());
		for (int i = 0; i < strlen(header); i++)
		{
			Serialize(header[i]);
		}
		WriteNewline();
		Write("Version", version);
	}

	static bool FileExists(const std::string filename)
	{
		return std::filesystem::exists(filename);
	}

	static bool FilePathExists(const std::filesystem::path path)
	{
		return std::filesystem::exists(path);
	}

	static void OpenWriteSaveFile(const std::string filename)
	{
		OpenWriteSaveFileByPath(std::filesystem::path(filename));
	}

	static void CloseWriteSaveFile()
	{
		ASSERT(outStream.is_open());
		outStream.close();
	}

	static bool OpenReadSaveFileByPath(const std::filesystem::path path)
	{
		if (!FilePathExists(path))
		{
			return false;
		}

		ASSERT(!outStream.is_open());
		offset = 0;
#ifdef JSON
		inStream.open(path);
#else
		inStream.open(path, std::ios::binary);
#endif
		ASSERT(inStream.is_open());
		char buf[5];
		for (int i = 0; i < strlen(header); i++)
		{
			Deserialize(buf[i]);
		}

		ReadNewline();
		if (strncmp(buf, header, 4) != 0)
		{
			return false;
		}
		short fileVersion;
		Read("Version", fileVersion);
		if (fileVersion != version)
		{
			return false;
		}

		return true;
	}

	static bool OpenReadSaveFile(const std::string filename)
	{
		return OpenReadSaveFileByPath(std::filesystem::path(filename));
	}

	static void CloseReadSaveFile()
	{
		ASSERT(inStream.is_open());
		inStream.close();
	}

	static void DeleteSaveFile(const std::string filename)
	{
		std::filesystem::remove(filename);
	}
};
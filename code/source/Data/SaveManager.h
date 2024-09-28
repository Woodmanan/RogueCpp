#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include "../Debug/Debug.h"

#ifdef _DEBUG
#define JSON
#endif

namespace RogueSaveManager {
	extern std::ofstream outStream;
	extern std::ifstream inStream;
	static int offset;
	const char* const tabString = "    ";
	extern char buffer[]; //Buffer for storing garbage while we read files

	const short version = 2;
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

	static void WriteBits(const char* name, char* bytes, int count)
	{
		ASSERT(outStream.is_open());
#ifdef JSON
		WriteTabs();
		outStream.write(name, strlen(name));
		outStream << " : ";
#endif // JSON

		outStream.write(bytes, count);

#ifdef JSON
		outStream << ",\n";
#endif // JSON
	}

	static void ReadBits(const char* name, char* bytes, int count)
	{
		ASSERT(inStream.is_open());

#ifdef JSON
		ReadTabs();
		inStream.read(buffer, strlen(name));
		inStream.read(buffer, 3);
#endif // JSON

		inStream.read(bytes, count);

#ifdef JSON
		inStream.read(buffer, 2);
#endif // JSON
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

	static void OpenWriteSaveFile(const std::string filename)
	{
		OpenWriteSaveFileByPath(std::filesystem::path(filename));
	}

	static void CloseWriteSaveFile()
	{
		ASSERT(outStream.is_open());
		outStream.close();
	}

	static void OpenReadSaveFileByPath(const std::filesystem::path path)
	{
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
			HALT();
		}
		short fileVersion;
		Read("Version", fileVersion);
		if (fileVersion != version)
		{
			HALT();
		}
	}

	static void OpenReadSaveFile(const std::string filename)
	{
		OpenReadSaveFileByPath(std::filesystem::path(filename));
	}

	static void CloseReadSaveFile()
	{
		ASSERT(inStream.is_open());
		inStream.close();
	}

	static bool FileExists(const std::string filename)
	{
		return std::filesystem::exists(filename);
	}

	static void DeleteSaveFile(const std::string filename)
	{
		std::filesystem::remove(filename);
	}
};
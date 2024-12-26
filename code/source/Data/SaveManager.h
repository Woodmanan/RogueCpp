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

	class SaveStreams
	{
	public:
		thread_local static std::ofstream outStream;
		thread_local static std::ifstream inStream;
		thread_local static int offset;
		thread_local static char buffer[];
	};
	
	const char* const tabString = "    ";

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
		SaveStreams::outStream << value;
#else
		ASSERT(SaveStreams::SaveStreams::outStream.is_open());
		char* bytePtr = (char*)&value;
		SaveStreams::outStream.write(bytePtr, sizeof(T));
#endif
	}

	template <typename T>
	void Deserialize(T& value)
	{
#ifdef JSON
		SaveStreams::inStream >> value;
#else
		ASSERT(SaveStreams::inStream.is_open());
		char* bytePtr = (char*)&value;
		SaveStreams::inStream.read(bytePtr, sizeof(T));
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
		ASSERT(SaveStreams::outStream.is_open());
#ifdef JSON
		WriteTabs();
		SaveStreams::outStream.write(name, strlen(name));
		SaveStreams::outStream << " : ";
#endif // JSON

		Serialize(value);

#ifdef JSON
		SaveStreams::outStream << ",\n";
#endif // JSON
	}

	template <typename T>
	static void Write(const char* name, std::vector<T> values)
	{
		ASSERT(SaveStreams::outStream.is_open());
#ifdef JSON
		WriteTabs();
		SaveStreams::outStream.write(name, strlen(name));
		SaveStreams::outStream << " : ";
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
		SaveStreams::outStream << ",\n";
#endif // JSON
	}

	template <typename T>
	static void Read(const char* name, T& value)
	{
		ASSERT(SaveStreams::inStream.is_open());

#ifdef JSON
		ReadTabs();
		SaveStreams::inStream.read(SaveStreams::buffer, strlen(name));
		SaveStreams::inStream.read(SaveStreams::buffer, 3);
#endif // JSON

		Deserialize(value);

#ifdef JSON
		SaveStreams::inStream.read(SaveStreams::buffer, 2);
#endif // JSON
	}

	template <typename T>
	static void Read(const char* name, std::vector<T>& values)
	{
		ASSERT(SaveStreams::inStream.is_open());

#ifdef JSON
		ReadTabs();
		SaveStreams::inStream.read(SaveStreams::buffer, strlen(name));
		SaveStreams::inStream.read(SaveStreams::buffer, 3);
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
		SaveStreams::inStream.read(SaveStreams::buffer, 2);
#endif // JSON
	}

	template <typename T>
	static T Read(const char* name)
	{
		ASSERT(SaveStreams::inStream.is_open());

		T hold;
		Read(name, hold);
		return hold;
	}

	static void Write(const char* name, std::string& str)
	{
		ASSERT(SaveStreams::outStream.is_open());

#ifdef JSON
		WriteTabs();
		SaveStreams::outStream.write(name, strlen(name));
		SaveStreams::outStream << " : ";
#endif // JSON

		//Serialize size
		size_t size = str.size();
#ifdef JSON
		SaveStreams::outStream << size;
		SaveStreams::outStream << ": ";
#else
		char* bytePtr = (char*)&size;
		SaveStreams::outStream.write(bytePtr, sizeof(size_t));
#endif

		SaveStreams::outStream.write(str.c_str(), str.size());

#ifdef JSON
		SaveStreams::outStream << ",\n";
#endif // JSON
	}

	static void Read(const char* name, std::string& str)
	{
		ASSERT(SaveStreams::inStream.is_open());

#ifdef JSON
		ReadTabs();
		SaveStreams::inStream.read(SaveStreams::buffer, strlen(name));
		SaveStreams::inStream.read(SaveStreams::buffer, 3);
#endif // JSON

		//Serialize Size
		size_t size;
#ifdef JSON
		SaveStreams::inStream >> size;
		SaveStreams::inStream.read(SaveStreams::buffer, 2);
#else
		char* bytePtr = (char*)&size;
		SaveStreams::inStream.read(bytePtr, sizeof(size_t));
#endif

		SaveStreams::inStream.read(SaveStreams::buffer, size);
		str = std::string(SaveStreams::buffer, size);

#ifdef JSON
		SaveStreams::inStream.read(SaveStreams::buffer, 2);
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
			SaveStreams::outStream << IntToHex(val >> 4) << IntToHex(val & 0xF);

			if ((i + 1) == len || ((i + 1) % 10) == 0)
			{
				WriteNewline();
			}
			else
			{
				SaveStreams::outStream << ' ';
			}
		}
#else
		SaveStreams::outStream.write(data, len);
#endif
	}

	template <typename T>
	static void WriteAsBuffer(const char* name, std::vector<T> values)
	{
		ASSERT(SaveStreams::outStream.is_open());
#ifdef JSON
		WriteTabs();
		SaveStreams::outStream.write(name, strlen(name));
		SaveStreams::outStream << " : ";
#endif // JSON
		AddOffset();
		Write("Size", values.size());
		WriteTabs();
#ifdef JSON
		SaveStreams::outStream.write("Contents", strlen("Contents"));
		SaveStreams::outStream << " : ";
#endif // JSON
		AddOffset();
		WriteRawBuffer((char*)values.data(), values.size() * sizeof(T));
		RemoveOffset();
		WriteNewline();
		RemoveOffset();
#ifdef JSON
		SaveStreams::outStream << ",\n";
#endif // JSON
	}

	static void ReadRawBuffer(char* data, size_t len)
	{
#ifdef JSON
		char str[3] = { 0 };
		for (size_t i = 0; i < len; i++)
		{
			if (i % 10 == 0) ReadTabs();
			SaveStreams::inStream.read(str, 2);
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
				SaveStreams::inStream.read(str, 1);
			}
		}
#else
		SaveStreams::inStream.read(data, len);
#endif
	}

	template <typename T>
	static void ReadAsBuffer(const char* name, std::vector<T>& values)
	{
		ASSERT(SaveStreams::inStream.is_open());

#ifdef JSON
		ReadTabs();
		SaveStreams::inStream.read(SaveStreams::buffer, strlen(name));
		SaveStreams::inStream.read(SaveStreams::buffer, 3);
#endif // JSON
		AddOffset();
		size_t size;
		Read("Size", size);
		values.resize(size);
		ReadTabs();
#ifdef JSON
		SaveStreams::inStream.read(SaveStreams::buffer, strlen("Contents"));
		SaveStreams::inStream.read(SaveStreams::buffer, 3);
#endif // JSON
		AddOffset();
		ReadRawBuffer((char*) values.data(), size * sizeof(T));
		RemoveOffset();
		ReadNewline();
		RemoveOffset();

#ifdef JSON
		SaveStreams::inStream.read(SaveStreams::buffer, 2);
#endif // JSON
	}

	static void OpenWriteSaveFileByPath(const std::filesystem::path path)
	{
		ASSERT(!SaveStreams::inStream.is_open());
		SaveStreams::offset = 0;
#ifdef JSON
		SaveStreams::outStream.open(path);
#else
		SaveStreams::outStream.open(path, std::ios::binary);
#endif
		ASSERT(SaveStreams::outStream.is_open());
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
		ASSERT(SaveStreams::outStream.is_open());
		SaveStreams::outStream.close();
	}

	static bool OpenReadSaveFileByPath(const std::filesystem::path path)
	{
		if (!FilePathExists(path))
		{
			return false;
		}

		ASSERT(!SaveStreams::outStream.is_open());
		SaveStreams::offset = 0;
#ifdef JSON
		SaveStreams::inStream.open(path);
#else
		SaveStreams::inStream.open(path, std::ios::binary);
#endif
		ASSERT(SaveStreams::inStream.is_open());
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
		ASSERT(SaveStreams::inStream.is_open());
		SaveStreams::inStream.close();
	}

	static void DeleteSaveFile(const std::string filename)
	{
		std::filesystem::remove(filename);
	}
};
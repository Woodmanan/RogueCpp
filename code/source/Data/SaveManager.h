#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include "Debug/Debug.h"
#include "Data/Serialization/BitStream.h"
#include "Data/Serialization/Serialization.h"

#ifdef _DEBUG
#define JSON
#endif

namespace RogueSaveManager {

#ifdef JSON
	typedef JSONStream SaveStreamType;
#else
	typedef PackedStream SaveStreamType;
#endif

	class SaveStreams
	{
	public:
		thread_local static std::ofstream outStream;
		thread_local static std::ifstream inStream;
		thread_local static SaveStreamType stream;
	};
	
	const short version = 3;
	const char* const header = "RSFL";


	template <typename T>
	static void Write(const char* name, T value)
	{
		ASSERT(SaveStreams::outStream.is_open());
		Serialization::Write(SaveStreams::stream, name, value);
	}

	template <typename T>
	static void Read(const char* name, T& value)
	{
		ASSERT(SaveStreams::inStream.is_open());
		Serialization::Read(SaveStreams::stream, name, value);
	}

	template <typename T>
	static T Read(const char* name)
	{
		ASSERT(SaveStreams::inStream.is_open());
		return Serialization::Read(SaveStreams::stream, name);
	}

	template <typename T>
	static void WriteAsBuffer(const char* name, std::vector<T> values)
	{
		ASSERT(SaveStreams::outStream.is_open());
		Serialization::WriteRawBytes(SaveStreams::stream, name, values);
	}

	template <typename T>
	static void ReadAsBuffer(const char* name, std::vector<T>& values)
	{
		ASSERT(SaveStreams::inStream.is_open());
		Serialization::ReadRawBytes(SaveStreams::stream, name, values);
	}

	static void OpenWriteSaveFileByPath(const std::filesystem::path path)
	{
		ASSERT(!SaveStreams::inStream.is_open());
#ifdef JSON
		SaveStreams::outStream.open(path);
#else
		SaveStreams::outStream.open(path, std::ios::binary);
#endif
		ASSERT(SaveStreams::outStream.is_open());
		SaveStreams::stream = SaveStreamType();

		SaveStreams::stream.Write(header, 4);
		SaveStreams::stream.FinishWrite();
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
		std::vector<char>& data = SaveStreams::stream.GetData();
		SaveStreams::outStream.write(data.data(), data.size());
		SaveStreams::outStream.close();
	}

	static bool OpenReadSaveFileByPath(const std::filesystem::path path)
	{
		if (!FilePathExists(path))
		{
			return false;
		}

		ASSERT(!SaveStreams::outStream.is_open());
#ifdef JSON
		SaveStreams::inStream.open(path, std::ios::ate);
#else
		SaveStreams::inStream.open(path, std::ios::binary | std::ios::ate);
#endif
		ASSERT(SaveStreams::inStream.is_open());

		std::streamsize size = SaveStreams::inStream.tellg();
		SaveStreams::inStream.seekg(0, std::ios::beg);
		SaveStreams::stream = SaveStreamType();
		std::vector<char>& data = SaveStreams::stream.GetData();
		data.resize(size);
		SaveStreams::inStream.read(data.data(), size);

		char buf[5];
		SaveStreams::stream.Read(buf, 4);
		SaveStreams::stream.FinishRead();
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
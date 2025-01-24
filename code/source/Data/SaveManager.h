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
#else
#define PACKED_FILE
#endif

namespace RogueSaveManager {

#if defined(JSON)
	typedef JSONStream SaveStreamType;
#elif defined(PACKED)
	typedef PackedStream SaveStreamType;
#elif defined(PACKED_FILE)
	typedef PackedFileStream SaveStreamType;
#endif

	class SaveStreams
	{
	public:
#if defined(JSON) || defined(PACKED)
		thread_local static std::ofstream outStream;
		thread_local static std::ifstream inStream;
#endif
		thread_local static SaveStreamType stream;
	};
	
	const short version = 4;
	const char* const header = "RSFL";


	template <typename T>
	static void Write(const char* name, T value)
	{
		Serialization::Write(SaveStreams::stream, name, value);
	}

	template <typename T>
	static void Read(const char* name, T& value)
	{
		Serialization::Read(SaveStreams::stream, name, value);
	}

	template <typename T>
	static T Read(const char* name)
	{
		return Serialization::Read(SaveStreams::stream, name);
	}

	template <typename T>
	static void WriteAsBuffer(const char* name, std::vector<T> values)
	{
		Serialization::WriteRawBytes(SaveStreams::stream, name, values);
	}

	template <typename T>
	static void ReadAsBuffer(const char* name, std::vector<T>& values)
	{
		Serialization::ReadRawBytes(SaveStreams::stream, name, values);
	}

	static void OpenWriteSaveFileByPath(const std::filesystem::path path)
	{
#ifdef JSON
			ASSERT(!SaveStreams::inStream.is_open());
			SaveStreams::outStream.open(path);
			SaveStreams::stream = SaveStreamType();

			ASSERT(SaveStreams::outStream.is_open());
#endif

#ifdef PACKED
			ASSERT(!SaveStreams::inStream.is_open());
			SaveStreams::outStream.open(path, std::ios::binary);
			SaveStreams::stream = SaveStreamType();

			ASSERT(SaveStreams::outStream.is_open());
#endif

#ifdef PACKED_FILE
			SaveStreams::stream = SaveStreamType(path, false);
#endif

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
#if (defined(JSON) || defined(PACKED))
			ASSERT(SaveStreams::outStream.is_open());
			std::vector<char>& data = SaveStreams::stream.GetData();
			SaveStreams::outStream.write(data.data(), data.size());
			SaveStreams::outStream.close();
#elif defined(PACKED_FILE)
			SaveStreams::stream.Close();
#endif
	}

	static bool OpenReadSaveFileByPath(const std::filesystem::path path)
	{
		if (!FilePathExists(path))
		{
			return false;
		}

#ifdef JSON
		ASSERT(!SaveStreams::outStream.is_open());
		SaveStreams::inStream.open(path, std::ios::ate);
		ASSERT(SaveStreams::inStream.is_open());

		std::streamsize size = SaveStreams::inStream.tellg();
		SaveStreams::inStream.seekg(0, std::ios::beg);
		SaveStreams::stream = SaveStreamType();
		std::vector<char>& data = SaveStreams::stream.GetData();
		data.resize(size);
		SaveStreams::inStream.read(data.data(), size);
#endif

#ifdef PACKED
		ASSERT(!SaveStreams::outStream.is_open());
		SaveStreams::inStream.open(path, std::ios::binary | std::ios::ate);
		ASSERT(SaveStreams::inStream.is_open());

		std::streamsize size = SaveStreams::inStream.tellg();
		SaveStreams::inStream.seekg(0, std::ios::beg);
		SaveStreams::stream = SaveStreamType();
		std::vector<char>& data = SaveStreams::stream.GetData();
		data.resize(size);
		SaveStreams::inStream.read(data.data(), size);
#endif

#ifdef PACKED_FILE
		SaveStreams::stream = SaveStreamType(path, true);
#endif

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
#if (defined(JSON) || defined(PACKED))
		ASSERT(SaveStreams::inStream.is_open());
		SaveStreams::inStream.close();
#elif defined(PACKED_FILE)
		SaveStreams::stream.Close();
#endif
	}

	static void DeleteSaveFile(const std::string filename)
	{
		std::filesystem::remove(filename);
	}
};
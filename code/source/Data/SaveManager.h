#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include "Debug/Debug.h"
#include "Debug/Profiling.h"
#include "Data/Serialization/BitStream.h"
#include "Data/Serialization/Serialization.h"

#ifdef _DEBUG
#define JSON
#else
#define PACKED
#endif

namespace RogueSaveManager {

#if defined(JSON)
	typedef JSONStream SaveStreamType;
#elif defined(PACKED)
	typedef PackedStream SaveStreamType;
#endif

	class Stream
	{
	public:
		static thread_local SaveStreamType stream;
	};
	
	const short version = 4;
	const char* const header = "RSFL";


	template <typename T>
	static void Write(const char* name, T value)
	{
		ROGUE_PROFILE_SECTION("File::Write");
		Serialization::Write(Stream::stream, name, value);
	}

	template <typename T>
	static void Read(const char* name, T& value)
	{
		ROGUE_PROFILE_SECTION("File::Read");
		Serialization::Read(Stream::stream, name, value);
	}

	template <typename T>
	static T Read(const char* name)
	{
		return Serialization::Read(Stream::stream, name);
	}

	template <typename T>
	static void WriteAsBuffer(const char* name, std::vector<T> values)
	{
		ROGUE_PROFILE_SECTION("File::WriteBuffer");
		Serialization::WriteRawBytes(Stream::stream, name, values);
	}

	template <typename T>
	static void ReadAsBuffer(const char* name, std::vector<T>& values)
	{
		ROGUE_PROFILE_SECTION("File::ReadBuffer");
		Serialization::ReadRawBytes(Stream::stream, name, values);
	}

	static void OpenWriteSaveFileByPath(const std::filesystem::path path)
	{
		Stream::stream = SaveStreamType(path, true);

		Stream::stream.Write(header, 4);
		Stream::stream.FinishWrite();
		Write("Version", version);
	}

	static bool FileExists(const std::string filename)
	{
		return std::filesystem::exists(filename);
	}

	static bool FilePathExists(const std::filesystem::path path)
	{
		ROGUE_PROFILE_SECTION("Check File Path");
		return std::filesystem::exists(path);
	}

	static void OpenWriteSaveFile(const std::string filename)
	{
		OpenWriteSaveFileByPath(std::filesystem::path(filename));
	}

	static void CloseWriteSaveFile()
	{
		Stream::stream.Close();
	}

	static bool OpenReadSaveFileByPath(const std::filesystem::path path)
	{
		if (!FilePathExists(path))
		{
			return false;
		}

		Stream::stream = SaveStreamType(path, false);

		{
			ROGUE_PROFILE_SECTION("Check Header");
			char buf[5];
			Stream::stream.Read(buf, 4);
			Stream::stream.FinishRead();
			if (strncmp(buf, header, 4) != 0)
			{
				return false;
			}
		}

		{
			ROGUE_PROFILE_SECTION("Check Version");
			short fileVersion;
			Read("Version", fileVersion);
			if (fileVersion != version)
			{
				return false;
			}
		}

		return true;
	}

	static bool OpenReadSaveFile(const std::string filename)
	{
		return OpenReadSaveFileByPath(std::filesystem::path(filename));
	}

	static void CloseReadSaveFile()
	{
		ROGUE_PROFILE_SECTION("Close Read File");
		Stream::stream.Close();
	}

	static void DeleteSaveFile(const std::string filename)
	{
		std::filesystem::remove(filename);
	}
};
#pragma once
#include <Debug/Debug.h>
#include "include/magic_enum/magic_enum.hpp"
#include <vector>
#include <filesystem>
#include <fstream>
#include <memory>

#define UseSpacesNotTabs

/* Serialization streams! */
/*
* These streams are designed to play nicely with Serialization.h (in this folder).
* Formatting is handled by the serialization functions, but it is up to the streams
* to provide format functions and serialization for base types.
*/

struct DataBackend
{
	virtual ~DataBackend() {}
	virtual void Write(const char* ptr, size_t length) = 0;
	virtual void Read(char* ptr, size_t length) = 0;
	virtual bool HasNextChar() = 0;
	virtual char Peek() = 0;
	virtual void Close() = 0;
};

struct FileBackend : public DataBackend
{
	FileBackend(std::filesystem::path path, bool write);
	virtual ~FileBackend();
	void Write(const char* ptr, size_t length) override;
	void Read(char* ptr, size_t length) override;
	bool HasNextChar() override;
	char Peek() override;
	void Close() override;

	std::fstream m_stream;
};

struct VectorBackend : public DataBackend
{
	VectorBackend() {}
	virtual ~VectorBackend() {}
	void Write(const char* ptr, size_t length) override;
	void Read(char* ptr, size_t length) override;
	bool HasNextChar() override;
	char Peek() override;
	void Close() override {}

	std::vector<char> m_data;
	int m_readPos = 0;
};

class PackedStream
{
public:
	PackedStream();
	PackedStream(std::filesystem::path path, bool write);

	void BeginWrite(const char* name) {}
	void FinishWrite() {}
	void OpenWriteScope() {}
	void CloseWriteScope() {}
	void WriteSpacing() {}
	void WriteListSeperator() {}
	void AllWritesFinished();

	void BeginRead(const char* name) {}
	void FinishRead() {}
	void OpenReadScope() {}
	void CloseReadScope() {}
	void ReadSpacing() {}
	void ReadListSeperator() {}

	void Close();

	void Write(const char* ptr, size_t length);
	void Read(char* ptr, size_t length);

	void WriteBits(const char* word, int bits);
	void WriteScratchBit();
	void WriteAlign();

	void ReadBits(char* ptr, int bits);
	void ReadScratchBit();
	void ReadAlign();

	void WriteRawBytes(const char* ptr, size_t length)
	{
		WriteAlign();
		m_backend->Write(ptr, length);
	}

	void ReadRawBytes(char* ptr, size_t length)
	{
		ReadAlign();
		m_backend->Read(ptr, length);
	}

	template<typename E>
	void WriteEnum(const E& value);

	template<typename E>
	void ReadEnum(E& value);

	template<typename T>
	void Write(const T& value);

	template<typename T>
	void Read(T& value);

	std::shared_ptr<DataBackend> GetDataBackend()
	{
		return m_backend;
	}

	uint32_t m_scratch = 0;
	int m_scratchBits = 0;

protected:
	std::shared_ptr<DataBackend> m_backend;
};

template<typename E>
void PackedStream::WriteEnum(const E& value)
{
	auto asInteger = magic_enum::enum_integer(value);
	Write(asInteger);
}

template<typename E>
void PackedStream::ReadEnum(E& value)
{
	int asInteger;
	Read(asInteger);
	auto enumValue = magic_enum::enum_cast<E>(asInteger);
	ASSERT(enumValue.has_value());
	value = enumValue.value();
}

template<typename T>
void PackedStream::Write(const T& value)
{
	char* bytePtr = (char*)&value;
	Write(bytePtr, sizeof(T));
}

template<>
inline void PackedStream::Write(const bool& value)
{
	char* bytePtr = (char*)&value;
	WriteBits(bytePtr, 1);
}

template<>
inline void PackedStream::Write(const int& value)
{
	bool fitsInInt8 = (char) (INT8_MIN <= value && value <= INT8_MAX);
	Write(fitsInInt8);

	if (fitsInInt8)
	{
		char asChar = value;
		Write(&asChar, 1);
	}
	else
	{
		char* bytePtr = (char*)&value;
		Write(bytePtr, 4);
	}
}

template<>
inline void PackedStream::Write(const std::string& value)
{
	size_t size = value.size();
	ASSERT(size <= 128);
	Write(size);
	Write(value.c_str(), size);
}

template<typename T>
void PackedStream::Read(T& value)
{
	char* bytePtr = (char*)&value;
	Read(bytePtr, sizeof(T));
}

template<>
inline void PackedStream::Read(bool& value)
{
	char* bytePtr = (char*)&value;
	ReadBits(bytePtr, 1);
}

template<>
inline void PackedStream::Read(int& value)
{

	bool fitsInInt8;
	Read(fitsInInt8);

	if (fitsInInt8)
	{
		char asChar;
		Read(&asChar, 1);
		value = (int) asChar;
	}
	else
	{
		char* bytePtr = (char*)&value;
		Read(bytePtr, 4);
	}
}

template<>
inline void PackedStream::Read(std::string& value)
{
	size_t size = 0;
	char buffer[128];
	Read(size);
	ASSERT(size <= 100);
	Read(buffer, size);
	value = std::string(buffer, size);
}

class JSONStream
{
public:
	JSONStream();
	JSONStream(std::filesystem::path path, bool write);

	void BeginWrite(const char* name);
	void FinishWrite();
	void OpenWriteScope();
	void CloseWriteScope();
	void WriteSpacing();
	void WriteListSeperator();
	void AllWritesFinished() {}

	void BeginRead(const char* name);
	void FinishRead();
	void OpenReadScope();
	void CloseReadScope();
	void ReadSpacing();
	void ReadListSeperator();

	void AddSpacing();
	void RemoveSpacing();
	void Skip(int characters);

	//Write
	void Write(const char* ptr, size_t length);
	void WriteRawBytes(const char* ptr, size_t length);
	void Write(const bool value);
	void Write(const char value);
	void Write(const unsigned char value);
	void Write(const short value);
	void Write(const int value);
	void Write(const unsigned int value);
	void Write(const float value);
	void Write(const size_t value);
	void Write(const std::string& value);
	void Write(const std::string_view& value);

	//Reads
	void Read(char* ptr, size_t length);
	void ReadRawBytes(char* ptr, size_t length);
	void Read(bool& value);
	void Read(char& value);
	void Read(unsigned char& value);
	void Read(short& value);
	void Read(int& value);
	void Read(unsigned int& value);
	void Read(float& value);
	void Read(size_t& value);
	void Read(std::string& value);

	template<typename E>
	void WriteEnum(const E& value);

	template<typename E>
	void ReadEnum(E& value);

	void Close();

	std::shared_ptr<DataBackend> GetDataBackend()
	{
		return m_backend;
	}

private:
	void ReadNextWordIntoBuffer(char* buffer, int bufSize);

	int HexToInt(char hex);
	char IntToHex(int val);

	int tabs = 0;
	std::shared_ptr<DataBackend> m_backend;
};

template<typename E>
void JSONStream::WriteEnum(const E& value)
{
	std::string_view enum_name = magic_enum::enum_name(value);
	Write(enum_name);
}

template<typename E>
void JSONStream::ReadEnum(E& value)
{
	string enum_name;
	Read(enum_name);

	value = magic_enum::enum_cast<E>(enum_name, magic_enum::case_insensitive).value();
}


#ifdef _DEBUG
typedef JSONStream DefaultStream;
#else
typedef PackedStream DefaultStream;
#endif
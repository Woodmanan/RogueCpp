#pragma once
#include <Debug/Debug.h>
#include <vector>
#include <filesystem>

#define UseSpacesNotTabs

/* Serialization streams! */
/*
* These streams are designed to play nicely with Serialization.h (in this folder).
* Formatting is handled by the serialization functions, but it is up to the streams
* to provide format functions and serialization for base types.
*/

class PackedStream
{
public:
	void BeginWrite(const char* name) {}
	void FinishWrite() {}
	void OpenWriteScope() {}
	void CloseWriteScope() {}
	void WriteSpacing() {}
	void WriteListSeperator() {}

	void BeginRead(const char* name) {}
	void FinishRead() {}
	void OpenReadScope() {}
	void CloseReadScope() {}
	void ReadSpacing() {}
	void ReadListSeperator() {}

	void Write(const char* ptr, size_t length);
	void Read(char* ptr, size_t length);

	void WriteRawBytes(const char* ptr, size_t length)
	{
		Write(ptr, length);
	}

	void ReadRawBytes(char* ptr, size_t length)
	{
		Read(ptr, length);
	}

	template<typename T>
	void Write(T& value);

	template<typename T>
	void Read(T& value);

	std::vector<char>& GetData() { return data; }

private:
	int readPos = 0;
	std::vector<char> data;
};

template<typename T>
void PackedStream::Write(T& value)
{
	char* bytePtr = (char*)&value;
	Write(bytePtr, sizeof(T));
}

template<>
inline void PackedStream::Write(std::string& value)
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
	void BeginWrite(const char* name);
	void FinishWrite();
	void OpenWriteScope();
	void CloseWriteScope();
	void WriteSpacing();
	void WriteListSeperator();

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
	void Write(bool value);
	void Write(char value);
	void Write(unsigned char value);
	void Write(short value);
	void Write(int value);
	void Write(unsigned int value);
	void Write(float value);
	void Write(size_t value);
	void Write(std::string& value);

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

	std::vector<char>& GetData() { return data; }

private:
	void ReadNextWordIntoBuffer(char* buffer, int bufSize);
	void ReadIntoBuffer(char* buffer, int bufSize, int numCharacters);

	int HexToInt(char hex);
	char IntToHex(int val);

	int tabs = 0;
	int readPos = 0;
	std::vector<char> data;
};


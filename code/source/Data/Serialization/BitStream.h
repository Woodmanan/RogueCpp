#pragma once
#include <Debug/Debug.h>
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

	void BeginRead(const char* name) {}
	void FinishRead() {}
	void OpenReadScope() {}
	void CloseReadScope() {}
	void ReadSpacing() {}
	void ReadListSeperator() {}

	void Close();

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

protected:
	std::shared_ptr<DataBackend> m_backend;
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
	JSONStream();
	JSONStream(std::filesystem::path path, bool write);

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

	void Close();

private:
	void ReadNextWordIntoBuffer(char* buffer, int bufSize);

	int HexToInt(char hex);
	char IntToHex(int val);

	int tabs = 0;
	std::shared_ptr<DataBackend> m_backend;
};


#ifdef _DEBUG
typedef JSONStream DefaultStream;
#else
typedef PackedStream DefaultStream;
#endif
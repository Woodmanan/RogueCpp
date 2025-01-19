#pragma once
#include <vector>

namespace Serialization
{
	template<typename Stream, typename T>
	void Serialize(Stream& stream, T& value)
	{
		stream.Write(value);
	}

	template<typename Stream, typename T>
	void SerializeObject(Stream& stream, T& value)
	{
		stream.OpenWriteScope();
		Serialize(stream, value);
		stream.CloseWriteScope();
	}

	template<typename Stream, typename T>
	void Write(Stream& stream, const char* name, T& value)
	{
		stream.BeginWrite(name);
		SerializeObject(stream, value);
		stream.FinishWrite();
	}

	template<typename Stream, typename T>
	void WriteRawBytes(Stream& stream, const char* name, std::vector<T>& values)
	{
		stream.BeginWrite(name);
		stream.OpenWriteScope();
		size_t size = values.size();
		Write(stream, "Size", size);
		stream.BeginWrite("Data");
		stream.OpenWriteScope();
		stream.WriteRawBytes((char*)values.data(), values.size() * sizeof(T));
		stream.CloseWriteScope();
		stream.FinishWrite();
		stream.CloseWriteScope();
		stream.FinishWrite();
	}

	template<typename Stream, typename T>
	void Deserialize(Stream& stream, T& value)
	{
		stream.Read(value);
	}

	template<typename Stream, typename T>
	void DeserializeObject(Stream& stream, T& value)
	{
		stream.OpenReadScope();
		Deserialize(stream, value);
		stream.CloseReadScope();
	}

	template<typename Stream, typename T>
	void Read(Stream& stream, const char* name, T& value)
	{
		stream.BeginRead(name);
		DeserializeObject(stream, value);
		stream.FinishRead();
	}

	template<typename Stream, typename T>
	T Read(Stream& stream, const char* name)
	{
		T value;
		Read(stream, name, value);
		return value;
	}

	template<typename Stream, typename T>
	void ReadRawBytes(Stream& stream, const char* name, std::vector<T>& values)
	{
		stream.BeginRead(name);
		stream.OpenReadScope();
		size_t size;
		Read(stream, "Size", size);
		values.resize(size);
		stream.BeginRead("Data");
		stream.OpenReadScope();
		stream.ReadRawBytes((char*)values.data(), values.size() * sizeof(T));
		stream.CloseReadScope();
		stream.FinishRead();
		stream.CloseReadScope();
		stream.FinishRead();
	}

	//Vector specializations
	template<typename Stream, typename T>
	void Serialize(Stream& stream, std::vector<T>& values)
	{
		size_t size = values.size();
		Write(stream, "Size", size);
		stream.BeginWrite("Values");
		stream.OpenWriteScope();
		for (size_t i = 0; i < size; i++)
		{
			stream.WriteSpacing();
			SerializeObject(stream, values[i]);
			stream.WriteListSeperator();
		}
		stream.CloseWriteScope();
		stream.FinishWrite();
	}

	template<typename Stream, typename T>
	void Deserialize(Stream& stream, std::vector<T>& values)
	{
		size_t size;
		Read(stream, "Size", size);
		values.resize(size);
		stream.BeginRead("Values");
		stream.OpenReadScope();
		for (size_t i = 0; i < size; i++)
		{
			stream.ReadSpacing();
			DeserializeObject(stream, values[i]);
			stream.ReadListSeperator();
		}
		stream.CloseReadScope();
		stream.FinishRead();
	}

	//Base type specializations - these should not create object scopes.
	template<typename Stream>
	void SerializeObject(Stream& stream, bool& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, char& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, unsigned char& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, short& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, int& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, unsigned int& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, float& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, size_t& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, std::string& value)
	{
		Serialize(stream, value);
	}

	//Base type specializations - these should not create object scopes.
	template<typename Stream>
	void DeserializeObject(Stream& stream, bool& value)
	{
		Deserialize(stream, value);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, char& value)
	{
		Deserialize(stream, value);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, unsigned char& value)
	{
		Deserialize(stream, value);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, short& value)
	{
		Deserialize(stream, value);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, int& value)
	{
		Deserialize(stream, value);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, unsigned int& value)
	{
		Deserialize(stream, value);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, float& value)
	{
		Deserialize(stream, value);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, size_t& value)
	{
		Deserialize(stream, value);
	}

	template<typename Stream>
	void DeserializeObject(Stream& stream, std::string& value)
	{
		Deserialize(stream, value);
	}
}
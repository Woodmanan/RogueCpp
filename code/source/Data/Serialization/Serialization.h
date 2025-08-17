#pragma once
#include <vector>
#include <map>
#include <unordered_map>

namespace Serialization
{
	template<typename Stream, typename T>
	void Serialize(Stream& stream, const T& value)
	{
		stream.Write(value);
	}

	template<typename Stream, typename T>
	void SerializeObject(Stream& stream, const T& value)
	{
		stream.OpenWriteScope();
		Serialize(stream, value);
		stream.CloseWriteScope();
	}

	template<typename Stream, typename T>
	void Write(Stream& stream, const char* name, const T& value)
	{
		stream.BeginWrite(name);
		SerializeObject(stream, value);
		stream.FinishWrite();
	}

	template<typename Stream, typename T>
	void WriteRawBytes(Stream& stream, const char* name, const std::vector<T>& values)
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
	void Serialize(Stream& stream, const std::vector<T>& values)
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

	//Map specializations
	template<typename Stream, typename K, typename V>
	void Serialize(Stream& stream, const std::map<K, V>& values)
	{
		size_t size = values.size();
		Write(stream, "Size", size);
		stream.BeginWrite("Values");
		stream.OpenWriteScope();
		for (auto it : values)
		{
			stream.WriteSpacing();
			stream.OpenWriteScope();
			Write(stream, "Key", it.first);
			Write(stream, "Value", it.second);
			stream.CloseWriteScope();
			stream.WriteListSeperator();
		}
		stream.CloseWriteScope();
		stream.FinishWrite();
	}

	template<typename Stream, typename K, typename V>
	void Deserialize(Stream& stream, std::map<K, V>& values)
	{
		size_t size;
		Read(stream, "Size", size);
		stream.BeginRead("Values");
		stream.OpenReadScope();
		for (size_t i = 0; i < size; i++)
		{
			stream.ReadSpacing();
			stream.OpenReadScope();
			K key;
			V value;
			Read(stream, "Key", key);
			Read(stream, "Value", value);
			values[key] = value;
			stream.CloseReadScope();
			stream.ReadListSeperator();
		}
		stream.CloseReadScope();
		stream.FinishRead();
	}

	template<typename Stream, typename K, typename V>
	void Serialize(Stream& stream, const std::unordered_map<K, V>& values)
	{
		size_t size = values.size();
		Write(stream, "Size", size);
		stream.BeginWrite("Values");
		stream.OpenWriteScope();
		for (auto it : values)
		{
			stream.WriteSpacing();
			stream.OpenWriteScope();
			Write(stream, "Key", it.first);
			Write(stream, "Value", it.second);
			stream.CloseWriteScope();
			stream.WriteListSeperator();
		}
		stream.CloseWriteScope();
		stream.FinishWrite();
	}

	template<typename Stream, typename K, typename V>
	void Deserialize(Stream& stream, std::unordered_map<K, V>& values)
	{
		size_t size;
		Read(stream, "Size", size);
		stream.BeginRead("Values");
		stream.OpenReadScope();
		for (size_t i = 0; i < size; i++)
		{
			stream.ReadSpacing();
			stream.OpenReadScope();
			K key;
			V value;
			Read(stream, "Key", key);
			Read(stream, "Value", value);
			values[key] = value;
			stream.CloseReadScope();
			stream.ReadListSeperator();
		}
		stream.CloseReadScope();
		stream.FinishRead();
	}

	//Base type specializations - these should not create object scopes.
	template<typename Stream>
	void SerializeObject(Stream& stream, const bool& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, const char& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, const unsigned char& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, const short& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, const int& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, const unsigned int& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, const float& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, const size_t& value)
	{
		Serialize(stream, value);
	}

	template<typename Stream>
	void SerializeObject(Stream& stream, const std::string& value)
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
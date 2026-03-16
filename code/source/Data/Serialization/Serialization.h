#pragma once
#include <vector>
#include <map>
#include <unordered_map>

namespace Serialization
{	
	template<typename T>
	struct Serializer 
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const T& value) = delete;

		template<typename Stream>
		static void Deserialize(Stream& stream, T& value) = delete;

		template<typename Stream>
		static void SerializeObject(Stream& stream, const T& value) = delete;
		
		template<typename Stream>
		static void DeserializeObject(Stream& stream, T& value) = delete;
	};

	template<typename T>
	struct ObjectSerializer
	{
		template<typename Stream>
		static void SerializeObject(Stream& stream, const T& value)
		{
			stream.OpenWriteScope();
			Serializer<T>::Serialize(stream, value);
			stream.CloseWriteScope();
		}
	
		template<typename Stream>
		static void DeserializeObject(Stream& stream, T& value)
		{
			stream.OpenReadScope();
			Serializer<T>::Deserialize(stream, value);
			stream.CloseReadScope();
		}
	};

	template<typename T>
	struct SimpleSerializer
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const T& value)
		{
			stream.Write(value);
		}

		template<typename Stream>
    	static void Deserialize(Stream& stream, T& value)
		{
			stream.Read(value);
		}

		template<typename Stream>
		static void SerializeObject(Stream& stream, const T& value)
		{
			Serialize(stream, value);
		}
		
		template<typename Stream>
		static void DeserializeObject(Stream& stream, T& value)
		{
			Deserialize(stream, value);
		}
	};

	template<typename T>
	struct EnumSerializer
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const T& value)
		{
			stream.WriteEnum(value);
		}

		template<typename Stream>
    	static void Deserialize(Stream& stream, T& value)
		{
			stream.ReadEnum(value);
		}

		template<typename Stream>
		static void SerializeObject(Stream& stream, const T& value)
		{
			Serialize(stream, value);
		}
		
		template<typename Stream>
		static void DeserializeObject(Stream& stream, T& value)
		{
			Deserialize(stream, value);
		}
	};

	template<> struct Serializer<bool> : SimpleSerializer<bool> {};
	template<> struct Serializer<char> : SimpleSerializer<char> {};
	template<> struct Serializer<unsigned char> : SimpleSerializer<unsigned char> {};
	template<> struct Serializer<short> : SimpleSerializer<short> {};
	template<> struct Serializer<int> : SimpleSerializer<int> {};
	template<> struct Serializer<unsigned int> : SimpleSerializer<unsigned int> {};
	template<> struct Serializer<float> : SimpleSerializer<float> {};
	template<> struct Serializer<size_t> : SimpleSerializer<size_t> {};
	template<> struct Serializer<std::string> : SimpleSerializer<std::string> {};	

	template<typename Stream, typename T>
	void Write(Stream& stream, const char* name, const T& value)
	{
		stream.BeginWrite(name);
		Serializer<T>::SerializeObject(stream, value);
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
	void Read(Stream& stream, const char* name, T& value)
	{
		stream.BeginRead(name);
		Serializer<T>::DeserializeObject(stream, value);
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
	template<typename T>
	struct Serializer<std::vector<T>> : ObjectSerializer<std::vector<T>>
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const std::vector<T>& values)
		{
			size_t size = values.size();
			Write(stream, "Size", size);
			stream.BeginWrite("Values");
			stream.OpenWriteScope();
			for (size_t i = 0; i < size; i++)
			{
				stream.WriteSpacing();
				Serializer<T>::SerializeObject(stream, values[i]);
				stream.WriteListSeperator();
			}
			stream.CloseWriteScope();
			stream.FinishWrite();
		}

		template<typename Stream>
		static void Deserialize(Stream& stream, std::vector<T>& values)
		{
			size_t size;
			Read(stream, "Size", size);
			values.resize(size);
			stream.BeginRead("Values");
			stream.OpenReadScope();
			for (size_t i = 0; i < size; i++)
			{
				stream.ReadSpacing();
				Serializer<T>::DeserializeObject(stream, values[i]);
				stream.ReadListSeperator();
			}
			stream.CloseReadScope();
			stream.FinishRead();
		}
	};

	//Map specializations
	template<typename K, typename V>
	struct Serializer<std::map<K, V>> : ObjectSerializer<std::map<K, V>>
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const std::map<K, V>& values)
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

		template<typename Stream>
		static void Deserialize(Stream& stream, std::map<K, V>& values)
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
	};

	template<typename K, typename V>
	struct Serializer<std::unordered_map<K,V>> : ObjectSerializer<std::unordered_map<K,V>>
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const std::unordered_map<K, V>& values)
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

		template<typename Stream>
		static void Deserialize(Stream& stream, std::unordered_map<K, V>& values)
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
	};
}

#pragma once
#include "Debug/Debug.h"
#include "Data/Serialization/Serialization.h"

#include <array>
#include <initializer_list>


//Fixed size array with error checking and support for iterators. Automatically maintains sizing info for convenience.

template<class T, size_t N>
class FixedArray
{
public:
	FixedArray() {}
	FixedArray(size_t size, const T& value)
	{
		m_size = size;
		for (size_t i = 0; i < m_size; i++)
		{
			m_data[i] = value;
		}
	}
	FixedArray(std::initializer_list<T> initializer_list)
	{
		m_data = initializer_list;
		m_size = initializer_list.size();
	}

	T* begin()
	{
		return &m_data[0];
	}

	const T* begin() const
	{
		return &m_data[0];
	}

	T* end()
	{
		return begin() + m_size;
	}

	const T* end() const
	{
		return begin() + m_size;
	}

	T& last()
	{
		ASSERT(m_size > 0);
		return m_data[m_size - 1];
	}

	const T& last() const
	{
		ASSERT(m_size > 0);
		return m_data[m_size - 1];
	}

	void push_back(const T& value)
	{
		ASSERT(m_size < N);
		m_data[m_size] = value;
		m_size++;
	}

	void insert(const T& value, size_t index)
	{
		if (empty())
		{
			push_back(value);
		}
		else
		{
			ASSERT(m_size < N);

			for (uint32_t moveIndex = m_size - 1; moveIndex >= index; moveIndex--)
			{
				m_data[moveIndex + 1] = m_data[moveIndex];
			}

			m_data[index] = value;
			m_size++;
		}
	}

	void remove(size_t index)
	{
		ASSERT(index < m_size);
		for (uint32_t i = index; i < m_size - 1; i++)
		{
			m_data[i] = m_data[i + 1];
		}
		m_size--;
	}

	void clear()
	{
		m_size = 0;
	}

	void resize(size_t size)
	{
		m_size = size;
	}

	T& operator [] (size_t index)
	{
		ASSERT(index < m_size);
		return m_data[index];
	}

	const T& operator [] (size_t index) const
	{
		ASSERT(index < m_size);
		return m_data[index];
	}

	T* find(T& element) const
	{
		for (T* search = begin(); search != end(); search++)
		{
			if (*search == element)
			{
				return search;
			}
		}

		return nullptr;
	}

	T* find(std::function<bool(T&)> func)
	{
		for (T* search = begin(); search != end(); search++)
		{
			if (func(*search))
			{
				return search;
			}
		}

		return nullptr;
	}

	size_t size() const
	{
		return m_size;
	}

	bool empty() const
	{
		return size() == 0;
	}


private:
	std::array<T, N> m_data;
	size_t m_size = 0;
};

namespace Serialization
{
	template<typename T, size_t N>
	struct Serializer<FixedArray<T, N>> : ObjectSerializer<FixedArray<T, N>>
	{
		template<typename Stream>
		static void Serialize(Stream& stream, const FixedArray<T, N>& values)
		{
			size_t size = values.size();
			Write(stream, "Size", size);
			stream.BeginWrite("Values");
			stream.OpenWriteScope();
			for (size_t index = 0; index < values.size(); index++)
			{
				stream.WriteSpacing();
				Serializer<T>::SerializeObject(stream, values[index]);
				stream.WriteListSeperator();
			}
			stream.CloseWriteScope();
			stream.FinishWrite();
		}

		template<typename Stream>
		static void Deserialize(Stream& stream, FixedArray<T, N>& values)
		{
			size_t size = Read<Stream, size_t>(stream, "Size");
			values.resize(size);
			stream.BeginRead("Values");
			stream.OpenReadScope();
			for (size_t index = 0; index < size; index++)
			{
				stream.ReadSpacing();
				Serializer<T>::DeserializeObject(stream, values[index]);
				stream.ReadListSeperator();
			}
			stream.CloseReadScope();
			stream.FinishRead();
		}
	};
}

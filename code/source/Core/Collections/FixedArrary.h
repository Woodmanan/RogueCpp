#pragma once
#include "Debug/Debug.h"

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
		ASSERT(m_size < N);

		for (int moveIndex = m_size - 1; moveIndex >= index; moveIndex--)
		{
			m_data[moveIndex + 1] = m_data[moveIndex];
		}

		m_data[index] = value;
		m_size++;
	}

	void remove(size_t index)
	{
		ASSERT(index < m_size);
		for (int i = index; i < m_size - 1; i++)
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

	size_t size() const
	{
		return m_size;
	}

private:
	std::array<T, N> m_data;
	size_t m_size = 0;
};

namespace Serialization
{
	template<typename Stream, class T, size_t N>
	void Serialize(Stream& stream, const FixedArray<T, N>& values)
	{
		size_t size = values.size();
		Write(stream, "Size", size);
		stream.BeginWrite("Values");
		stream.OpenWriteScope();
		for (size_t index = 0; index < values.size(); index++)
		{
			stream.WriteSpacing();
			SerializeObject(stream, values[index]);
			stream.WriteListSeperator();
		}
		stream.CloseWriteScope();
		stream.FinishWrite();
	}

	template<typename Stream, class T, size_t N>
	void Deserialize(Stream& stream, FixedArray<T, N>& values)
	{
		size_t size = Read<Stream, size_t>(stream, "Size");
		values.resize(size);
		stream.BeginRead("Values");
		stream.OpenReadScope();
		for (int index = 0; index < size; index++)
		{
			stream.ReadSpacing();
			DeserializeObject(stream, values[index]);
			stream.ReadListSeperator();
		}
		stream.CloseReadScope();
		stream.FinishRead();
	}
}
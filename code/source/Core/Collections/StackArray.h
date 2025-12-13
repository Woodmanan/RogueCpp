#pragma once
#include <malloc.h>
#include "Core/CoreDataTypes.Forward.h"

template<typename T>
class StackArray
{
public:
	StackArray(T* buffer, uint size) : m_buffer(buffer), m_maxSize(size) {};

	T* begin()
	{
		return m_buffer;
	}

	const T* begin() const
	{
		return m_buffer;
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
		return m_buffer[m_size - 1];
	}

	const T& last() const
	{
		ASSERT(m_size > 0);
		return m_buffer[m_size - 1];
	}

	void push_back(const T& value)
	{
		ASSERT(m_size < m_maxSize);
		m_buffer[m_size] = value;
		m_size++;
	}

	void insert(const T& value, size_t index)
	{
		ASSERT(m_size < m_maxSize);

		for (int moveIndex = m_size - 1; moveIndex >= index; moveIndex--)
		{
			m_buffer[moveIndex + 1] = m_buffer[moveIndex];
		}

		m_buffer[index] = value;
		m_size++;
	}

	void remove(size_t index)
	{
		ASSERT(index < m_size);
		for (int i = index; i < m_size - 1; i++)
		{
			m_buffer[i] = m_buffer[i + 1];
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
		return m_buffer[index];
	}

	const T& operator [] (size_t index) const
	{
		ASSERT(index < m_size);
		return m_buffer[index];
	}

	size_t size() const
	{
		return m_size;
	}

	bool contains(const T& value)
	{
		for (const T& containedValue : *this)
		{
			if (value == containedValue)
			{
				return true;
			}
		}

		return false;
	}

private:
	T* m_buffer;
	uint m_size = 0;
	uint m_maxSize;
};

#define STACKARRAY(type, name, size) \
	uint name##BufferSize = std::max(1, size); \
	type* name##Buffer = (type*) _malloca((name##BufferSize) * sizeof(type)); \
	StackArray<type> name = StackArray<type>(name##Buffer, name##BufferSize);
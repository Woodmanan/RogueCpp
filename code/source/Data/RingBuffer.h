#pragma once
#include <mutex>
#include "Debug/Debug.h"
#include "Debug/Profiling.h"
#include <array>

template<typename T, size_t N>
class RingBuffer
{
public:
	bool Push(const T& value)
	{
		m_lock.lock();
		ASSERT(m_count >= 0 && m_count <= N);
		ASSERT(m_head >= 0 && m_head < N);
		bool success = false;
		if (m_count != N)
		{
			m_data[m_head] = value;
			m_count++;
			m_head = (m_head + 1) % N;
			success = true;
		}
		m_lock.unlock();
		return success;
	}

	bool Pop(T& value)
	{
		m_lock.lock();
		ASSERT(m_count >= 0 && m_count <= N);
		ASSERT(m_head >= 0 && m_head < N);
		bool success = false;
		if (m_count != 0)
		{
			size_t index = (m_head + N - m_count) % N;
			value = m_data[index];
			m_count--;
			ASSERT(m_count >= 0);
			success = true;
		}
		m_lock.unlock();
		return success;
	}

	void Clear()
	{
		m_lock.lock();
		m_count = 0;
		m_lock.unlock();
	}

private:
	ROGUE_LOCK(std::mutex, m_lock);
	std::array<T, N> m_data;
	size_t m_head = 0;
	size_t m_count = 0;
};
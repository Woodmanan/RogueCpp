#include "JobSystem.h"
#include "RingBuffer.h"
#include "Debug/Profiling.h"
#include <algorithm>
#include <atomic>
#include <thread>
#include <array>

namespace Jobs
{
	struct JobWorkerData
	{
		std::atomic<bool> alive;
		std::thread thread;
	};

	uint numThreads = 0;
	std::array<JobWorkerData, MaxWorkers> workerData;
	std::atomic<uint64> m_currentCount;
	std::atomic<uint64> m_scheduledCount;
	std::condition_variable wakeCondition;
	std::mutex wakeMutex;
	RingBuffer<std::function<void()>, MaxJobs> jobBuffer;

	void WorkerThread(int index)
	{
		std::string name = string_format("Job thread %d", index);
		ROGUE_NAME_THREAD(name.c_str());

		std::function<void()> job;

		while (workerData[index].alive)
		{
			if (jobBuffer.Pop(job))
			{
				job();
				m_currentCount.fetch_add(1);
			}
			else
			{
				std::unique_lock<std::mutex> lock(wakeMutex);
				wakeCondition.wait(lock);
			}
		}
	}

	void Initialize(uint numWorkers)
	{
		ROGUE_PROFILE_SECTION("Jobs::Initialize");
		numThreads = std::min(numWorkers, MaxWorkers);
		DEBUG_PRINT("Creating %d job worker threads.", numThreads);

		for (int i = 0; i < numThreads; i++)
		{
			workerData[i].alive = true;
			workerData[i].thread = std::thread(WorkerThread, i);
		}
	}

	void Shutdown()
	{
		ROGUE_PROFILE_SECTION("Jobs::Shutdown");
		for (int i = 0; i < numThreads; i++)
		{
			workerData[i].alive = false;
		}

		wakeCondition.notify_all();

		for (int i = 0; i < numThreads; i++)
		{
			workerData[i].thread.join();
		}
	}

	void Poll()
	{
		wakeCondition.notify_one(); // wake one worker thread
		std::this_thread::yield(); // allow this thread to be rescheduled
	}

	void QueueJob(const std::function<void()>& job)
	{
		ROGUE_PROFILE_SECTION("Jobs::QueueJob");
		m_scheduledCount.fetch_add(1);

		while (!jobBuffer.Push(job))
		{
			Poll();
		}

		wakeCondition.notify_one();
	}

	bool IsBusy()
	{
		return m_currentCount != m_scheduledCount;
	}

	void Wait()
	{
		ROGUE_PROFILE_SECTION("Jobs::Wait");
		while (m_currentCount != m_scheduledCount)
		{
			Poll();
		}
	}
}
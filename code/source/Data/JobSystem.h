#pragma once
#include <functional>
#include <thread>

using uint = uint32_t;
using uint64 = uint64_t;

//Interface and design from wicked engine blog: "https://wickedengine.net/2018/11/simple-job-system-using-standard-c/"
namespace Jobs
{
	static constexpr uint MaxWorkers = 16;
	static constexpr uint MaxJobs = 256;

	void Initialize(uint numWorkers);
	void Shutdown();

	void QueueJob(const std::function<void()>& job);
	bool IsBusy();
	void Wait();
}
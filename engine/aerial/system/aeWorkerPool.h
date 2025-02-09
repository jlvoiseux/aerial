#pragma once
#include "aerial/pch.h"

class aeWorkerPool
{
public:
	void init(size_t threads);
	void shutdown();

	std::future<void> enqueue(const std::function<void()> &f);
	size_t size();
	void waitForFinished();

private:
	std::vector<std::thread> m_workers;
	std::queue<std::function<void()>> m_tasks;
	std::atomic<size_t> m_tasksInProgress = 0;

	std::mutex m_queueMutex;
	std::condition_variable m_condition;
	bool m_stop = false;
};
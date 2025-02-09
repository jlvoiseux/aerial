#include "aeWorkerPool.h"

void aeWorkerPool::init(size_t threads)
{
	for (size_t i = 0; i < threads; ++i)
	{
		m_workers.emplace_back([this]
							   {
								   for (;;)
								   {
									   std::function<void()> task;
									   {
										   std::unique_lock<std::mutex> lock(this->m_queueMutex);
										   this->m_condition.wait(lock, [this]{ return this->m_stop || !this->m_tasks.empty(); });
										   if (this->m_stop)
											   return;
										   task = std::move(this->m_tasks.front());
										   this->m_tasks.pop();
									   }
									   m_tasksInProgress++;
									   task();
									   m_tasksInProgress--;
								   }
							   }
		);
	}
}

void aeWorkerPool::shutdown()
{
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_stop = true;
	}
	m_condition.notify_all();
	for (std::thread &worker: m_workers)
		worker.join();
}

std::future<void> aeWorkerPool::enqueue(const std::function<void()> &f)
{
	auto task = std::make_shared<std::packaged_task<void()>>(f);
	std::future<void> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		if (!m_stop)
			m_tasks.emplace([task](){ (*task)(); });
	}
	m_condition.notify_one();
	return res;
}

size_t aeWorkerPool::size()
{
	return m_workers.size();
}

void aeWorkerPool::waitForFinished()
{
	while (m_tasksInProgress > 0)
	{
		std::this_thread::yield();
	}
}
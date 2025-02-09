#pragma once
#include "aerial/system/aeWorkerPool.h"

class aeFilewatcher
{
public:
	aeFilewatcher();
	~aeFilewatcher();

	void watch(aeWorkerPool* pWorkerPool, const std::string &path, std::function<void(const std::string &)> callback);
	void shutdown();

	class Impl
	{
	public:
		virtual ~Impl() = default;
		virtual void watch(aeWorkerPool* pWorkerPool, const std::string& path, std::function<void(const std::string&)> callback) = 0;
		virtual void shutdown() = 0;
	};

private:
	std::unique_ptr<Impl> pImpl;
};

#if defined(_WIN32)
class aeFilewatcherWindows : public aeFilewatcher::Impl
{
public:
	aeFilewatcherWindows();
	~aeFilewatcherWindows();

	void watch(aeWorkerPool* pWorkerPool, const std::string& path, std::function<void(const std::string&)> callback) override;
	void shutdown() override;

private:
	void watchDirectory(HANDLE handle, const std::function<void(const std::string&)>& callback);

	std::map<std::string, HANDLE> m_handleMap;
	std::atomic<bool> m_bRunning;
	std::mutex m_mutex;
	std::condition_variable m_cv;
};

#elif defined(__APPLE__)

#endif
#include "aeFilewatcher.h"

#include "aerial/debug/aeTracy.h"
#include <utility>

aeFilewatcher::aeFilewatcher()
{
#if defined(_WIN32)
	pImpl = std::make_unique<aeFilewatcherWindows>();
#endif
}

aeFilewatcher::~aeFilewatcher() = default;

void aeFilewatcher::watch(aeWorkerPool* pWorkerPool, const std::string& path, std::function<void(const std::string&)> callback)
{
	pImpl->watch(pWorkerPool, path, std::move(callback));
}

void aeFilewatcher::shutdown()
{
	pImpl->shutdown();
}

#if defined(_WIN32)

aeFilewatcherWindows::aeFilewatcherWindows() : m_bRunning(false) {}

aeFilewatcherWindows::~aeFilewatcherWindows()
{
	shutdown();
}

void aeFilewatcherWindows::watch(aeWorkerPool* pWorkerPool, const std::string& path, std::function<void(const std::string&)> callback)
{
	auto handle = CreateFileA(path.c_str(),
							  FILE_LIST_DIRECTORY,
							  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
							  nullptr,
							  OPEN_EXISTING,
							  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
							  nullptr);

	if (handle == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to open directory handle");
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_bRunning = true;
	}
	pWorkerPool->enqueue([this, handle, callback]() { watchDirectory(handle, callback); });
}

void aeFilewatcherWindows::shutdown()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_bRunning = false;
	}
	m_cv.notify_all();
}

void aeFilewatcherWindows::watchDirectory(HANDLE handle, const std::function<void(const std::string&)>& callback)
{
	char buffer[1024];
	DWORD bytesReturned;
	FILE_NOTIFY_INFORMATION* fni;

	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			if (!m_bRunning)
				break;
		}

		if (ReadDirectoryChangesW(handle, buffer, sizeof(buffer), TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS |
			FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY,
			&bytesReturned, nullptr, nullptr))
		{
			fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
			do
			{
				std::wstring fileName(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
				callback(std::string(fileName.begin(), fileName.end()));
				fni = fni->NextEntryOffset ? reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<BYTE*>(fni) + fni->NextEntryOffset) : nullptr;
			} while (fni);
		}
	}
	CancelIo(handle);
	CloseHandle(handle);
}

#elif defined(__APPLE__)

#else

#endif

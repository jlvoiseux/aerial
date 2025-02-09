#include "aeLog.h"

#include "aerial/debug/aeDebugMenu.h"

#ifdef DEBUG_MENU_ENABLE
aeDebugMenu* aeLog::sm_pDebugMenu = nullptr;

void aeLog::setDebugMenu(aeDebugMenu* pDebugMenu)
{
	sm_pDebugMenu = pDebugMenu;
}
#endif

void aeLog::log(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
#ifdef DEBUG_MENU_ENABLE
	if (sm_pDebugMenu)
		sm_pDebugMenu->addLog(fmt, args);
#endif
	std::vprintf(fmt, args);
	std::printf("\n");
	va_end(args);
}
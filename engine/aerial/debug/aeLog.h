#pragma once

#include <cstdio>
#include "aerial/debug/aeDebugMenu.h"

#ifdef LOGGER_ENABLE
#define AE_LOG(...) aeLog::log(__VA_ARGS__)
#else
#define AE_LOG(...)
#endif

class aeLog
{
public:
#ifdef DEBUG_MENU_ENABLE
	static void setDebugMenu(aeDebugMenu* pDebugMenu);
#endif
	static void log(const char* fmt, ...);

private:
#ifdef DEBUG_MENU_ENABLE
	static aeDebugMenu* sm_pDebugMenu;
#endif
};

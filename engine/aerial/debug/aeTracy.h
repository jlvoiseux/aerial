#pragma once

#ifdef TRACY_ENABLE
#include "tracy/Tracy.hpp"
#define AE_TRACY_FRAME_MARK() FrameMark
#define AE_TRACY_ZONE_SCOPED(name) ZoneScopedN(name)
#else
#define AE_FRAME_MARK() ((void) 0)
#define AE_ZONE_SCOPED(name) ((void) 0)
#endif

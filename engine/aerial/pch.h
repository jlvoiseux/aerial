#pragma once

#define NOMINMAX
#define VK_NO_PROTOTYPES

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/trace/collector.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hd/renderPass.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/presentTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/taskController.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/imaging/hgiInterop/hgiInterop.h"
#include "pxr/imaging/hgiVulkan/buffer.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/instance.h"
#include "pxr/imaging/hgiVulkan/texture.h"
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/renderPass.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdUtils/stitch.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

#include "volk.h"

#include "aerial/debug/aeLog.h"
#include "aerial/debug/aeTracy.h"
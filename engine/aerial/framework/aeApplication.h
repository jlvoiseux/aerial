#pragma once

#include "aerial/pch.h"

#include "aerial/render/aeHydraRenderer.h"
#include "aerial/render/aeVulkan.h"
#include "aerial/scene/aeScene.h"
#include "aerial/scripting/aeLua.h"
#include "aerial/system/aeFilewatcher.h"

#ifdef DEBUG_MENU_ENABLE
#include "aerial/debug/aeDebugMenu.h"
#endif

class aeApplication
{
public:
	void init();
	void update(float t, float dt);
	void render();
	void shutdown();

	bool shouldClose();

private:
	std::unique_ptr<aeWorkerPool> m_pWorkerPool;
	std::unique_ptr<aeFilewatcher> m_pFilewatcher;
	std::unique_ptr<aeVulkan> m_pVulkan;
	std::unique_ptr<aeLua> m_pLua;
	std::unique_ptr<aeScene> m_pScene;
	std::unique_ptr<aeHydraRenderer> m_pHydraRenderer;
#ifdef DEBUG_MENU_ENABLE
	std::unique_ptr<aeDebugMenu> m_pDebugMenu;
#endif

	std::atomic_bool m_reloadScene = false;
};

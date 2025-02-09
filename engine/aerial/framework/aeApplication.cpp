#include "aeApplication.h"
#include "aerial/debug/aeTracy.h"

const std::string SHADERS_PATH = "res/shaders";
const std::string USD_PATH = "res/usd";
const uint32_t INIT_WIDTH = 1280;
const uint32_t INIT_HEIGHT = 720;

void aeApplication::init()
{
	m_pWorkerPool = std::make_unique<aeWorkerPool>();
	m_pFilewatcher = std::make_unique<aeFilewatcher>();
	m_pVulkan = std::make_unique<aeVulkan>();
	m_pLua = std::make_unique<aeLua>();
	m_pScene = std::make_unique<aeScene>();
	m_pHydraRenderer = std::make_unique<aeHydraRenderer>();
#ifdef DEBUG_MENU_ENABLE
	m_pDebugMenu = std::make_unique<aeDebugMenu>();
#endif

	m_pWorkerPool->init(std::thread::hardware_concurrency());
	m_pVulkan->init();
	m_pLua->init();
	m_pScene->init();
	m_pHydraRenderer->init(INIT_WIDTH, INIT_HEIGHT, m_pScene.get());

#ifdef DEBUG_MENU_ENABLE
	m_pDebugMenu->init(m_pVulkan.get(), m_pLua->getGameInput());
	aeLog::setDebugMenu(m_pDebugMenu.get());
#endif

	m_pFilewatcher->watch(m_pWorkerPool.get(), USD_PATH, [&](const std::string& path) {
							  AE_LOG("USD scene %s changed", path.c_str());
							  m_reloadScene = true;
						  });
}

void aeApplication::update(float t, float dt)
{
	AE_TRACY_ZONE_SCOPED("App update");
	m_pLua->update(t, dt);

	if (m_reloadScene)
	{
		m_pScene->reload();
		m_pHydraRenderer->reloadScene();
		m_reloadScene = false;
	}

	m_pHydraRenderer->setCameraTransform(m_pLua->getGameCamera()->getCameraTransform());
	m_pHydraRenderer->update(t, dt);

#ifdef DEBUG_MENU_ENABLE
	m_pDebugMenu->update(m_pVulkan.get(), m_pHydraRenderer.get(), m_pLua->getGameInput(), m_pScene.get(), t, dt);
#endif
}

void aeApplication::render()
{
	AE_TRACY_ZONE_SCOPED("App draw");
	m_pHydraRenderer->render();
#ifdef DEBUG_MENU_ENABLE
	m_pDebugMenu->render(m_pVulkan.get());
#endif
}

void aeApplication::shutdown()
{
#ifdef DEBUG_MENU_ENABLE
	m_pDebugMenu->shutdown(m_pVulkan.get());
#endif
	m_pHydraRenderer->shutdown();
	m_pScene->shutdown();
	m_pLua->shutdown();
	m_pVulkan->shutdown();
	m_pFilewatcher->shutdown();
	m_pWorkerPool->shutdown();
}

bool aeApplication::shouldClose()
{
#ifdef DEBUG_MENU_ENABLE
	return m_pDebugMenu->shouldClose();
#endif
}
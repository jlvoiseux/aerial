#pragma once

#include "aerial/pch.h"

#include "aerial/scene/aeScene.h"

class aeHydraRenderer
{
public:
	void init(uint32_t width, uint32_t height, aeScene *pScene);
	void update(float t, float dt);
	void render();
	void shutdown();

	void reloadScene();
	void setCameraTransform(const pxr::GfMatrix4d& transform);

	void setOutputTextureSize(uint32_t width, uint32_t height);
	pxr::HgiTextureHandle getOutputTextureHandle();
	const char* getOutputTextureAsByteArray();

private:
	aeScene* m_pScene;

	pxr::HgiUniquePtr m_pHgi;
	pxr::HdDriver m_hgiDriver;
	pxr::HdEngine m_engine;
	pxr::HdPluginRenderDelegateUniqueHandle m_pRenderDelegate;
	std::unique_ptr<pxr::HdRenderIndex> m_pRenderIndex;
	std::unique_ptr<pxr::HdxTaskController> m_pTaskController;
	pxr::HdRprimCollection m_collection;
	std::unique_ptr<pxr::UsdImagingDelegate> m_pSceneDelegate;
	pxr::SdfPath m_taskControllerId;
	pxr::HgiInterop m_interop;
	pxr::HdxRenderTaskParams m_params;

	pxr::GfCamera m_camera;
	pxr::GlfSimpleLight m_cameraLight;
	pxr::GlfSimpleMaterial m_material;
	pxr::GlfSimpleLightingContextRefPtr m_pLightingContext;

	std::vector<char> m_outputByteArray;
	size_t m_outputByteArraySize = 0;

	uint32_t m_width = 1280;
	uint32_t m_height = 720;
};
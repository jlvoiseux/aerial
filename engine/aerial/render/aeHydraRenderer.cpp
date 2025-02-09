#include "aeHydraRenderer.h"
#include <aerial/system/aeWorkerPool.h>

#include "aerial/debug/aeTracy.h"
#include "aerial/debug/aeDebugMenu.h"

const pxr::GfVec4f SCENE_AMBIENT(0.1f, 0.1f, 0.1f, 1.0f);
const pxr::GfVec4f SPECULAR_DEFAULT(0.1f, 0.1f, 0.1f, 1.0f);
const pxr::GfVec4f AMBIENT_DEFAULT(0.2f, 0.2f, 0.2f, 1.0f);
constexpr float SHININESS_DEFAULT(32.0f);

void aeHydraRenderer::init(uint32_t width, uint32_t height, aeScene *pScene)
{
	AE_TRACY_ZONE_SCOPED("aeHydraRenderer::init");

	m_pHgi = pxr::Hgi::CreatePlatformDefaultHgi();
	m_hgiDriver = { pxr::HgiTokens->renderDriver, pxr::VtValue(m_pHgi.get()) };
	pxr::HdRendererPluginRegistry &registry = pxr::HdRendererPluginRegistry::GetInstance();
	m_pRenderDelegate = registry.CreateRenderDelegate(pxr::TfToken("HdStormRendererPlugin"));

	m_pRenderIndex.reset(pxr::HdRenderIndex::New(m_pRenderDelegate.Get(), {&m_hgiDriver}));

	m_camera.SetFocalLength(12.f);
	m_camera.SetFocusDistance(5);
	m_camera.SetVerticalAperture(9.0f);
	m_camera.SetHorizontalAperture(16.0f);

	m_material.SetAmbient(AMBIENT_DEFAULT);
	m_material.SetSpecular(SPECULAR_DEFAULT);
	m_material.SetShininess(SHININESS_DEFAULT);

	m_pLightingContext = pxr::GlfSimpleLightingContext::New();

	m_pScene = pScene;
	m_width = width;
	m_height = height;

	m_pSceneDelegate = std::make_unique<pxr::UsdImagingDelegate>(m_pRenderIndex.get(), pxr::SdfPath("/"));
	m_pSceneDelegate->SetDisplayUnloadedPrimsWithBounds(false);
	m_pSceneDelegate->SetUsdDrawModesEnabled(true);
	m_pSceneDelegate->SetSceneMaterialsEnabled(true);
	m_pSceneDelegate->Populate(m_pScene->getStage()->GetPseudoRoot());

	m_collection.SetName(pxr::HdTokens->geometry);
	m_collection.SetRootPath(m_pScene->getStage()->GetPseudoRoot().GetPath());
	m_collection.SetMaterialTag(pxr::HdStMaterialTagTokens->defaultMaterialTag);
	m_collection.SetReprSelector(pxr::HdReprSelector(pxr::HdReprTokens->smoothHull));

	pxr::TfTokenVector aov{ pxr::HdAovTokens->color };
	m_pTaskController = std::make_unique<pxr::HdxTaskController>(m_pRenderIndex.get(), pxr::SdfPath("/defaultTaskController"));
	m_pTaskController->SetRenderTags({ pxr::HdRenderTagTokens->geometry, pxr::HdRenderTagTokens->render });
	m_pTaskController->SetEnablePresentation(false);
	m_pTaskController->SetCollection(m_collection);
	m_pTaskController->SetRenderOutputs(aov);
	m_pTaskController->SetEnableSelection(false);
}

void aeHydraRenderer::update(float t, float dt)
{
	AE_TRACY_ZONE_SCOPED("aeHydraRenderer::update");

	pxr::GfFrustum frustum = m_camera.GetFrustum();
	m_pTaskController->SetFreeCameraMatrices(frustum.ComputeViewMatrix(), frustum.ComputeProjectionMatrix());

	pxr::GfVec3d cameraPosition = m_camera.GetTransform().ExtractTranslation();
	m_cameraLight.SetAmbient(pxr::GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
	m_cameraLight.SetPosition(pxr::GfVec4f((float)cameraPosition[0], (float)cameraPosition[1], (float)cameraPosition[2], 1.f));

	m_pLightingContext->SetLights({m_cameraLight});
	m_pLightingContext->SetMaterial(m_material);
	m_pLightingContext->SetSceneAmbient(SCENE_AMBIENT);
	m_pLightingContext->SetUseLighting(true);

	m_pTaskController->SetLightingState(m_pLightingContext);
}

void aeHydraRenderer::render()
{
	AE_TRACY_ZONE_SCOPED("aeHydraRenderer::render");

	pxr::HdAovDescriptor aovDesc = m_pTaskController->GetRenderOutputSettings(pxr::HdAovTokens->color);
	aovDesc.clearValue = pxr::VtValue(pxr::GfVec4f(0.f, 0.f, 0.f, 1.f));
	m_pTaskController->SetRenderOutputSettings(pxr::HdAovTokens->color, aovDesc);

	setOutputTextureSize(m_width, m_height);

	pxr::HdTaskSharedPtrVector tasks = m_pTaskController->GetRenderingTasks();

	{
		AE_TRACY_ZONE_SCOPED("StartFrame");
		m_pHgi->StartFrame();
	}

	{
		AE_TRACY_ZONE_SCOPED("Execute");
		m_engine.Execute(m_pRenderIndex.get(), &tasks);
	}

	{
		AE_TRACY_ZONE_SCOPED("EndFrame");
		m_pHgi->EndFrame();
	}
}

void aeHydraRenderer::shutdown()
{
}

void aeHydraRenderer::reloadScene()
{
	m_pSceneDelegate->ApplyPendingUpdates();
}

void aeHydraRenderer::setCameraTransform(const pxr::GfMatrix4d& transform)
{
	m_camera.SetTransform(transform);
}

void aeHydraRenderer::setOutputTextureSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;

	m_pTaskController->SetRenderViewport(pxr::GfVec4f(0, 0, (float)width, (float)height));
	m_pTaskController->SetRenderBufferSize(pxr::GfVec2i(width, height));

	pxr::GfRange2f displayWindow(pxr::GfVec2f(0, 0), pxr::GfVec2f((float)width, (float)height));
	pxr::GfRect2i renderBufferRect(pxr::GfVec2i(0, 0), width, height);
	pxr::CameraUtilFraming framing(displayWindow, renderBufferRect);

	m_pTaskController->SetFraming(framing);

	m_params.viewport = pxr::GfVec4f(0, 0, (float)width, (float)height);
	m_params.enableLighting = true;
	m_params.wireframeColor = pxr::GfVec4f(1.f, 0.f, 0.f, 1.f);
	m_pTaskController->SetRenderParams(m_params);
}

pxr::HgiTextureHandle aeHydraRenderer::getOutputTextureHandle()
{
	pxr::VtValue aov;
	if (m_engine.GetTaskContextData(pxr::HdAovTokens->color, &aov))
	{
		if (aov.IsHolding<pxr::HgiTextureHandle>())
		{
			pxr::HgiTextureHandle pAovTex = aov.Get<pxr::HgiTextureHandle>();
			return pAovTex;
		}
	}

	return {};
}

const char* aeHydraRenderer::getOutputTextureAsByteArray()
{
	AE_TRACY_ZONE_SCOPED("aeHydraRenderer::getOutputTextureAsByteArray");

	pxr::VtValue aov;
	if (m_engine.GetTaskContextData(pxr::HdAovTokens->color, &aov))
	{
		if (aov.IsHolding<pxr::HgiTextureHandle>())
		{
			pxr::HgiTextureHandle pAovTex = aov.Get<pxr::HgiTextureHandle>();

			uint32_t width = pAovTex->GetDescriptor().dimensions[0];
			uint32_t height = pAovTex->GetDescriptor().dimensions[1];

			const size_t bufferByteSize = width * height * HgiGetDataSizeOfFormat(pAovTex->GetDescriptor().format);
			if (bufferByteSize != m_outputByteArraySize)
			{
				m_outputByteArraySize = bufferByteSize;
				m_outputByteArray.reserve(bufferByteSize);
			}

			pxr::HgiTextureGpuToCpuOp copyOp;
			copyOp.gpuSourceTexture = pAovTex;
			copyOp.sourceTexelOffset = pxr::GfVec3i(0);
			copyOp.mipLevel = 0;
			copyOp.cpuDestinationBuffer = m_outputByteArray.data();
			copyOp.destinationByteOffset = 0;
			copyOp.destinationBufferByteSize = bufferByteSize;

			pxr::HgiBlitCmdsUniquePtr blitCmds = m_pHgi->CreateBlitCmds();
			blitCmds->CopyTextureGpuToCpu(copyOp);
			m_pHgi->SubmitCmds(blitCmds.get(), pxr::HgiSubmitWaitTypeNoWait);

			return m_outputByteArray.data();
		}
	}

	return nullptr;
}
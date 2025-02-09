#include "aeScene.h"

void aeScene::init()
{
	//m_stage = pxr::UsdStage::CreateNew("res/usd/temp.usd");
	//m_root = pxr::UsdGeomMesh::Define(m_stage, pxr::SdfPath("/Root"));
	// appendUSDFile("res/usd/sponza.usda");

	m_stage = pxr::UsdStage::Open("res/usd/Kitchen_set.usd", pxr::UsdStage::LoadAll);
}

void aeScene::appendUSDFile(const std::string &path)
{
	pxr::UsdStageRefPtr srcStage = pxr::UsdStage::Open(path);
	pxr::UsdUtilsStitchLayers(m_stage->GetRootLayer(), srcStage->GetRootLayer());
	m_stage->GetRootLayer()->Save();
}

void aeScene::reload()
{
	m_stage->Reload();
}

void aeScene::shutdown()
{

}
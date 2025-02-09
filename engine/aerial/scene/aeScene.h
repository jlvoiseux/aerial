#pragma once
#include "aerial/pch.h"

class aeScene
{
public:
	void init();
	void shutdown();

	void appendUSDFile(const std::string& path);
	pxr::UsdStageRefPtr getStage() { return m_stage; }
	void reload();

private:
	pxr::UsdStageRefPtr m_stage;
	pxr::UsdGeomMesh m_root;
};
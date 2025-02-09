#pragma once

#include "aerial/pch.h"
#include "sol/sol.hpp"

class aeGameCamera
{
public:
	void init(sol::state* lua);

	const pxr::GfMatrix4d& getCameraTransform() { return m_cameraTransform; }

private:
	void registerLuaFunctions(sol::state* lua);

	pxr::GfMatrix4d m_cameraTransform{};
};
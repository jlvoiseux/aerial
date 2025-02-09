#pragma once

#include "aerial/pch.h"
#include "aerial/scripting/aeGameCamera.h"
#include "aerial/scripting/aeGameInput.h"
#include "sol/sol.hpp"

class aeLua
{
public:
	void init();
	void loadScript(const std::string& path);
	void update(float t, float dt);
	void shutdown();

	sol::state* getLua() { return &m_lua; }

	aeGameCamera* getGameCamera() { return &m_luaGameCamera; }
	aeGameInput* getGameInput() { return &m_luaGameInput; }

private:
	sol::state m_lua;
	std::vector<std::string> m_scriptPaths;

	aeGameCamera m_luaGameCamera;
	aeGameInput m_luaGameInput;
};
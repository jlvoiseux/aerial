#include "aeLua.h"

void aeLua::init()
{
	m_lua.open_libraries(sol::lib::base, sol::lib::math);
	m_luaGameInput.init(getLua());
	m_luaGameCamera.init(getLua());
	loadScript("res/scripts/fpsController.lua");
}

void aeLua::loadScript(const std::string& path)
{
	m_lua.script_file(path);
	m_scriptPaths.push_back(path);
}

void aeLua::update(float t, float dt)
{
//	for (const auto& scriptPath : m_scriptPaths)
//	{
		m_lua["update"](dt);
//	}
}

void aeLua::shutdown()
{

}
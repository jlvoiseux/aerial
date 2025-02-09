#include "aerial/pch.h"

#include "aeGameInput.h"

void aeGameInput::init(sol::state* lua)
{
	m_cursorPos = {0.0, 0.0};
	m_cursorPosDelta = {0.0, 0.0};

	m_keyMap["W"] = 'W';
	m_keyMap["A"] = 'A';
	m_keyMap["S"] = 'S';
	m_keyMap["D"] = 'D';
	m_keyMap["SPACE"] = VK_SPACE;
	m_keyMap["LEFT_CTRL"] = VK_LCONTROL;
	m_keyMap["LEFT_SHIFT"] = VK_LSHIFT;
	m_keyMap["TAB"] = VK_TAB;

	registerLuaFunctions(lua);
}

void aeGameInput::registerLuaFunctions(sol::state* lua)
{
	lua->set_function("isKeyPressed", [&](const std::string& key) {
		return isKeyPressed(key);
	});

	lua->set_function("getMouseDelta", [&]() {
		auto delta = getMouseDelta();
		return std::make_tuple(delta.x, delta.y);
	});
}

bool aeGameInput::isKeyPressed(const std::string& key) const
{
	if (!m_bIsFocused)
		return false;

	return GetAsyncKeyState(m_keyMap.at(key)) & 0x8000;
}

void aeGameInput::setMousePos(const aeGameInput::aeInputVec2 &pos)
{
	m_cursorPosDelta.x = pos.x - m_cursorPos.x;
	m_cursorPosDelta.y = pos.y - m_cursorPos.y;
	m_cursorPos = pos;
}

aeGameInput::aeInputVec2 aeGameInput::getMouseDelta() const
{
	if (!m_bIsFocused)
		return {0, 0};

	return m_cursorPosDelta;
}

void aeGameInput::setFocused(bool focused)
{
	m_bIsFocused = focused;
}
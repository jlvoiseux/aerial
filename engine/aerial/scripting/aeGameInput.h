#pragma once

#include "sol/sol.hpp"

class aeGameInput
{
public:
	void init(sol::state* lua);

	struct aeInputVec2
	{
		double x;
		double y;
	};

	bool isKeyPressed(const std::string& key) const;
	aeInputVec2 getMouseDelta() const;

	void setMousePos(const aeInputVec2& pos);
	void setFocused(bool focused);

private:
	void registerLuaFunctions(sol::state* lua);

	aeInputVec2 m_cursorPos;
	aeInputVec2 m_cursorPosDelta;
	bool m_bIsFocused = true;
	std::unordered_map<std::string, int> m_keyMap;
};
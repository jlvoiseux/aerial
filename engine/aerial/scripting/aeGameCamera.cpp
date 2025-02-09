#include "aeGameCamera.h"

void aeGameCamera::init(sol::state* lua)
{
	m_cameraTransform.SetIdentity();
	m_cameraTransform.SetTranslateOnly(pxr::GfVec3f(0.0f, 0.0f, 0.0f));
	m_cameraTransform.SetRotateOnly(pxr::GfRotation(pxr::GfVec3f(1.0f, 0.0f, 0.0f), 90.0f));

	registerLuaFunctions(lua);
}

void aeGameCamera::registerLuaFunctions(sol::state* lua)
{
	lua->set_function("moveForward", [&](float amount) {
		auto localForward = m_cameraTransform.TransformDir(pxr::GfVec3f(0.0f, 0.0f, -1.0f));
		m_cameraTransform.SetTranslateOnly(m_cameraTransform.ExtractTranslation() +  amount * localForward);
	});

	lua->set_function("moveRight", [&](float amount) {
		auto localRight = m_cameraTransform.TransformDir(pxr::GfVec3f(1.0f, 0.0f, 0.0f));
		m_cameraTransform.SetTranslateOnly(m_cameraTransform.ExtractTranslation() + amount * localRight);
	});

	lua->set_function("moveUp", [&](float amount) {
		auto localUp = m_cameraTransform.TransformDir(pxr::GfVec3f(0.0f, 1.0f, 0.0f));
		m_cameraTransform.SetTranslateOnly(m_cameraTransform.ExtractTranslation() + amount * localUp);
	});

	lua->set_function("rotate", [&](float yaw, float pitch) {
		auto rot = m_cameraTransform.ExtractRotation();
		auto rotYaw = pxr::GfRotation(pxr::GfVec3f(0.0f, 0.0f, 1.0f), -yaw); // Rotate around Z, the global up axis

		// The local right vector is obtained by doing the cross product of the local forward vector and the global up vector
		auto rotPitch = pxr::GfRotation(pxr::GfCross(m_cameraTransform.TransformDir(pxr::GfVec3f(0.0f, 0.0f, -1.0f)), pxr::GfVec3f(0.0f, 0.0f, 1.0f)).GetNormalized(), -pitch);
		m_cameraTransform.SetRotateOnly(rot * rotYaw * rotPitch);
	});
}
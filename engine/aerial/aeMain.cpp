#include "aerial/framework/aeApplication.h"
#include "aerial/debug/aeTracy.h"

int main()
{
	AE_LOG("USD Version: %d", PXR_VERSION);

	auto startTime = std::chrono::high_resolution_clock::now();

	float t = 0.0f;
	float dt = 0.0f;
	float lastFrameTime = 0.0f;

	aeApplication app;
	app.init();

	while (!app.shouldClose())
	{
		auto currTime = std::chrono::high_resolution_clock::now();
		float currFrameTime = std::chrono::duration<float>(currTime - startTime).count();
		dt = currFrameTime - lastFrameTime;
		t += dt;
		lastFrameTime = currFrameTime;

		app.update(t, dt);
		app.render();

		AE_TRACY_FRAME_MARK();
	}

	app.shutdown();
	return 0;
}


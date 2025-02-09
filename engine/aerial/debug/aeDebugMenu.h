#pragma once

#include "aerial/pch.h"
#include "aerial/scripting/aeGameInput.h"

#define GLFW_INCLUDE_VULKAN
#include "glfw/glfw3.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_IMPL_VULKAN_USE_VOLK
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#ifdef DEBUG_MENU_ENABLE

class aeVulkan;
class aeHydraRenderer;
class aeScene;

class aeDebugMenu
{
public:
	bool init(aeVulkan* pVk, aeGameInput* pGameInput);
	void update(aeVulkan* pVk, aeHydraRenderer* pHydraRenderer, aeGameInput* pGameInput, aeScene* pScene, float t, float dt);
	void render(aeVulkan* pVk);
	void shutdown(aeVulkan* pVk);

	void addLog(const char* fmt, va_list args) IM_FMTARGS(2);
	bool shouldClose() { return glfwWindowShouldClose(m_window); }

private:
	struct aeOutputTextureResources
	{
		VkDescriptorSet descriptorSet;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t pixelSizeInBytes = 0;

		VkImageView imageView = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory imageMemory = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
		VkBuffer uploadBuffer = VK_NULL_HANDLE;
		VkDeviceMemory uploadBufferMemory = VK_NULL_HANDLE;

		aeOutputTextureResources() { memset(this, 0, sizeof(*this)); }
	};

	void setupVulkanWindow(aeVulkan* pVk, VkSurfaceKHR surface, uint32_t width, uint32_t height);
	void frameRender(aeVulkan* pVk, ImDrawData* draw_data);
	void framePresent(aeVulkan* pVk);

	void setOutputTextureResources(aeVulkan* pVk, aeHydraRenderer* pHydraRenderer);

	ImGuiTextBuffer m_editorLogBuffer;
	GLFWwindow* m_window = nullptr;
	bool m_cursorCaptured = false;
	bool m_bFirstUpdate = true;
	ImVec2 m_lastViewportSize = ImVec2(16, 0);
	aeGameInput::aeInputVec2 m_cursorPos = {0, 0};

	ImGui_ImplVulkanH_Window m_mainWindowData;
	aeOutputTextureResources m_outputTextureResources;
};

#endif
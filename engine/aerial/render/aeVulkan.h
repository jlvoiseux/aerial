#pragma once

#include "aerial/pch.h"

class aeVulkan
{
public:
	void init();
	void shutdown();

	VkAllocationCallbacks* getAllocator() const { return m_allocator; }
	VkInstance getVkInstance() const { return m_vkInstance; }
	VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
	VkDevice getDevice() const { return m_device; }
	uint32_t getQueueFamily() const { return m_queueFamily; }
	VkQueue getQueue() const { return m_queue; }
	VkPipelineCache getPipelineCache() const { return m_pipelineCache; }
	VkDescriptorPool getDescriptorPool() const { return m_descriptorPool; }
	uint32_t getMinImageCount() const { return m_minImageCount; }

	bool getSwapChainRebuild() const { return m_swapChainRebuild; }
	void setSwapChainRebuild(bool swapChainRebuild) { m_swapChainRebuild = swapChainRebuild; }

	static void checkVkResult(VkResult err);

private:
	VkAllocationCallbacks*   m_allocator = nullptr;
	VkInstance               m_vkInstance = VK_NULL_HANDLE;
	VkPhysicalDevice         m_physicalDevice = VK_NULL_HANDLE;
	VkDevice                 m_device = VK_NULL_HANDLE;
	uint32_t                 m_queueFamily = (uint32_t)-1;
	VkQueue                  m_queue = VK_NULL_HANDLE;
	VkPipelineCache          m_pipelineCache = VK_NULL_HANDLE;
	VkDescriptorPool         m_descriptorPool = VK_NULL_HANDLE;
	uint32_t                 m_minImageCount = 2;
	bool                     m_swapChainRebuild = false;
};
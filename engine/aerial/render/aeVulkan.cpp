#include "aeVulkan.h"
#include <imgui/imgui.h>

namespace
{
	bool isExtensionAvailable(const std::vector<VkExtensionProperties>& properties, const char* extension)
	{
		for (const VkExtensionProperties& p : properties)
			if (strcmp(p.extensionName, extension) == 0)
				return true;
		return false;
	}

	VkPhysicalDevice selectVkPhysicalDevice(VkInstance instance)
	{
		uint32_t gpu_count;
		VkResult err = vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);
		aeVulkan::checkVkResult(err);
		assert(gpu_count > 0);

		std::vector<VkPhysicalDevice> gpus;
		gpus.resize(gpu_count);
		err = vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());
		aeVulkan::checkVkResult(err);

		for (VkPhysicalDevice& device : gpus)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				return device;
		}

		if (gpu_count > 0)
			return gpus[0];
		return VK_NULL_HANDLE;
	}
}

void aeVulkan::checkVkResult(VkResult err)
{
	if (err == 0)
		return;
	AE_LOG("[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

void aeVulkan::init()
{
	VkResult err;
	volkInitialize();

	std::vector<const char*> extensions;
	// GLFW required extensions
	extensions.push_back("VK_KHR_surface");
	extensions.push_back("VK_KHR_win32_surface");

	// Create Vulkan Instance
	{
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

		uint32_t properties_count;
		std::vector<VkExtensionProperties> properties;
		vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
		properties.resize(properties_count);
		err = vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data());
		checkVkResult(err);

		if (isExtensionAvailable(properties, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
			extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
		if (isExtensionAvailable(properties, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
		{
			extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
			createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		}
#endif

		createInfo.enabledExtensionCount = (uint32_t)extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();
		err = vkCreateInstance(&createInfo, m_allocator, &m_vkInstance);
		checkVkResult(err);
		volkLoadInstance(m_vkInstance);
	}

	m_physicalDevice = selectVkPhysicalDevice(m_vkInstance);

	{
		uint32_t count;
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, nullptr);
		VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &count, queues);
		for (uint32_t i = 0; i < count; i++)
		{
			if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				m_queueFamily = i;
				break;
			}
		}
		free(queues);
		assert(m_queueFamily != (uint32_t)-1);
	}

	{
		std::vector<const char*> device_extensions;
		device_extensions.push_back("VK_KHR_swapchain");

		uint32_t properties_count;
		std::vector<VkExtensionProperties> properties;
		vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &properties_count, nullptr);
		properties.resize(properties_count);
		vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &properties_count, properties.data());

		const float queue_priority[] = { 1.0f };
		VkDeviceQueueCreateInfo queueInfo[1] = {};
		queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo[0].queueFamilyIndex = m_queueFamily;
		queueInfo[0].queueCount = 1;
		queueInfo[0].pQueuePriorities = queue_priority;
		VkDeviceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = sizeof(queueInfo) / sizeof(queueInfo[0]);
		createInfo.pQueueCreateInfos = queueInfo;
		createInfo.enabledExtensionCount = (uint32_t)device_extensions.size();
		createInfo.ppEnabledExtensionNames = device_extensions.data();
		err = vkCreateDevice(m_physicalDevice, &createInfo, m_allocator, &m_device);
		checkVkResult(err);
		vkGetDeviceQueue(m_device, m_queueFamily, 0, &m_queue);
	}

	{
		VkDescriptorPoolSize pool_sizes[] =
				{
						{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
						{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
						{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
						{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
						{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
						{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
						{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
						{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
						{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
						{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
						{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
				};

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolInfo.maxSets = 1000 * (uint32_t)std::size(pool_sizes);
		poolInfo.poolSizeCount = (uint32_t)std::size(pool_sizes);
		poolInfo.pPoolSizes = pool_sizes;
		err = vkCreateDescriptorPool(m_device, &poolInfo, m_allocator, &m_descriptorPool);
		checkVkResult(err);
	}
}

void aeVulkan::shutdown()
{
	vkDestroyDescriptorPool(m_device, m_descriptorPool, m_allocator);
	vkDestroyDevice(m_device, m_allocator);
	vkDestroyInstance(m_vkInstance, m_allocator);
}

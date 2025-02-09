#include "aeDebugMenu.h"
#include "aerial/render/aeHydraRenderer.h"
#include "aerial/render/aeVulkan.h"
#include "aerial/scripting/aeLua.h"

#ifdef DEBUG_MENU_ENABLE

namespace
{
	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void drawSceneNode(const pxr::UsdPrim& node)
	{
		auto children = node.GetChildren();
		auto nodeName = node.GetName().GetString();
		ImGuiTreeNodeFlags nodeFlags = children.empty() ? ImGuiTreeNodeFlags_Leaf : 0;
		nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

		bool nodeOpen = ImGui::TreeNodeEx(node.GetName().GetText(), nodeFlags);

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		{
			AE_LOG("Selected node: %s", nodeName.c_str());
		}

		if (nodeOpen)
		{
			for (auto child : node.GetChildren())
			{
				drawSceneNode(child);
			}
			ImGui::TreePop();
		}
	}

	void processInput(GLFWwindow *window, aeGameInput* pGameInput)
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		aeGameInput::aeInputVec2 temp{};
		glfwGetCursorPos(window, &temp.x, &temp.y);
		pGameInput->setMousePos(temp);
	}
}

void aeDebugMenu::setOutputTextureResources(aeVulkan* pVk, aeHydraRenderer* pHydraRenderer)
{
	AE_TRACY_ZONE_SCOPED("aeDebugMenu::setOutputTextureResources");

	VkResult err;

	pxr::HgiTextureHandle texHandle = pHydraRenderer->getOutputTextureHandle();
	if (!texHandle)
		return;

	m_outputTextureResources.width = texHandle->GetDescriptor().dimensions[0];
	m_outputTextureResources.height = texHandle->GetDescriptor().dimensions[1];
	m_outputTextureResources.pixelSizeInBytes = 8;

	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		vkCreateSampler(pVk->getDevice(), &samplerInfo, pVk->getAllocator(), &m_outputTextureResources.sampler);
	}

	{
		VkImageCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		info.imageType = VK_IMAGE_TYPE_2D;
		info.format = VK_FORMAT_R16G16B16A16_SFLOAT; // TODO fix
		info.extent.width = m_outputTextureResources.width;
		info.extent.height = m_outputTextureResources.height;
		info.extent.depth = 1;
		info.mipLevels = 1;
		info.arrayLayers = 1;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.tiling = VK_IMAGE_TILING_OPTIMAL;
		info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		err = vkCreateImage(pVk->getDevice(), &info, pVk->getAllocator(), &m_outputTextureResources.image);
		aeVulkan::checkVkResult(err);
		VkMemoryRequirements req;
		vkGetImageMemoryRequirements(pVk->getDevice(), m_outputTextureResources.image, &req);
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = req.size;
		allocInfo.memoryTypeIndex = findMemoryType(pVk->getPhysicalDevice(), req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		err = vkAllocateMemory(pVk->getDevice(), &allocInfo, pVk->getAllocator(), &m_outputTextureResources.imageMemory);
		aeVulkan::checkVkResult(err);
		err = vkBindImageMemory(pVk->getDevice(), m_outputTextureResources.image, m_outputTextureResources.imageMemory, 0);
		aeVulkan::checkVkResult(err);
	}

	{
		VkImageViewCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		info.image = m_outputTextureResources.image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;
		err = vkCreateImageView(pVk->getDevice(), &info, pVk->getAllocator(), &m_outputTextureResources.imageView);
		aeVulkan::checkVkResult(err);
	}

	m_outputTextureResources.descriptorSet = ImGui_ImplVulkan_AddTexture(m_outputTextureResources.sampler, m_outputTextureResources.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	size_t imageSize = m_outputTextureResources.width * m_outputTextureResources.height * m_outputTextureResources.pixelSizeInBytes;

	{
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		err = vkCreateBuffer(pVk->getDevice(), &bufferInfo, pVk->getAllocator(), &m_outputTextureResources.uploadBuffer);
		aeVulkan::checkVkResult(err);
		VkMemoryRequirements req;
		vkGetBufferMemoryRequirements(pVk->getDevice(), m_outputTextureResources.uploadBuffer, &req);
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = req.size;
		allocInfo.memoryTypeIndex = findMemoryType(pVk->getPhysicalDevice(), req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		err = vkAllocateMemory(pVk->getDevice(), &allocInfo, pVk->getAllocator(), &m_outputTextureResources.uploadBufferMemory);
		aeVulkan::checkVkResult(err);
		err = vkBindBufferMemory(pVk->getDevice(), m_outputTextureResources.uploadBuffer, m_outputTextureResources.uploadBufferMemory, 0);
		aeVulkan::checkVkResult(err);
	}
}

void aeDebugMenu::setupVulkanWindow(aeVulkan* pVk, VkSurfaceKHR surface, uint32_t width, uint32_t height)
{
	ImGui_ImplVulkanH_Window* wd = &m_mainWindowData;
	wd->Surface = surface;

	VkBool32 res;
	vkGetPhysicalDeviceSurfaceSupportKHR(pVk->getPhysicalDevice(), pVk->getQueueFamily(), wd->Surface, &res);
	if (res != VK_TRUE)
	{
		fprintf(stderr, "Error no WSI support on physical device 0\n");
		exit(-1);
	}

	const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(pVk->getPhysicalDevice(), wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
	wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(pVk->getPhysicalDevice(), wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));

	IM_ASSERT(pVk->getMinImageCount() >= 2);
	ImGui_ImplVulkanH_CreateOrResizeWindow(pVk->getVkInstance(), pVk->getPhysicalDevice(), pVk->getDevice(), wd, pVk->getQueueFamily(), pVk->getAllocator(), width, height, pVk->getMinImageCount());
}

void aeDebugMenu::frameRender(aeVulkan* pVk, ImDrawData* draw_data)
{
	AE_TRACY_ZONE_SCOPED("aeDebugMenu::frameRender");

	ImGui_ImplVulkanH_Window* wd = &m_mainWindowData;
	VkResult err;

	VkSemaphore image_acquired_semaphore  = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
	VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
	err = vkAcquireNextImageKHR(pVk->getDevice(), wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
	{
		pVk->setSwapChainRebuild(true);
		return;
	}
	aeVulkan::checkVkResult(err);

	ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
	{
		err = vkWaitForFences(pVk->getDevice(), 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
		aeVulkan::checkVkResult(err);

		err = vkResetFences(pVk->getDevice(), 1, &fd->Fence);
		aeVulkan::checkVkResult(err);
	}
	{
		err = vkResetCommandPool(pVk->getDevice(), fd->CommandPool, 0);
		aeVulkan::checkVkResult(err);
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
		aeVulkan::checkVkResult(err);
	}
	{
		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = wd->RenderPass;
		info.framebuffer = fd->Framebuffer;
		info.renderArea.extent.width = wd->Width;
		info.renderArea.extent.height = wd->Height;
		info.clearValueCount = 1;
		info.pClearValues = &wd->ClearValue;
		vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	}

	ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

	vkCmdEndRenderPass(fd->CommandBuffer);
	{
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &image_acquired_semaphore;
		info.pWaitDstStageMask = &wait_stage;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &fd->CommandBuffer;
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &render_complete_semaphore;

		err = vkEndCommandBuffer(fd->CommandBuffer);
		aeVulkan::checkVkResult(err);
		err = vkQueueSubmit(pVk->getQueue(), 1, &info, fd->Fence);
		aeVulkan::checkVkResult(err);
		err = vkDeviceWaitIdle(pVk->getDevice());
		aeVulkan::checkVkResult(err);
	}
}

void aeDebugMenu::framePresent(aeVulkan* pVk)
{
	if (pVk->getSwapChainRebuild())
		return;

	ImGui_ImplVulkanH_Window* wd = &m_mainWindowData;

	VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &render_complete_semaphore;
	info.swapchainCount = 1;
	info.pSwapchains = &wd->Swapchain;
	info.pImageIndices = &wd->FrameIndex;
	VkResult err = vkQueuePresentKHR(pVk->getQueue(), &info);
	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
	{
		pVk->setSwapChainRebuild(true);
		return;
	}
	aeVulkan::checkVkResult(err);
	wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->SemaphoreCount; // Now we can use the next set of semaphores
}

bool aeDebugMenu::init(aeVulkan* pVk, aeGameInput* pGameInput)
{
	glfwInit();
	uint32_t width = 1920;
	uint32_t height = 1080;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow *window = glfwCreateWindow((int)width, (int)height, "Aerial", nullptr, nullptr);
	if (window == nullptr)
	{
		AE_LOG("Failed to create GLFW window");
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	m_window = window;
	m_cursorCaptured = false;

	VkSurfaceKHR surface;
	VkResult err = glfwCreateWindowSurface(pVk->getVkInstance(), window, pVk->getAllocator(), &surface);
	aeVulkan::checkVkResult(err);

	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	ImGui_ImplVulkanH_Window* wd = &m_mainWindowData;
	setupVulkanWindow(pVk, surface, w, h);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo initInfo = {};
	initInfo.Instance = pVk->getVkInstance();
	initInfo.PhysicalDevice = pVk->getPhysicalDevice();
	initInfo.Device = pVk->getDevice();
	initInfo.QueueFamily = pVk->getQueueFamily();
	initInfo.Queue = pVk->getQueue();
	initInfo.PipelineCache = pVk->getPipelineCache();
	initInfo.DescriptorPool = pVk->getDescriptorPool();
	initInfo.RenderPass = wd->RenderPass;
	initInfo.Subpass = 0;
	initInfo.MinImageCount = pVk->getMinImageCount();
	initInfo.ImageCount = wd->ImageCount;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.Allocator = pVk->getAllocator();
	initInfo.CheckVkResultFn = aeVulkan::checkVkResult;
	ImGui_ImplVulkan_Init(&initInfo);

	glfwGetCursorPos(m_window, &m_cursorPos.x, &m_cursorPos.y);
	pGameInput->setMousePos(m_cursorPos);
	pGameInput->setFocused(false);

	return true;
}

void aeDebugMenu::update(aeVulkan* pVk, aeHydraRenderer* pHydraRenderer, aeGameInput* pGameInput, aeScene* pScene, float t, float dt)
{
	glfwPollEvents();
	processInput(m_window, pGameInput);

	int fbWidth, fbHeight;
	glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
	if (fbWidth > 0 && fbHeight > 0 && (pVk->getSwapChainRebuild() || m_mainWindowData.Width != fbWidth || m_mainWindowData.Height != fbHeight))
	{
		ImGui_ImplVulkan_SetMinImageCount(pVk->getMinImageCount());
		ImGui_ImplVulkanH_CreateOrResizeWindow(pVk->getVkInstance(), pVk->getPhysicalDevice(), pVk->getDevice(), &m_mainWindowData, pVk->getQueueFamily(), pVk->getAllocator(), fbWidth, fbHeight, pVk->getMinImageCount());
		m_mainWindowData.FrameIndex = 0;
		pVk->setSwapChainRebuild(false);
	}

	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplVulkan_NewFrame();
	ImGui::NewFrame();

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("DockSpace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoDocking);
	ImGui::PopStyleVar(3);

	ImGuiID dockSpaceId = ImGui::GetID("DockSpaceId");
	ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	if (m_bFirstUpdate)
	{
		ImGui::DockBuilderRemoveNode(dockSpaceId);
		ImGui::DockBuilderAddNode(dockSpaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockSpaceId, viewport->Size);

		ImGuiID dockLeftId = ImGui::DockBuilderSplitNode(dockSpaceId, ImGuiDir_Left, 0.2f, nullptr, &dockSpaceId);
		ImGuiID dockRightId = dockSpaceId;
		ImGuiID dockRightTopId = ImGui::DockBuilderSplitNode(dockRightId, ImGuiDir_Up, 0.7f, nullptr, &dockRightId);
		ImGuiID dockRightBottomId = dockRightId;

		ImGui::DockBuilderDockWindow("Scene Tree", dockLeftId);
		ImGui::DockBuilderDockWindow("Renderer output", dockRightTopId);
		ImGui::DockBuilderDockWindow("Log", dockRightBottomId);

		ImGui::DockBuilderFinish(dockSpaceId);
	}

	ImGui::End();

	ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Scene Tree"))
	{
		drawSceneNode(pScene->getStage()->GetPseudoRoot());
		ImGui::End();
	}

	ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Log"))
	{
		if (ImGui::Button("Clear")) m_editorLogBuffer.clear();
		ImGui::Separator();
		ImGui::BeginChild("ScrollView");
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0,1));
		ImGui::TextUnformatted(m_editorLogBuffer.begin());
		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::End();
	}

	ImGui::SetNextWindowSize(ImVec2(1280,720), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Renderer output"))
	{
		if (m_bFirstUpdate)
		{
			ImGui::SetWindowFocus("Scene Tree");
		}

		ImVec2 regAvail = ImGui::GetContentRegionAvail();
		ImVec2 viewportSize = ImVec2{std::max(1.f, regAvail.x), std::max(1.f, regAvail.y)};

		if (m_lastViewportSize.x != viewportSize.x || m_lastViewportSize.y != viewportSize.y || !m_outputTextureResources.descriptorSet)
		{
			pHydraRenderer->setOutputTextureSize((int)viewportSize.x, (int)viewportSize.y);
			m_lastViewportSize = viewportSize;

			if (m_outputTextureResources.descriptorSet)
			{
				ImGui_ImplVulkan_RemoveTexture(m_outputTextureResources.descriptorSet);
				m_outputTextureResources.descriptorSet = VK_NULL_HANDLE;
			}

			setOutputTextureResources(pVk, pHydraRenderer);
		}

		const char* outputTextureData = pHydraRenderer->getOutputTextureAsByteArray();
		if (outputTextureData != nullptr && m_outputTextureResources.descriptorSet)
		{
			AE_TRACY_ZONE_SCOPED("update debug menu viewport");

			VkResult err;

			{
				size_t imageSize = m_outputTextureResources.width * m_outputTextureResources.height * m_outputTextureResources.pixelSizeInBytes;
				void* map = nullptr;
				err = vkMapMemory(pVk->getDevice(), m_outputTextureResources.uploadBufferMemory, 0, imageSize, 0, &map);
				aeVulkan::checkVkResult(err);
				memcpy(map, outputTextureData, imageSize);
				VkMappedMemoryRange range[1] = {};
				range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				range[0].memory = m_outputTextureResources.uploadBufferMemory;
				range[0].size = imageSize;
				err = vkFlushMappedMemoryRanges(pVk->getDevice(), 1, range);
				aeVulkan::checkVkResult(err);
				vkUnmapMemory(pVk->getDevice(), m_outputTextureResources.uploadBufferMemory);
			}

			VkCommandPool commandPool = m_mainWindowData.Frames[m_mainWindowData.FrameIndex].CommandPool;
			VkCommandBuffer command_buffer;
			{
				VkCommandBufferAllocateInfo allocInfo{};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandPool = commandPool;
				allocInfo.commandBufferCount = 1;

				err = vkAllocateCommandBuffers(pVk->getDevice(), &allocInfo, &command_buffer);
				aeVulkan::checkVkResult(err);

				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				err = vkBeginCommandBuffer(command_buffer, &beginInfo);
				aeVulkan::checkVkResult(err);
			}

			{
				VkImageMemoryBarrier copy_barrier[1] = {};
				copy_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				copy_barrier[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				copy_barrier[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				copy_barrier[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				copy_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				copy_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				copy_barrier[0].image = m_outputTextureResources.image;
				copy_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copy_barrier[0].subresourceRange.levelCount = 1;
				copy_barrier[0].subresourceRange.layerCount = 1;
				vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, copy_barrier);

				VkBufferImageCopy region = {};
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.layerCount = 1;
				region.imageExtent.width = m_outputTextureResources.width;
				region.imageExtent.height = m_outputTextureResources.height;
				region.imageExtent.depth = 1;
				vkCmdCopyBufferToImage(command_buffer, m_outputTextureResources.uploadBuffer, m_outputTextureResources.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

				VkImageMemoryBarrier use_barrier[1] = {};
				use_barrier[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				use_barrier[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				use_barrier[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				use_barrier[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				use_barrier[0].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				use_barrier[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				use_barrier[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				use_barrier[0].image = m_outputTextureResources.image;
				use_barrier[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				use_barrier[0].subresourceRange.levelCount = 1;
				use_barrier[0].subresourceRange.layerCount = 1;
				vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, use_barrier);
			}

			{
				VkSubmitInfo endInfo = {};
				endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				endInfo.commandBufferCount = 1;
				endInfo.pCommandBuffers = &command_buffer;
				err = vkEndCommandBuffer(command_buffer);
				aeVulkan::checkVkResult(err);
				err = vkQueueSubmit(pVk->getQueue(), 1, &endInfo, VK_NULL_HANDLE);
				aeVulkan::checkVkResult(err);
				err = vkDeviceWaitIdle(pVk->getDevice());
				aeVulkan::checkVkResult(err);
			}

			ImGui::Image((ImTextureID)m_outputTextureResources.descriptorSet, ImVec2(viewportSize.x, viewportSize.y), ImVec2(0, 1), ImVec2(1, 0));
		}

		if (ImGui::IsWindowFocused())
		{
			if (!m_cursorCaptured)
			{
				pGameInput->setFocused(true);
				glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				m_cursorCaptured = true;
			}

			if (pGameInput->isKeyPressed("TAB"))
			{
				pGameInput->setFocused(false);
				glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				m_cursorCaptured = false;

				ImGui::SetWindowFocus("Scene Tree");
			}
		}
		else if (m_cursorCaptured)
		{
			pGameInput->setFocused(true);
			glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			m_cursorCaptured = false;
		}

		ImGui::End();
	}

	m_bFirstUpdate = false;

	ImGui::EndFrame();
}

void aeDebugMenu::render(aeVulkan* pVk)
{
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkanH_Window* wd = &m_mainWindowData;
	const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	if (!is_minimized)
	{
		wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
		wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
		wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
		wd->ClearValue.color.float32[3] = clear_color.w;
		frameRender(pVk, draw_data);
		framePresent(pVk);
	}

	glfwSwapBuffers(m_window);
}

void aeDebugMenu::shutdown(aeVulkan* pVk)
{
	VkResult err = vkDeviceWaitIdle(pVk->getDevice());
	aeVulkan::checkVkResult(err);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	ImGui_ImplVulkanH_DestroyWindow(pVk->getVkInstance(), pVk->getDevice(), &m_mainWindowData, pVk->getAllocator());

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void aeDebugMenu::addLog(const char* fmt, va_list args)
{
	m_editorLogBuffer.appendfv(fmt, args);
	m_editorLogBuffer.append("\n");
}

#endif
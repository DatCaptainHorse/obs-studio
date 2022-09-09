/******************************************************************************
    Copyright (C) 2022 by Kristian Ollikainen <kristiankulta2000@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "vk-subsystem.hpp"
#include "vk-helpers.hpp"

#include <graphics/vec3.h>

void gs_device::CreateLogicalDevice()
{
	const auto physicalDevice = GetPhysicalDevice();
	const auto queueFamilyProperties =
		physicalDevice.getQueueFamilyProperties();

	/* get queue family with graphics and transfer capabilities */
	queueFamilyIndex = static_cast<uint32_t>(std::distance(
		queueFamilyProperties.begin(),
		std::find_if(
			queueFamilyProperties.begin(),
			queueFamilyProperties.end(),
			[](const vk::QueueFamilyProperties &queueFamilyProperty) {
				return queueFamilyProperty.queueFlags &
					       vk::QueueFlagBits::eGraphics |
				       vk::QueueFlagBits::eTransfer;
			})));

	float priority = 0.0f;
	const auto deviceQueueCreateInfo =
		vk::DeviceQueueCreateInfo({}, queueFamilyIndex, 1, &priority);

	/* find required device layers */
	const auto deviceLayerProperties =
		physicalDevice.enumerateDeviceLayerProperties();

	std::vector<const char *> deviceLayers;
	std::vector<const char *> requiredDeviceLayers = {
#ifdef _DEBUG
		"VK_LAYER_KHRONOS_validation"
#endif
	};

	for (const auto &layerProperty : deviceLayerProperties) {
		for (const auto &requiredLayer : requiredDeviceLayers) {
			if (strcmp(layerProperty.layerName.data(),
				   requiredLayer) == 0)
				deviceLayers.emplace_back(requiredLayer);
		}
	}

	if (deviceLayers.size() != requiredDeviceLayers.size())
		throw std::runtime_error(
			"Could not find all required device layers");

	/* find required device extensions */
	const auto deviceExtensionProperties =
		physicalDevice.enumerateDeviceExtensionProperties();

	std::vector<const char *> deviceExtensions;
	std::vector<const char *> requiredDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME};

	for (const auto &extensionProperty : deviceExtensionProperties) {
		for (const auto &requiredExtension : requiredDeviceExtensions) {
			if (strcmp(extensionProperty.extensionName.data(),
				   requiredExtension) == 0)
				deviceExtensions.emplace_back(
					requiredExtension);
		}
	}

	if (deviceExtensions.size() != requiredDeviceExtensions.size())
		throw std::runtime_error(
			"Could not find all required device extensions");

	const auto deviceFeatures = physicalDevice.getFeatures();

	const auto deviceCreateInfo = vk::DeviceCreateInfo(
		{}, 1, &deviceQueueCreateInfo,
		static_cast<uint32_t>(deviceLayers.size()), deviceLayers.data(),
		static_cast<uint32_t>(deviceExtensions.size()),
		deviceExtensions.data(), &deviceFeatures);

	try {
		std::get<1>(device) =
			physicalDevice.createDevice(deviceCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}

	queue = GetLogicalDevice().getQueue(queueFamilyIndex, 0);
}

gs_swap_chain *gs_device::CreateSwapchain(const gs_init_data *data)
{
	int32_t scIdx = -1;
	if ((scIdx = GetLoadedSwapchainIdx(data)) != -1) {
		currentSwapchain = scIdx;
		return loadedSwapchains[currentSwapchain].get();
	}

	try {
		loadedSwapchains.emplace_back(std::make_unique<gs_swap_chain>(
			this, data,
			std::make_unique<vulkan_surface>(instance, data),
			queueFamilyIndex));
	} catch (const std::runtime_error &error) {
		blog(LOG_ERROR, "%s: Failed to create swapchain: %s",
		     deviceName.c_str(), error.what());
		return nullptr;
	}

	if (loadedSwapchains.size() == 1) {
		const auto &swapchain = loadedSwapchains.front();
		blog(LOG_INFO, "%s: CreateRenderPasses", deviceName.c_str());
		CreateRenderPasses(swapchain->format);
		blog(LOG_INFO, "%s: CreateFramebuffers", deviceName.c_str());
		CreateFramebuffers();
		blog(LOG_INFO, "%s: CreateCommandBuffers", deviceName.c_str());
		CreateCommandBuffers();
		blog(LOG_INFO, "%s: CreateSyncObjects", deviceName.c_str());
		CreateSyncObjects();
	}

	currentSwapchain = static_cast<int32_t>(loadedSwapchains.size() - 1);
	return loadedSwapchains[currentSwapchain].get();
}

void gs_device::CreateRenderPasses(vk::Format format)
{
	const auto logicalDevice = GetLogicalDevice();

	const auto depthFormat = FindSupportedFormat(
		{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
		 vk::Format::eD24UnormS8Uint},
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);

	const std::array attachmentDescriptions = {
		vk::AttachmentDescription({}, format,
					  vk::SampleCountFlagBits::e1,
					  vk::AttachmentLoadOp::eClear,
					  vk::AttachmentStoreOp::eStore,
					  vk::AttachmentLoadOp::eDontCare,
					  vk::AttachmentStoreOp::eDontCare,
					  vk::ImageLayout::eUndefined,
					  vk::ImageLayout::ePresentSrcKHR),
		vk::AttachmentDescription(
			{}, depthFormat, vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthStencilAttachmentOptimal)};

	const vk::AttachmentReference colorReference(
		0, vk::ImageLayout::eColorAttachmentOptimal);
	const vk::AttachmentReference depthReference(
		1, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	const vk::SubpassDescription subpassDescription(
		{}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1,
		&colorReference, nullptr, &depthReference);

	const vk::SubpassDependency dependency(
		VK_SUBPASS_EXTERNAL, 0,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::PipelineStageFlagBits::eColorAttachmentOutput,
		vk::AccessFlagBits::eMemoryRead,
		vk::AccessFlagBits::eColorAttachmentRead |
			vk::AccessFlagBits::eColorAttachmentWrite);

	const vk::RenderPassCreateInfo renderPassCreateInfo(
		{}, static_cast<uint32_t>(attachmentDescriptions.size()),
		attachmentDescriptions.data(), 1, &subpassDescription, 1,
		&dependency);

	try {
		renderPass =
			logicalDevice.createRenderPass(renderPassCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

vk::DescriptorSetLayout gs_device::CreateDescriptorSetLayout(
	const std::vector<vk::DescriptorSetLayoutBinding> &bindings)
{
	const vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
		{}, static_cast<uint32_t>(bindings.size()), bindings.data());

	try {
		return GetLogicalDevice().createDescriptorSetLayout(
			descriptorSetLayoutCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

std::vector<vk::DescriptorSet>
gs_device::CreateDescriptorSets(VulkanShader *shader)
{
	const auto logicalDevice = GetLogicalDevice();

	const std::array descriptorSetLayouts = {shader->descriptorSetLayout};
	const vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(
		descriptorPool,
		static_cast<uint32_t>(descriptorSetLayouts.size()),
		descriptorSetLayouts.data());

	const auto descriptorSets =
		logicalDevice.allocateDescriptorSets(descriptorSetAllocateInfo);

	std::vector<std::tuple<uint32_t, vk::DescriptorBufferInfo>> UBOs;
	if (shader->vertexShader->uniformBuffer) {
		UBOs.emplace_back(shader->vertexShader->bindings.begin()->first, vk::DescriptorBufferInfo(
			shader->vertexShader->uniformBuffer->buffer, 0, shader->vertexShader->uniformBuffer->deviceSize));
	}
	if (shader->fragmentShader->uniformBuffer) {
		UBOs.emplace_back(shader->fragmentShader->bindings.begin()->first, vk::DescriptorBufferInfo(
			shader->fragmentShader->uniformBuffer->buffer, 0, shader->fragmentShader->uniformBuffer->deviceSize));
	}

	std::vector<vk::DescriptorImageInfo> descImageInfos;
	if (!loadedTextures.empty() && currentTexture >= 0) {
		const auto &t = loadedTextures[currentTexture];
		for (const auto &tex : shader->fragmentShader->samplers) {
			descImageInfos.emplace_back(vk::DescriptorImageInfo(
				t->samplerState->sampler, t->imageView,
				vk::ImageLayout::eShaderReadOnlyOptimal));
		}
	}

	std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
	for (const auto &descriptorSet : descriptorSets) {
		for (const auto &[bind, ubo] : UBOs)
			writeDescriptorSets.emplace_back(vk::WriteDescriptorSet(
				descriptorSet, bind, 0, 1,
				vk::DescriptorType::eUniformBuffer, nullptr,
				&ubo));

		for (const auto &descImageInfo : descImageInfos)
			writeDescriptorSets.emplace_back(vk::WriteDescriptorSet(
				descriptorSet, 1, 0, 1,
				vk::DescriptorType::eCombinedImageSampler,
				&descImageInfo));
	}

	logicalDevice.updateDescriptorSets(writeDescriptorSets, nullptr);
	return descriptorSets;
}

vk::PipelineLayout
gs_device::CreatePipelineLayout(vk::DescriptorSetLayout descriptorSetLayout)
{
	const vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
		{}, 1, &descriptorSetLayout, 0, nullptr);

	try {
		return GetLogicalDevice().createPipelineLayout(
			pipelineLayoutCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

void gs_device::CreateCommandPool()
{
	const auto logicalDevice = GetLogicalDevice();

	const vk::CommandPoolCreateInfo commandPoolCreateInfo({},
							      queueFamilyIndex);

	try {
		commandPool =
			logicalDevice.createCommandPool(commandPoolCreateInfo);

		instantPool =
			logicalDevice.createCommandPool(commandPoolCreateInfo);

		instantBuffer = logicalDevice.allocateCommandBuffers(
			vk::CommandBufferAllocateInfo(
				instantPool, vk::CommandBufferLevel::ePrimary,
				1))[0];
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

/* TODO: See if enough or could be dynamic */
constexpr uint32_t maxPoolSize = 128;

void gs_device::CreateDescriptorPool()
{
	const auto logicalDevice = GetLogicalDevice();

	const std::array descriptorPoolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer,
				       maxPoolSize),
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler,
				       maxPoolSize)};

	const vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(
		{}, maxPoolSize,
		static_cast<uint32_t>(descriptorPoolSizes.size()),
		descriptorPoolSizes.data());

	try {
		descriptorPool = logicalDevice.createDescriptorPool(
			descriptorPoolCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

void gs_device::CreateFramebuffers()
{
	const auto logicalDevice = GetLogicalDevice();
	const auto &swapchain = loadedSwapchains.front();

	for (uint32_t i = 0; i < swapchain->imageCount; i++) {
		const std::array attachments = {
			swapchain->colorImages[i].imageView,
			swapchain->depthImages[i].imageView};

		const vk::FramebufferCreateInfo framebufferCreateInfo(
			{}, renderPass,
			static_cast<uint32_t>(attachments.size()),
			attachments.data(), swapchain->extent.width,
			swapchain->extent.height, 1);

		try {
			framebuffers.emplace_back(
				logicalDevice.createFramebuffer(
					framebufferCreateInfo));
		} catch (const vk::Error &error) {
			throw std::runtime_error(error.what());
		}
	}
}

void gs_device::CreateCommandBuffers()
{
	const auto logicalDevice = GetLogicalDevice();
	const auto &swapchain = loadedSwapchains.front();

	const vk::CommandBufferAllocateInfo commandBufferAllocateInfo(
		commandPool, vk::CommandBufferLevel::ePrimary,
		static_cast<uint32_t>(framebuffers.size()));

	try {
		commandBuffers = logicalDevice.allocateCommandBuffers(
			commandBufferAllocateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}

	const vk::CommandBufferBeginInfo commandBufferBeginInfo(
		vk::CommandBufferUsageFlagBits::eSimultaneousUse);

	for (uint32_t i = 0; i < commandBuffers.size(); i++) {
		try {
			commandBuffers[i].begin(commandBufferBeginInfo);
		} catch (const vk::Error &error) {
			throw std::runtime_error(error.what());
		}

		std::array<vk::ClearValue, 2> clearValues;
		clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{
				clearColor->x, clearColor->y, clearColor->z,
				clearColor->w});
		clearValues[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);

		const vk::RenderPassBeginInfo renderPassBeginInfo(
			renderPass, framebuffers[i],
			vk::Rect2D({0, 0}, swapchain->extent),
			static_cast<uint32_t>(clearValues.size()),
			clearValues.data());

		commandBuffers[i].setViewport(0, viewport);
		commandBuffers[i].setScissor(0, scissor);
		commandBuffers[i].beginRenderPass(renderPassBeginInfo,
						  vk::SubpassContents::eInline);

		for (const auto &renderable : loadedRenderables) {
		//if (currentRenderable >= 0 && !loadedRenderables[currentRenderable]->descriptorSets.empty()) {
			//const auto &renderable = loadedRenderables[currentRenderable];
			if (renderable->descriptorSets.empty() || !renderable->shader->pipeline)
				continue;

			commandBuffers[i].bindPipeline(
				vk::PipelineBindPoint::eGraphics,
				renderable->shader->pipeline);

			if (renderable->vertexBuffer) {
				commandBuffers[i].bindVertexBuffers(
					0, renderable->vertexBuffer->buffer,
					{0});
			}
			if (renderable->indexBuffer)
				commandBuffers[i].bindIndexBuffer(
					renderable->indexBuffer->buffer, 0,
					vk::IndexType::eUint32);

			commandBuffers[i].bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				renderable->shader->pipelineLayout, 0,
				renderable->descriptorSets, {});

			if (renderable->indexBuffer)
				commandBuffers[i].drawIndexed(static_cast<uint32_t>(renderable->indexBuffer->nIndices), 1, 0, 0,
							      0);
			else if (renderable->vertexBuffer)
				commandBuffers[i].draw(static_cast<uint32_t>(renderable->vertexBuffer->data->num), 1, 0, 0);
		}

		commandBuffers[i].endRenderPass();
		commandBuffers[i].end();
	}
}

void gs_device::CreateSyncObjects()
{
	const auto logicalDevice = GetLogicalDevice();
	const auto &swapchain = loadedSwapchains.front();

	imageAvailableSemaphore = logicalDevice.createSemaphore({});
	renderFinishedSemaphore = logicalDevice.createSemaphore({});

	for (uint32_t i = 0; i < swapchain->imageCount; i++)
		inFlightFences.emplace_back(
			logicalDevice.createFence(vk::FenceCreateInfo(
				vk::FenceCreateFlagBits::eSignaled)));
}

void gs_device::Resize(uint32_t cx, uint32_t cy)
{
	if (currentSwapchain == -1 || commandBuffers.empty() || framebuffers.empty())
		return;

	const auto logicalDevice = GetLogicalDevice();
	logicalDevice.waitIdle();

	for (const auto &framebuffer : framebuffers)
		logicalDevice.destroyFramebuffer(framebuffer);

	framebuffers.clear();

	logicalDevice.freeCommandBuffers(commandPool, commandBuffers);
	commandBuffers.clear();

	const auto &swapchain = loadedSwapchains.front();
	swapchain->Recreate(cx, cy);

	CreateFramebuffers();
	CreateCommandBuffers();
}

vk::Pipeline gs_device::CreateGraphicsPipeline(gs_vertex_shader *vertShader, gs_fragment_shader *fragShader, vk::PipelineLayout pipelineLayout)
{
	const auto logicalDevice = GetLogicalDevice();
	const auto &swapchain = loadedSwapchains.front();

	/* shader modules */
	const std::array shaderStages{
		vk::PipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eVertex,
			vertShader->module, "main"),
		vk::PipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eFragment,
			fragShader->module, "main")};

	/* vertex input */
	const vk::VertexInputBindingDescription vertexInputBindingDescription(
		0, vertShader->shaderInputs.lastOffset, vk::VertexInputRate::eVertex);

	const vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo(
		{}, 1, &vertexInputBindingDescription,
		static_cast<uint32_t>(vertShader->shaderInputs.descs.size()),
		vertShader->shaderInputs.descs.data());

	/* input assembly */
	const vk::PipelineInputAssemblyStateCreateInfo
		inputAssemblyStateCreateInfo(
			{}, vk::PrimitiveTopology::eTriangleList);

	/* viewport and scissor */
	const vk::Viewport viewport(
		0.0f, 0.0f, static_cast<float>(swapchain->extent.width),
		static_cast<float>(swapchain->extent.height), 0.0f,
		1.0f);

	const vk::Rect2D scissor({0, 0}, swapchain->extent);

	const vk::PipelineViewportStateCreateInfo viewportStateCreateInfo(
		{}, 1, &viewport, 1, &scissor);

	/* rasterization */
	const vk::PipelineRasterizationStateCreateInfo
		rasterizationStateCreateInfo({}, false, false,
					     vk::PolygonMode::eFill,
					     vk::CullModeFlagBits::eNone,
					     vk::FrontFace::eClockwise,
					     false, 0.0f, 0.0f, 0.0f, 1.0f);

	/* color blending */
	const vk::ColorComponentFlags colorComponentFlags(
		vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA);

	const vk::PipelineColorBlendAttachmentState colorBlendAttachmentState(
		true, vk::BlendFactor::eSrcAlpha,
		vk::BlendFactor::eOneMinusSrcAlpha, vk::BlendOp::eAdd,
		vk::BlendFactor::eOne, vk::BlendFactor::eZero,
		vk::BlendOp::eAdd, colorComponentFlags);

	const vk::PipelineColorBlendStateCreateInfo colorBlendStateCreateInfo(
		{}, false, vk::LogicOp::eCopy, 1, &colorBlendAttachmentState,
		{0.0f, 0.0f, 0.0f, 0.0f});

	/* depth stencil */
	const vk::PipelineDepthStencilStateCreateInfo
		depthStencilStateCreateInfo({}, true, true,
					    vk::CompareOp::eLessOrEqual);

	/* multisampling (disabled) */
	const vk::PipelineMultisampleStateCreateInfo multisampleStateCreateInfo;

	/* dynamic state */
	const std::array dynamicStateEnables = {vk::DynamicState::eViewport,
						vk::DynamicState::eScissor};

	const vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo(
		{}, static_cast<uint32_t>(dynamicStateEnables.size()),
		dynamicStateEnables.data());

	const vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
		{}, static_cast<uint32_t>(shaderStages.size()), shaderStages.data(), &vertexInputStateCreateInfo,
		&inputAssemblyStateCreateInfo, nullptr,
		&viewportStateCreateInfo, &rasterizationStateCreateInfo,
		&multisampleStateCreateInfo, &depthStencilStateCreateInfo,
		&colorBlendStateCreateInfo, &dynamicStateCreateInfo,
		pipelineLayout, renderPass);

	try {
		return logicalDevice
			.createGraphicsPipeline({}, pipelineCreateInfo)
			.value;
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

vk::ShaderModule gs_device::CreateShaderModule(gs_shader_t *shader)
{
	const auto logicalDevice = GetLogicalDevice();

	vk::ShaderModuleCreateInfo shaderCreateInfo;
	if (shader->type == GS_SHADER_VERTEX) {
		shaderCreateInfo = vk::ShaderModuleCreateInfo(
			{}, shader->spirv.size() * sizeof(uint32_t),
			shader->spirv.data());
	} else if (shader->type == GS_SHADER_PIXEL) {
		shaderCreateInfo = vk::ShaderModuleCreateInfo(
			{}, shader->spirv.size() * sizeof(uint32_t),
			shader->spirv.data());
	} else
		throw std::runtime_error("Invalid Shader type");

	try {
		return logicalDevice.createShaderModule(shaderCreateInfo,
							nullptr);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

std::tuple<vk::Image, vk::DeviceMemory> gs_device::CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	const auto physicalDevice = GetPhysicalDevice();
	const auto logicalDevice = GetLogicalDevice();

	const vk::ImageCreateInfo imageCreateInfo(
		{}, vk::ImageType::e2D, format, {width, height, 1}, 1, 1,
		vk::SampleCountFlagBits::e1, tiling, usage,
		vk::SharingMode::eExclusive);

	vk::Image image;
	try {
		image = logicalDevice.createImage(imageCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}

	const auto memoryRequirements =
		logicalDevice.getImageMemoryRequirements(image);

	const vk::MemoryAllocateInfo memoryAllocateInfo(
		memoryRequirements.size,
		vk_findMemoryType(physicalDevice.getMemoryProperties(),
				  memoryRequirements.memoryTypeBits,
				  properties));

	vk::DeviceMemory deviceMemory;
	try {
		deviceMemory =
			logicalDevice.allocateMemory(memoryAllocateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}

	logicalDevice.bindImageMemory(image, deviceMemory, 0);

	return std::make_tuple(image, deviceMemory);
}

vk::ImageView gs_device::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
	const vk::ImageViewCreateInfo imageViewCreateInfo(
		{}, image, vk::ImageViewType::e2D, format,
		{vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
		 vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
		{vk::ImageSubresourceRange(aspectFlags, 0, 1, 0, 1)});

	try {
		return GetLogicalDevice().createImageView(imageViewCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

void gs_device::BeginCommandBuffer()
{
	try {
		instantBuffer.begin(vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		if (!instantFence)
			instantFence = GetLogicalDevice().createFence({});
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

void gs_device::EndCommandBuffer()
{
	const auto logicalDevice = GetLogicalDevice();

	instantBuffer.end();

	queue.submit(vk::SubmitInfo({}, {}, {}, 1, &instantBuffer), instantFence);

	auto res = logicalDevice.waitForFences(1, &instantFence, true, std::numeric_limits<uint64_t>::max());
	if (res != vk::Result::eSuccess)
		throw std::runtime_error("Failed to wait for fence");

	res = logicalDevice.resetFences(1, &instantFence);
	if (res != vk::Result::eSuccess)
		throw std::runtime_error("Failed to reset fence");

	logicalDevice.resetCommandPool(instantPool);
}

#include <iostream>
void gs_device::GarbageCollect()
{
	/* cleaning up buffers */
	uint32_t markedForDeletion = 0;
	for (size_t i = 0; i < loadedBuffers.size(); i++) {
		if (loadedBuffers[i]->markedForDeletion) {
			markedForDeletion++;
			// delete buffer from loadedBuffers
			loadedBuffers.erase(loadedBuffers.begin() + i);
		}
	}
	if (markedForDeletion > 0)
		std::cout << "[Vulkan::GC] Buffers: " << loadedBuffers.size() << " - GC'd: " << markedForDeletion << std::endl;

	/* cleaning up shaders */
	markedForDeletion = 0;
	for (size_t i = 0; i < loadedShaders.size(); i++) {
		if (loadedShaders[i]->vertexShader->markedForDeletion ||
		    loadedShaders[i]->fragmentShader->markedForDeletion) {
			markedForDeletion++;
			loadedShaders.erase(loadedShaders.begin() + i);
		}
	}
	if (markedForDeletion > 0)
		std::cout << "[Vulkan::GC] Shaders: " << loadedShaders.size() << " - GC'd: " << markedForDeletion << std::endl;

	/* cleaning up textures */
	markedForDeletion = 0;
	for (size_t i = 0; i < loadedTextures.size(); i++) {
		if (loadedTextures[i]->markedForDeletion) {
			markedForDeletion++;
			loadedTextures.erase(loadedTextures.begin() + i);
		}
	}
	if (markedForDeletion > 0)
		std::cout << "[Vulkan::GC] Textures: " << loadedTextures.size() << " - GC'd: " << markedForDeletion << std::endl;

	/* cleaning up renderables */
	markedForDeletion = 0;
	for (size_t i = 0; i < loadedRenderables.size(); i++) {
		if (loadedRenderables[i]->vertexBuffer->markedForDeletion || (loadedRenderables[i]->indexBuffer && loadedRenderables[i]->indexBuffer->markedForDeletion)) {
			markedForDeletion++;
			loadedRenderables.erase(loadedRenderables.begin() + i);
		}
	}
	if (markedForDeletion > 0)
		std::cout << "[Vulkan::GC] Renderables: " << loadedRenderables.size() << " - GC'd: " << markedForDeletion << std::endl;
}

gs_shader_t *gs_device::SubmitShader(std::unique_ptr<gs_shader_t> shader)
{
	auto [shrIdx, type] = GetLoadedShaderIdx(shader.get());
	if (shrIdx != -1) {
		currentShader = shrIdx;
		GarbageCollect();

		auto &shr = loadedShaders[shrIdx];
		if (!shr->pipeline)
			shr->Recreate();

		if (type == vk_type::VK_VERTEXBUFFER)
			return shr->vertexShader.get();
		else if (type == vk_type::VK_INDEXBUFFER)
			return shr->fragmentShader.get();
	}

	/* if OBS passes fragment shader, check if we already have a vertex one to match it */
	if (lastVertexShader && shader->type == GS_SHADER_PIXEL) {
		loadedShaders.emplace_back(std::make_unique<VulkanShader>(
			this,
			static_cast<gs_vertex_shader *>(
				lastVertexShader.release()),
			static_cast<gs_fragment_shader *>(shader.release())));

		lastVertexShader = nullptr;
		GarbageCollect();
		return loadedShaders.back()->fragmentShader.get();
	} else if (!lastVertexShader && shader->type == GS_SHADER_VERTEX) {
		lastVertexShader = std::move(shader);
		GarbageCollect();
		return lastVertexShader.get();
	}

	return nullptr;
}

void gs_device::SetShader(gs_shader_t *shader)
{
	GarbageCollect();
	if (!shader) {
		currentShader = -1;
		return;
	}

	auto [shrIdx, type] = GetLoadedShaderIdx(shader);
	if (shrIdx != -1)
		currentShader = shrIdx;
}

gs_buffer *gs_device::SubmitBuffer(std::unique_ptr<gs_buffer> buffer)
{
	auto [bufIdx, type] = GetLoadedBufferIdx(buffer.get());
	if (bufIdx != -1) {
		if (type == vk_type::VK_VERTEXBUFFER)
			currentVertexBuffer = bufIdx;
		else if (type == vk_type::VK_INDEXBUFFER)
			currentIndexBuffer = bufIdx;

		GarbageCollect();

		return loadedBuffers[bufIdx].get();
	}

	auto buf = loadedBuffers.emplace_back(std::move(buffer)).get();
	if (buf->type == vk_type::VK_VERTEXBUFFER)
		currentVertexBuffer = static_cast<uint32_t>(loadedBuffers.size() - 1);
	else if (buf->type == vk_type::VK_INDEXBUFFER)
		currentIndexBuffer = static_cast<uint32_t>(loadedBuffers.size() - 1);

	GarbageCollect();

	return buf;
}

void gs_device::SetBuffer(gs_buffer *buffer)
{
	GarbageCollect();
	if (!buffer) {
		currentVertexBuffer = -1;
		currentIndexBuffer = -1;
		return;
	}

	auto [bufIdx, type] = GetLoadedBufferIdx(buffer);
	if (bufIdx != -1) {
		if (type == vk_type::VK_VERTEXBUFFER)
			currentVertexBuffer = bufIdx;
		else if (type == vk_type::VK_INDEXBUFFER)
			currentIndexBuffer = bufIdx;

		if (currentIndexBuffer != -1 && currentVertexBuffer != -1 && currentShader != -1) {
			SubmitRenderable(
				loadedShaders[currentShader].get(),
				static_cast<gs_vertex_buffer *>(
					loadedBuffers[currentVertexBuffer]
						.get()),
				static_cast<gs_index_buffer *>(
					loadedBuffers[currentIndexBuffer]
						.get()));
		} else if (currentVertexBuffer != -1 && currentShader != -1) {
			SubmitRenderable(
				loadedShaders[currentShader].get(),
				static_cast<gs_vertex_buffer *>(
					loadedBuffers[currentVertexBuffer]
						.get()),
				nullptr);
		}
	}
}

gs_texture_t *gs_device::SubmitTexture(std::unique_ptr<gs_texture_t> texture)
{
	int32_t texIdx = -1;
	if ((texIdx = GetLoadedTextureIdx(texture.get())) != -1) {
		currentTexture = texIdx;
		GarbageCollect();
		return loadedTextures[texIdx].get();
	}

	GarbageCollect();

	texture->samplerState = defaultSampler.get();
	return loadedTextures.emplace_back(std::move(texture)).get();
}

void gs_device::SetTexture(gs_texture_t *texture)
{
	GarbageCollect();
	if (!texture) {
		currentTexture = -1;
		return;
	}

	const auto texIdx = GetLoadedTextureIdx(texture);
	if (texIdx != -1)
		currentTexture = texIdx;
}

void gs_device::SubmitRenderable(VulkanShader *shader, gs_vertex_buffer *vertexBuffer,
				 gs_index_buffer *indexBuffer)
{
	if (!shader || !vertexBuffer)
		return;

	int32_t rndIdx = -1;
	if ((rndIdx = GetLoadedRenderableIdx(vertexBuffer)) != -1) {
		currentRenderable = rndIdx;
		GarbageCollect();
		return;
	}

	try {
		loadedRenderables.emplace_back(std::make_unique<VulkanRenderable>(
			this, shader, vertexBuffer, indexBuffer));
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}

	GarbageCollect();
}

std::tuple<int32_t, vk_type> gs_device::GetLoadedBufferIdx(gs_buffer *buffer)
{
	for (uint32_t i = 0; i < loadedBuffers.size(); i++) {
		if (loadedBuffers[i].get() == buffer)
			return std::make_tuple(i, loadedBuffers[i]->type);
	}
	return std::make_tuple(-1, vk_type::VK_INVALID);
}

std::tuple<int32_t, vk_type> gs_device::GetLoadedShaderIdx(gs_shader_t *shader)
{
	for (uint32_t i = 0; i < loadedShaders.size(); i++) {
		if (loadedShaders[i]->vertexShader.get() == shader)
			return std::make_tuple(i, vk_type::VK_VERTEXSHADER);
		else if (loadedShaders[i]->fragmentShader.get() == shader)
			return std::make_tuple(i, vk_type::VK_FRAGMENTSHADER);
	}
	return std::make_tuple(-1, vk_type::VK_INVALID);
}

int32_t gs_device::GetLoadedRenderableIdx(vk_object *buffer)
{
	for (uint32_t i = 0; i < loadedRenderables.size(); i++) {
		if (loadedRenderables[i]->vertexBuffer == buffer)
			return i;
		else if (loadedRenderables[i]->indexBuffer == buffer)
			return i;
	}
	return -1;
}

int32_t gs_device::GetLoadedTextureIdx(gs_texture_t *texture)
{
	for (uint32_t i = 0; i < loadedTextures.size(); i++) {
		if (loadedTextures[i].get() == texture)
			return i;
	}
	return -1;
}

int32_t gs_device::GetLoadedSwapchainIdx(const gs_init_data *data)
{
	for (uint32_t i = 0; i < loadedSwapchains.size(); i++) {
		if (loadedSwapchains[i]->initData->window.hwnd == data->window.hwnd)
			return i;
	}
	return -1;
}

bool ColorIsSame(const vec4 *color, const vec4 *newColor)
{
	const auto epsilon = std::numeric_limits<float>::epsilon();
	return std::abs(color->x - newColor->x) < epsilon &&
		   std::abs(color->y - newColor->y) < epsilon &&
		   std::abs(color->z - newColor->z) < epsilon &&
		   std::abs(color->w - newColor->w) < epsilon;
}

void gs_device::SetClearColor(const vec4 *color)
{
	if (color && !ColorIsSame(clearColor, color)) {
		clearColor->x = color->x;
		clearColor->y = color->y;
		clearColor->z = color->z;
		clearColor->w = color->w;
	}
}

void gs_device::UpdateDraw(uint32_t startVert, uint32_t nVert)
{
	if (currentRenderable < 0 || loadedRenderables.empty() || lastRenderable == currentRenderable)
		return;

	const auto logicalDevice = GetLogicalDevice();
	logicalDevice.waitIdle();

	const auto &renderable = loadedRenderables[currentRenderable];
	if (renderable->descriptorSets.empty() && (renderable->shader->fragmentShader->samplers.empty() || currentTexture >= 0))
		renderable->descriptorSets =
			CreateDescriptorSets(renderable->shader);

	if (!renderable->shader->pipeline)
		renderable->shader->Recreate();

	gs_effect_t *effect = gs_get_effect();
	if (effect)
		gs_effect_update_params(effect);

	gs_matrix_get(&currentView);
	matrix4_mul(&currentViewProjection, &currentView, &currentProjection);
	//matrix4_transpose(&currentViewProjection, &currentViewProjection);

	const auto &shr = renderable->shader;
	if (shr->vertexShader->viewProjection)
		gs_shader_set_matrix4(shr->vertexShader->viewProjection,
				      &currentViewProjection);

	shr->vertexShader->UploadParams();
	shr->fragmentShader->UploadParams();

	RecreateCommandBuffers();

	lastRenderable = currentRenderable;
}

void gs_device::RecreateCommandBuffers()
{
	if (commandBuffers.empty())
		return;

	const auto logicalDevice = GetLogicalDevice();
	logicalDevice.waitIdle();

	logicalDevice.freeCommandBuffers(commandPool, commandBuffers);
	commandBuffers.clear();

	CreateCommandBuffers();
}

vk::Format
gs_device::FindSupportedFormat(const std::vector<vk::Format> &formats,
			       vk::ImageTiling tiling,
			       vk::FormatFeatureFlags featureFlags)
{
	const auto physicalDevice = GetPhysicalDevice();

	for (const auto &format : formats) {
		const auto properties =
			physicalDevice.getFormatProperties(format);

		if (tiling == vk::ImageTiling::eLinear &&
		    (properties.linearTilingFeatures & featureFlags) ==
			    featureFlags) {
			return format;
		} else if (tiling == vk::ImageTiling::eOptimal &&
			   (properties.optimalTilingFeatures & featureFlags) ==
				   featureFlags) {
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format");
}

gs_device::gs_device(vulkan_instance *_instance, const vk::PhysicalDevice &physicalDevice)
	: instance(_instance)
{
	deviceProperties = physicalDevice.getProperties();
	deviceName = deviceProperties.deviceName.data();
	deviceID = deviceProperties.deviceID;
	vendorID = deviceProperties.vendorID;
	std::get<0>(device) = physicalDevice;
	clearColor = new vec4{0.0f, 0.0f, 0.0f, 1.0f};
	matrix4_identity(&currentView);
	matrix4_identity(&currentProjection);
	matrix4_identity(&currentViewProjection);

	scissor = vk::Rect2D({0, 0}, {1, 1});
	viewport = vk::Viewport(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f);

	try {
		blog(LOG_INFO, "%s: CreateLogicalDevice", deviceName.c_str());
		CreateLogicalDevice();
		blog(LOG_INFO, "%s: CreateCommandPool", deviceName.c_str());
		CreateCommandPool();
		blog(LOG_INFO, "%s: CreateDescriptorPool", deviceName.c_str());
		CreateDescriptorPool();
	} catch (const std::runtime_error &error) {
		throw error;
	}

	gs_sampler_info defaultSamplerInfo;
	defaultSamplerInfo.filter = GS_FILTER_POINT;
	defaultSamplerInfo.address_u = GS_ADDRESS_BORDER;
	defaultSamplerInfo.address_v = GS_ADDRESS_BORDER;
	defaultSamplerInfo.address_w = GS_ADDRESS_BORDER;
	defaultSamplerInfo.max_anisotropy = 1;
	defaultSamplerInfo.border_color = 0;
	defaultSampler =
		std::unique_ptr<gs_sampler_state>(device_samplerstate_create(this, &defaultSamplerInfo));
}

gs_device::~gs_device()
{
	const auto logicalDevice = GetLogicalDevice();

	logicalDevice.waitIdle();
	for (const auto &framebuffer : framebuffers)
		logicalDevice.destroyFramebuffer(framebuffer);

	framebuffers.clear();

	if (!commandBuffers.empty())
		logicalDevice.freeCommandBuffers(commandPool, commandBuffers);

	commandBuffers.clear();

	loadedSwapchains.clear();

	logicalDevice.destroyDescriptorPool(descriptorPool);
	currentShader = {};
	logicalDevice.destroyRenderPass(renderPass);
	logicalDevice.destroySemaphore(imageAvailableSemaphore);
	logicalDevice.destroySemaphore(renderFinishedSemaphore);
	for (const auto &fences : inFlightFences)
		logicalDevice.destroyFence(fences);

	inFlightFences.clear();

	logicalDevice.destroyCommandPool(commandPool);
}
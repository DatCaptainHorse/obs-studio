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
	if (currentSwapchain) {
		blog(LOG_ERROR, "%s: Swapchain already exists",
		     deviceName.c_str());
		return nullptr;
	}

	try {
		currentSwapchain = std::make_unique<gs_swap_chain>(
			this, data,
			std::make_unique<vulkan_surface>(instance, data),
			queueFamilyIndex);
	} catch (const std::runtime_error &error) {
		blog(LOG_ERROR, "%s: Failed to create swapchain: %s",
		     deviceName.c_str(), error.what());
		return nullptr;
	}

	blog(LOG_INFO, "%s: CreateRenderPasses", deviceName.c_str());
	CreateRenderPasses();
	blog(LOG_INFO, "%s: CreateFramebuffers", deviceName.c_str());
	CreateFramebuffers();
	blog(LOG_INFO, "%s: CreateCommandBuffers", deviceName.c_str());
	CreateCommandBuffers();
	blog(LOG_INFO, "%s: CreateSyncObjects", deviceName.c_str());
	CreateSyncObjects();

	return currentSwapchain.get();
}

void gs_device::CreateRenderPasses()
{
	const auto logicalDevice = GetLogicalDevice();

	const auto depthFormat = FindSupportedFormat(
		{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
		 vk::Format::eD24UnormS8Uint},
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment);

	const std::array attachmentDescriptions = {
		vk::AttachmentDescription({}, currentSwapchain->format,
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

void gs_device::CreateDescriptorSetLayout()
{
	const auto logicalDevice = GetLogicalDevice();

	const std::array descriptorSetLayoutBindings = {
		vk::DescriptorSetLayoutBinding(
			0, vk::DescriptorType::eCombinedImageSampler, 1,
			vk::ShaderStageFlagBits::eFragment)};

	const vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo(
		{}, static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
		descriptorSetLayoutBindings.data());

	try {
		descriptorSetLayout = logicalDevice.createDescriptorSetLayout(
			descriptorSetLayoutCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

void gs_device::CreatePipelineLayout()
{
	const auto logicalDevice = GetLogicalDevice();

	const vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
		{}, 1, &descriptorSetLayout, 0, nullptr);

	try {
		pipelineLayout = logicalDevice.createPipelineLayout(
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
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

/* TODO: See if enough or could be dynamic */
constexpr uint32_t maxPoolSize = 100;

void gs_device::CreateDescriptorPool()
{
	const auto logicalDevice = GetLogicalDevice();

	const std::array descriptorPoolSizes = {vk::DescriptorPoolSize(
		vk::DescriptorType::eCombinedImageSampler, maxPoolSize)};

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

	for (uint32_t i = 0; i < currentSwapchain->imageCount; i++) {
		const std::array attachments = {
			currentSwapchain->colorImages[i].imageView,
			currentSwapchain->depthImages[i].imageView};

		const vk::FramebufferCreateInfo framebufferCreateInfo(
			{}, renderPass,
			static_cast<uint32_t>(attachments.size()),
			attachments.data(), currentSwapchain->extent.width,
			currentSwapchain->extent.height, 1);

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

		const vk::Viewport viewport(
			0.0f, 0.0f,
			static_cast<float>(currentSwapchain->extent.width),
			static_cast<float>(currentSwapchain->extent.height),
			0.0f, 1.0f);

		const vk::Rect2D scissor({0, 0}, currentSwapchain->extent);

		const std::array clearValues = {
			vk::ClearValue(vk::ClearColorValue(std::array<float, 4>(
				{0.0f, 0.0f, 0.0f, 1.0f}))),
			vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0))};

		const vk::RenderPassBeginInfo renderPassBeginInfo(
			renderPass, framebuffers[i],
			vk::Rect2D({0, 0}, currentSwapchain->extent),
			static_cast<uint32_t>(clearValues.size()),
			clearValues.data());

		commandBuffers[i].setViewport(0, viewport);
		commandBuffers[i].setScissor(0, scissor);
		commandBuffers[i].beginRenderPass(renderPassBeginInfo,
						  vk::SubpassContents::eInline);

		/* TODO: This */

		commandBuffers[i].endRenderPass();
		commandBuffers[i].end();
	}
}

void gs_device::CreateSyncObjects()
{
	const auto logicalDevice = GetLogicalDevice();
	imageAvailableSemaphore = logicalDevice.createSemaphore({});
	renderFinishedSemaphore = logicalDevice.createSemaphore({});

	const vk::FenceCreateInfo fenceCreateInfo(
		vk::FenceCreateFlagBits::eSignaled);

	for (uint32_t i = 0; i < currentSwapchain->imageCount; i++)
		inFlightFences.emplace_back(
			logicalDevice.createFence(fenceCreateInfo));
}

void gs_device::Resize(uint32_t cx, uint32_t cy)
{
	const auto logicalDevice = GetLogicalDevice();

	logicalDevice.waitIdle();

	for (const auto &framebuffer : framebuffers)
		logicalDevice.destroyFramebuffer(framebuffer);

	framebuffers.clear();

	logicalDevice.freeCommandBuffers(commandPool, commandBuffers);
	commandBuffers.clear();

	currentSwapchain->Recreate(cx, cy);
	logicalDevice.destroyDescriptorPool(descriptorPool);

	CreateFramebuffers();
	CreateDescriptorPool();
	CreateCommandBuffers();
}

vk::Pipeline gs_device::CreateGraphicsPipeline(vk::ShaderModule vertexShader,
					       vk::ShaderModule fragmentShader)
{
	const auto logicalDevice = GetLogicalDevice();

	/* shared modules */
	const std::array shaderStages{
		vk::PipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eVertex,
			vertexShader, "main"),
		vk::PipelineShaderStageCreateInfo(
			{}, vk::ShaderStageFlagBits::eFragment,
			fragmentShader, "main")};

	/* vertex input */
	const vk::VertexInputBindingDescription vertexInputBindingDescription(
		0, sizeof(gs_vb_data), vk::VertexInputRate::eVertex);

	const std::array vertexInputAttributeDescriptions = {
		vk::VertexInputAttributeDescription(
			0, 0, vk::Format::eR32G32B32Sfloat,
			offsetof(gs_vb_data, gs_vb_data::points)),
		vk::VertexInputAttributeDescription(
			1, 0, vk::Format::eR32G32B32Sfloat,
			offsetof(gs_vb_data, gs_vb_data::normals)),
		vk::VertexInputAttributeDescription(
			2, 0, vk::Format::eR32G32B32Sfloat,
			offsetof(gs_vb_data, gs_vb_data::tangents)),
		vk::VertexInputAttributeDescription(
			3, 0, vk::Format::eR32Sfloat,
			offsetof(gs_vb_data, gs_vb_data::colors)),
	};

	const vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo(
		{}, 1, &vertexInputBindingDescription,
		static_cast<uint32_t>(vertexInputAttributeDescriptions.size()),
		vertexInputAttributeDescriptions.data());

	/* input assembly */
	const vk::PipelineInputAssemblyStateCreateInfo
		inputAssemblyStateCreateInfo(
			{}, vk::PrimitiveTopology::eTriangleList);

	/* viewport and scissor */
	const vk::Viewport viewport(
		0.0f, 0.0f, static_cast<float>(currentSwapchain->extent.width),
		static_cast<float>(currentSwapchain->extent.height), 0.0f,
		1.0f);

	const vk::Rect2D scissor({0, 0}, currentSwapchain->extent);

	const vk::PipelineViewportStateCreateInfo viewportStateCreateInfo(
		{}, 1, &viewport, 1, &scissor);

	/* rasterization */
	const vk::PipelineRasterizationStateCreateInfo
		rasterizationStateCreateInfo({}, false, false,
					     vk::PolygonMode::eFill,
					     vk::CullModeFlagBits::eNone,
					     vk::FrontFace::eCounterClockwise,
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
		{}, 2, shaderStages.data(), &vertexInputStateCreateInfo,
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

vk::ShaderModule gs_device::CreateShaderModule(gs_shader *shader)
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

vk::CommandBuffer gs_device::BeginCommandBuffer()
{
	const auto logicalDevice = GetLogicalDevice();

	const vk::CommandBufferAllocateInfo commandBufferAllocateInfo(
		commandPool, vk::CommandBufferLevel::ePrimary, 1);

	try {
		const auto commandBuffer = logicalDevice.allocateCommandBuffers(
			commandBufferAllocateInfo)[0];

		commandBuffer.begin(vk::CommandBufferBeginInfo(
			vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

		return commandBuffer;
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

void gs_device::EndCommandBuffer(const vk::CommandBuffer &commandBuffer)
{
	const auto logicalDevice = GetLogicalDevice();

	commandBuffer.end();

	const vk::SubmitInfo submitInfo({}, {}, {}, 1, &commandBuffer);
	queue.submit(submitInfo, nullptr);
	queue.waitIdle();

	logicalDevice.freeCommandBuffers(commandPool, commandBuffer);
}

vulkan_image gs_device::CreateImage(uint32_t width, uint32_t height,
				    vk::Format format, vk::ImageTiling tiling,
				    vk::ImageUsageFlags usage,
				    vk::MemoryPropertyFlags properties)
{
	vulkan_image image = {};
	const auto physicalDevice = GetPhysicalDevice();
	const auto logicalDevice = GetLogicalDevice();

	const vk::ImageCreateInfo imageCreateInfo(
		{}, vk::ImageType::e2D, format, {width, height, 1}, 1, 1,
		vk::SampleCountFlagBits::e1, tiling, usage,
		vk::SharingMode::eExclusive);

	try {
		image.image = logicalDevice.createImage(imageCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}

	const auto memoryRequirements =
		logicalDevice.getImageMemoryRequirements(image.image);

	const vk::MemoryAllocateInfo memoryAllocateInfo(
		memoryRequirements.size,
		vk_findMemoryType(physicalDevice.getMemoryProperties(),
				  memoryRequirements.memoryTypeBits,
				  properties));

	try {
		image.deviceMemory =
			logicalDevice.allocateMemory(memoryAllocateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}

	logicalDevice.bindImageMemory(image.image, image.deviceMemory, 0);

	return image;
}

vk::ImageView gs_device::CreateImageView(vk::Image image,
					 vk::ImageViewType viewType,
					 vk::Format format,
					 vk::ImageAspectFlags aspectFlags)
{
	const auto logicalDevice = GetLogicalDevice();
	const vk::ImageViewCreateInfo imageViewCreateInfo(
		{}, image, viewType, format,
		{vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
		 vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA},
		{vk::ImageSubresourceRange(aspectFlags, 0, 1, 0, 1)});

	try {
		return logicalDevice.createImageView(imageViewCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
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
	: instance(_instance), currentSwapchain(nullptr)
{
	const auto properties = physicalDevice.getProperties();
	deviceName = properties.deviceName.data();
	deviceID = properties.deviceID;
	vendorID = properties.vendorID;
	std::get<0>(device) = physicalDevice;

	try {
		blog(LOG_INFO, "%s: CreateLogicalDevice", deviceName.c_str());
		CreateLogicalDevice();
		blog(LOG_INFO, "%s: CreateDescriptorSetLayout", deviceName.c_str());
		CreateDescriptorSetLayout();
		blog(LOG_INFO, "%s: CreatePipelineLayout", deviceName.c_str());
		CreatePipelineLayout();
		blog(LOG_INFO, "%s: CreateCommandPool", deviceName.c_str());
		CreateCommandPool();
		blog(LOG_INFO, "%s: CreateDescriptorPool", deviceName.c_str());
		CreateDescriptorPool();
	} catch (const std::runtime_error &error) {
		throw error;
	}
}

gs_device::~gs_device()
{
	/* TODO: This */
}
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

#pragma once

#include <array>
#include <tuple>
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <algorithm>

#include "vk-includes.hpp"

#include <util/base.h>
#include <graphics/graphics.h>
#include <graphics/device-exports.h>

struct gs_device;
struct gs_buffer;
struct shader_var;
struct shader_sampler;
struct vulkan_instance;

static inline const char *GetVulkanVendor(std::uint32_t vendorID)
{
	switch (vendorID) {
	case 4098:
		return "AMD";
	case 4318:
		return "NVIDIA";
	case 32902:
		return "Intel";
	case 20803:
		return "Qualcomm";
	case 5045:
		return "ARM";
	case 4112:
		return "ImgTec";
	default:
		return "Unknown";
	}
}

/* Returns given vk::DriverId back as a nice const char *
 * REM: Most of these will probably never see OBS,
 * however it's nice to have them? */
static inline const char *GetVulkanDriverID(vk::DriverId driverID)
{
	switch (driverID) {
	case vk::DriverId::eAmdProprietary:
		return "AMD Proprietary";
	case vk::DriverId::eAmdOpenSource:
		return "AMD Open Source";
	case vk::DriverId::eMesaRadv:
		return "Mesa RADV";
	case vk::DriverId::eNvidiaProprietary:
		return "NVIDIA Proprietary";
	case vk::DriverId::eIntelProprietaryWindows:
		return "Intel Proprietary Windows";
	case vk::DriverId::eIntelOpenSourceMESA:
		return "Intel Open Source Mesa";
	case vk::DriverId::eImaginationProprietary:
		return "Imagination Proprietary";
	case vk::DriverId::eQualcommProprietary:
		return "Qualcomm Proprietary";
	case vk::DriverId::eArmProprietary:
		return "ARM Proprietary";
	case vk::DriverId::eGoogleSwiftshader:
		return "Google SwiftShader";
	case vk::DriverId::eGgpProprietary:
		return "GGP Proprietary";
	case vk::DriverId::eBroadcomProprietary:
		return "Broadcom Proprietary";
	case vk::DriverId::eMesaLlvmpipe:
		return "Mesa LLVMpipe";
	case vk::DriverId::eMoltenvk:
		return "MoltenVK";
	case vk::DriverId::eJuiceProprietary:
		return "Juice Proprietary";
	case vk::DriverId::eVerisiliconProprietary:
		return "VeriSilicon Proprietary";
	case vk::DriverId::eMesaTurnip:
		return "Mesa Turnip";
	case vk::DriverId::eMesaV3Dv:
		return "Mesa v3dv";
	case vk::DriverId::eMesaPanvk:
		return "Mesa PanVK";
	case vk::DriverId::eSamsungProprietary:
		return "Samsung Proprietary";
	case vk::DriverId::eMesaVenus:
		return "Mesa Venus";
	default:
		return "Unknown";
	}
}

static inline const char *GetVulkanDeviceType(vk::PhysicalDeviceType type)
{
	switch (type) {
	case vk::PhysicalDeviceType::eDiscreteGpu:
		return "Discrete GPU";
	case vk::PhysicalDeviceType::eIntegratedGpu:
		return "Integrated GPU";
	case vk::PhysicalDeviceType::eCpu:
		return "CPU";
	case vk::PhysicalDeviceType::eVirtualGpu:
		return "Virtual GPU";
	case vk::PhysicalDeviceType::eOther:
		return "Other";
	default:
		return "Unknown";
	}
}

struct vulkan_surface {
	uint32_t width, height;
	vk::SurfaceKHR surfaceKHR;
	vulkan_instance *instance;

	vulkan_surface(vulkan_instance *_instance, const gs_init_data *data);
	~vulkan_surface();
};

struct vulkan_image {
	vk::Image image;
	vk::ImageView imageView;
	vk::DeviceMemory deviceMemory;

	inline vulkan_image() {}
	inline vulkan_image(vk::Image image) : image(image) {}
};

struct gs_swap_chain {
	gs_device_t *device;
	std::unique_ptr<gs_init_data> initData;
	vk::Extent2D extent;
	vk::SwapchainKHR swapchainKHR;
	vk::Format format;
	vk::ColorSpaceKHR colorSpaceKHR;
	vk::PresentModeKHR presentModeKHR;
	uint32_t imageCount = 0, usedFamilyIndex = 0;
	std::vector<vulkan_image> colorImages;
	std::vector<vulkan_image> depthImages;
	std::unique_ptr<vulkan_surface> surface;

	void Recreate(uint32_t cx, uint32_t cy);

	gs_swap_chain(gs_device_t *_device, const gs_init_data *data,
		      std::unique_ptr<vulkan_surface> _surface,
		      uint32_t queueFamilyIndex);

	~gs_swap_chain();
};

struct gs_stage_surface {
	gs_device_t *device;
	gs_color_format format;
	uint32_t width, height, bytes_per_pixel;

	std::unique_ptr<gs_buffer> packBuffer;

	gs_stage_surface() {}
	gs_stage_surface(gs_device_t *_device, gs_color_format _format,
			 uint32_t _width, uint32_t _height);
};

struct gs_sampler_state {
	gs_device_t *device;
	gs_sampler_info info;
	vk::Sampler sampler;

	gs_sampler_state() {}
	gs_sampler_state(gs_device_t *device, const gs_sampler_info *_info);
	gs_sampler_state(gs_device_t *device, vk::SamplerCreateInfo _info);
};

struct gs_buffer {
	gs_device_t *device;
	void *mapped;
	vk::Buffer buffer;
	vk::DeviceSize deviceSize;
	vk::DeviceMemory deviceMemory;
	vk::BufferUsageFlags bufferUsageFlags;
	vk::MemoryPropertyFlags memoryPropertyFlags;

	void Map();
	void Unmap();

	gs_buffer() {}
	gs_buffer(gs_device_t *_device, vk::DeviceSize size,
		  vk::BufferUsageFlags usage,
		  vk::MemoryPropertyFlags properties);

	~gs_buffer();
};

struct gs_vertex_buffer : gs_buffer {
	gs_vb_data *vbd;
	gs_vertex_buffer(gs_device_t *_device, vk::DeviceSize size,
			 const void *data);
};

struct gs_index_buffer : gs_buffer {
	void *indices;
	gs_index_buffer(gs_device_t *_device, vk::DeviceSize size,
			const void *data);
};

struct gs_uniform_buffer : gs_buffer {
	gs_uniform_buffer() {}
	gs_uniform_buffer(gs_device_t *_device, vk::DeviceSize size,
			  const void *data);
};

struct gs_texture {
	gs_device_t *device;
	gs_texture_type type;
	gs_color_format format;
	uint32_t flags;
	vulkan_image image;
	gs_sampler_state sampler;
	std::unique_ptr<gs_buffer> buffer;
	std::unique_ptr<gs_buffer> unpackBuffer;

	inline gs_texture(gs_device_t *_device, gs_texture_type _type)
		: device(_device), type(_type)
	{
	}

	inline gs_texture(gs_texture_type _type,
			  gs_color_format _format)
		: type(_type), format(_format)
	{
	}

	inline gs_texture(gs_device_t *_device,
			  gs_texture_type _type, gs_color_format _format, uint32_t _flags)
		: device(_device), type(_type), format(_format), flags(_flags)
	{
	}
};

struct gs_texture_2d : gs_texture {
	uint32_t width = 0, height = 0;

	inline gs_texture_2d() : gs_texture(GS_TEXTURE_2D, GS_UNKNOWN) {}

	gs_texture_2d(gs_device_t *_device, uint32_t _width, uint32_t _height,
		      gs_color_format colorFormat, const uint8_t *const *_data,
		      uint32_t flags);
};

struct gs_shader_param {
	std::string name;
	gs_shader_param_type type;

	int arrayCount;

	size_t pos;

	std::vector<uint8_t> curValue;
	std::vector<uint8_t> defaultValue;
	bool changed;

	gs_shader_param(shader_var &var);
};

struct gs_shader {
	gs_device_t *device;
	gs_shader_type type;
	vk::Pipeline pipeline;
	std::vector<uint32_t> spirv;
	vk::ShaderModule vertexModule, fragmentModule;
	std::vector<gs_shader_param> params;
	std::unique_ptr<gs_uniform_buffer> uniformBuffer;
	size_t constantSize;

	void BuildConstantBuffer();

	gs_shader(gs_device_t *device, gs_shader_type type, const char *source, const char *file);
	~gs_shader();
};

struct gs_vertex_shader : gs_shader {
	gs_vertex_shader(gs_device_t *device, const char *source, const char *file);
};

struct gs_fragment_shader : gs_shader {
	gs_fragment_shader(gs_device_t *device, const char *source, const char *file);
};

struct gs_device {
	vulkan_instance *instance = nullptr;

	std::string deviceName;
	uint32_t deviceID, vendorID;
	std::tuple<vk::PhysicalDevice, vk::Device> device;

	vk::Queue queue;
	uint32_t queueFamilyIndex = 0;
	std::unique_ptr<gs_swap_chain> currentSwapchain = nullptr;

	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorPool descriptorPool;

	vk::RenderPass renderPass;
	vk::PipelineLayout pipelineLayout;
	std::vector<vk::Framebuffer> framebuffers;

	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;

	uint32_t currentFrame = 0;
	std::vector<vk::Fence> inFlightFences;
	vk::Semaphore imageAvailableSemaphore, renderFinishedSemaphore;

	void CreateLogicalDevice();
	void CreateRenderPasses();
	void CreateDescriptorSetLayout();
	void CreatePipelineLayout();
	void CreateCommandPool();
	void CreateDescriptorPool();
	void CreateFramebuffers();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	void Resize(uint32_t cx, uint32_t cy);

	gs_swap_chain *CreateSwapchain(const gs_init_data *data);

	vk::ShaderModule CreateShaderModule(gs_shader *shader);
	vk::Pipeline CreateGraphicsPipeline(vk::ShaderModule vertexShader,
					    vk::ShaderModule fragmentShader);

	vk::CommandBuffer BeginCommandBuffer();
	void EndCommandBuffer(const vk::CommandBuffer &commandBuffer);

	vulkan_image CreateImage(uint32_t width, uint32_t height,
				 vk::Format format, vk::ImageTiling tiling,
				 vk::ImageUsageFlags usage,
				 vk::MemoryPropertyFlags properties);

	vk::ImageView CreateImageView(vk::Image image,
				      vk::ImageViewType viewType,
				      vk::Format format,
				      vk::ImageAspectFlags aspectFlags);

	vk::Format FindSupportedFormat(const std::vector<vk::Format> &formats,
				       vk::ImageTiling tiling,
				       vk::FormatFeatureFlags featureFlags);

	inline vk::PhysicalDevice GetPhysicalDevice() const
	{
		return std::get<0>(device);
	}
	inline vk::Device GetLogicalDevice() const
	{
		return std::get<1>(device);
	}

	gs_device(vulkan_instance *_instance, const vk::PhysicalDevice &physicalDevice);
	~gs_device();
};

struct vulkan_instance {
	vk::Instance vulkanInstance;
	std::vector<const char *> layers, extensions;
	std::vector<std::unique_ptr<gs_device>> devices; /* REM: Doing opposite of previous implementations, as there might be future use-cases for multi-GPU support */
	std::vector<std::unique_ptr<vulkan_surface>> surfaces;

	vulkan_instance(std::vector<const char *> requestedLayers,
			std::vector<const char *> requestedExtensions);

	~vulkan_instance();
};
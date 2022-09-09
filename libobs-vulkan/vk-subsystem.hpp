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

#include <map>
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
#include <graphics/vec2.h>
#include <graphics/matrix4.h>

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

enum class vk_type
{
	VK_INVALID,
	VK_GENERICBUFFER,
	VK_VERTEXBUFFER,
	VK_INDEXBUFFER,
	VK_UNIFORMBUFFER,
	VK_TEXTUREBUFFER,
	VK_RENDERABLE,
	VK_SAMPLER,
	VK_TEXTURE,
	VK_VERTEXSHADER,
	VK_FRAGMENTSHADER,
	VK_COMBINEDSHADER,
	VK_SWAPCHAIN
};

struct vk_vb_data {
	vec3 point;
	vec3 normal;
	vec3 tangent;
	uint32_t color;
	vec2 uv;
};

struct vk_vb_info {
	bool hasPoints = false;
	bool hasNormals = false;
	bool hasTangents = false;
	bool hasColors = false;
	bool hasUVs = false;
};

struct vk_object {
	gs_device_t *device = nullptr;
	bool markedForDeletion = false;
	vk_type type;

	vk_object() {}
	inline vk_object(gs_device_t *_device, vk_type _type) : device(_device), type(_type) {}
};

struct gs_buffer : vk_object {
	void *mapped = nullptr;
	vk::Buffer buffer;
	vk::DeviceSize deviceSize;
	vk::DeviceMemory deviceMemory;
	vk::BufferUsageFlags bufferUsageFlags;
	vk::MemoryPropertyFlags memoryPropertyFlags;

	void Map();
	void Unmap();
	void CreateBuffer(vk::DeviceSize size,
			  vk::BufferUsageFlags usage,
			  vk::MemoryPropertyFlags properties);

	inline virtual void Update(const void *data, size_t size) {}

	inline gs_buffer() {}
	gs_buffer(gs_device_t *_device, vk_type _type);
	~gs_buffer();
};

struct vulkan_surface {
	uint32_t width, height;
	vk::SurfaceKHR surfaceKHR;
	vulkan_instance *instance;

	vulkan_surface(vulkan_instance *_instance, const gs_init_data *data);
	~vulkan_surface();
};

struct gs_stage_surface {
	gs_device_t *device;
	gs_color_format format;
	uint32_t width, height, bytes_per_pixel;
	std::unique_ptr<gs_buffer> packBuffer;

	gs_stage_surface() {}
	gs_stage_surface(gs_device_t *_device, gs_color_format _format,
			 uint32_t _width, uint32_t _height);
	~gs_stage_surface();
};

struct gs_vertex_buffer : gs_buffer {
	gs_vb_data *data = nullptr;
	std::vector<vk_vb_data> vbd;
	std::unique_ptr<gs_buffer> hostBuffer = nullptr;

	void Update(const void *data, size_t size) override;

	gs_vertex_buffer(gs_device_t *_device, gs_vb_data *_data);
	inline ~gs_vertex_buffer() { hostBuffer.reset(); }
};

struct gs_index_buffer : gs_buffer {
	void *indices;
	size_t nIndices;
	gs_index_type indexType;
	std::unique_ptr<gs_buffer> hostBuffer = nullptr;

	gs_index_buffer(gs_device_t *_device, gs_index_type type, size_t _nIndices, vk::DeviceSize size,
			void *data);
	inline ~gs_index_buffer() { hostBuffer.reset(); }
};

struct gs_uniform_buffer : gs_buffer {
	std::unique_ptr<gs_buffer> hostBuffer = nullptr;

	void Update(const void *data, size_t size) override;

	gs_uniform_buffer() {}
	gs_uniform_buffer(gs_device_t *_device, vk::DeviceSize size);
};

struct gs_sampler_state : vk_object {
	vk::SamplerCreateInfo info;
	vk::Sampler sampler;

	gs_sampler_state() {}
	gs_sampler_state(gs_device_t *device, const gs_sampler_info *_info);
	inline gs_sampler_state(gs_device_t *device, vk::SamplerCreateInfo _info)
		: vk_object(device, vk_type::VK_SAMPLER), info(_info)
	{}
};

struct gs_texture : vk_object {
	gs_texture_type textureType;
	gs_color_format format;
	uint32_t flags;
	vk::Image image;
	vk::ImageView imageView;
	vk::DeviceMemory deviceMemory;
	gs_sampler_state *samplerState = nullptr;
	std::unique_ptr<gs_buffer> buffer;

	inline virtual void UpdateFromMapped() {}

	inline gs_texture(gs_device_t *_device, gs_texture_type _type)
		: vk_object(_device, vk_type::VK_TEXTURE), textureType(_type)
	{
	}

	inline gs_texture(gs_device_t *_device, const gs_sampler_info *_info)
		: vk_object(_device, vk_type::VK_TEXTURE)
	{
	}

	inline gs_texture(gs_texture_type _type,
			  gs_color_format _format)
		: textureType(_type), format(_format)
	{
	}

	inline gs_texture(gs_device_t *_device,
			  gs_texture_type _type, gs_color_format _format, uint32_t _flags)
		: vk_object(_device, vk_type::VK_TEXTURE), textureType(_type), format(_format), flags(_flags)
	{
	}

	~gs_texture();
};

struct gs_texture_2d : gs_texture {
	uint32_t width = 0, height = 0;

	void UpdateFromMapped() override;
	inline gs_texture_2d() : gs_texture(GS_TEXTURE_2D, GS_UNKNOWN) {}
	gs_texture_2d(gs_device_t *_device, uint32_t _width, uint32_t _height,
		      gs_color_format colorFormat, const uint8_t *const *_data,
		      uint32_t flags);
};

struct vk_image {
	vk::Image image;
	vk::ImageView imageView;
	vk::DeviceMemory deviceMemory;
};

struct gs_swap_chain : vk_object {
	std::unique_ptr<gs_init_data> initData;
	vk::Extent2D extent;
	vk::SwapchainKHR swapchainKHR;
	vk::Format format;
	vk::ColorSpaceKHR colorSpaceKHR;
	vk::PresentModeKHR presentModeKHR;
	uint32_t imageCount = 0, usedFamilyIndex = 0;
	std::vector<vk_image> colorImages, depthImages;
	std::unique_ptr<vulkan_surface> surface;

	void Recreate(uint32_t cx, uint32_t cy);
	void Recreate();

	gs_swap_chain(gs_device_t *_device, const gs_init_data *data,
		      std::unique_ptr<vulkan_surface> _surface,
		      uint32_t queueFamilyIndex);

	~gs_swap_chain();
};

struct gs_shader_param {
	std::string name;
	gs_shader_param_type type;

	int arrayCount;
	uint32_t textureID;
	gs_sampler_state *nextSampler = nullptr;

	size_t pos = 0;

	std::vector<uint8_t> curValue;
	std::vector<uint8_t> defaultValue;
	bool changed;

	gs_shader_param(shader_var &var, uint32_t &texCounter);
};

struct ShaderInputs {
	uint32_t lastOffset = 0;
	std::vector<std::string> names;
	std::vector<vk::VertexInputAttributeDescription> descs;
};

struct gs_shader : vk_object {
	std::string name, file;
	gs_shader_type type;
	vk::ShaderModule module;
	std::vector<uint32_t> spirv;
	std::vector<gs_shader_param> params;
	std::unique_ptr<gs_uniform_buffer> uniformBuffer;
	std::map<uint32_t, vk::DescriptorType> bindings;
	std::map<uint32_t, std::string> locations;
	std::string code;
	size_t constantSize = 0;

	void BuildUniformBuffer();
	void Compile(const std::string &source);
	void UpdateParam(std::vector<uint8_t> &constData,
		    gs_shader_param &param, bool &upload);
	void UploadParams();

	gs_shader(gs_device_t *device, gs_shader_type type, std::string file);
	~gs_shader();
};

struct gs_vertex_shader : gs_shader {
	ShaderInputs shaderInputs;
	uint32_t nTexUnits = 0;
	bool hasNormals = false, hasTangents = false, hasColors = false;

	gs_shader_param *world, *viewProjection;

	void GetBuffersExpected(const ShaderInputs &inputs);

	gs_vertex_shader(gs_device_t *device, const char *source, const char *file);
};

struct gs_fragment_shader : gs_shader {
	std::vector<std::unique_ptr<gs_samplerstate_t>> samplers;

	gs_fragment_shader(gs_device_t *device, const char *source,
			   const char *file);
};

struct VulkanShader : vk_object {
	std::unique_ptr<gs_vertex_shader> vertexShader = nullptr;
	std::unique_ptr<gs_fragment_shader> fragmentShader = nullptr;
	vk::PipelineLayout pipelineLayout;
	vk::Pipeline pipeline;
	vk::DescriptorSetLayout descriptorSetLayout;
	vk::DescriptorSet descriptorSet;
	std::vector<vk::DescriptorSetLayoutBinding> descSetLayoutBindings;

	matrix4 projection, view, viewProjection;

	std::vector<vk::DescriptorSetLayoutBinding> SetupBindings();
	void Recreate();

	inline VulkanShader(gs_device_t *_device) : vk_object(_device, vk_type::VK_COMBINEDSHADER) {}
	VulkanShader(gs_device_t *_device, gs_vertex_shader *_vertexShader,
		     gs_fragment_shader *_fragmentShader);
	~VulkanShader();
};

struct VulkanRenderable : vk_object {
	gs_vertex_buffer *vertexBuffer = nullptr;
	gs_index_buffer *indexBuffer = nullptr;
	std::vector<vk::DescriptorSet> descriptorSets;
	std::vector<gs_texture_t *> textures;
	VulkanShader *shader = nullptr;

	inline VulkanRenderable(gs_device_t *_device) : vk_object(_device, vk_type::VK_RENDERABLE) {}
	VulkanRenderable(gs_device_t *_device, VulkanShader *_shader, gs_vertex_buffer *_vertexBuffer,
			 gs_index_buffer *_indexBuffer);
	~VulkanRenderable();
};

struct gs_device {
	vulkan_instance *instance = nullptr;

	std::vector<matrix4> projectionStack;
	matrix4 currentProjection, currentView, currentViewProjection;

	std::string deviceName;
	uint32_t deviceID, vendorID;
	std::tuple<vk::PhysicalDevice, vk::Device> device;
	vk::PhysicalDeviceProperties deviceProperties;

	vk::Queue queue;
	uint32_t queueFamilyIndex = 0;
	vk::DescriptorPool descriptorPool;

	vk::Rect2D scissor;
	vk::Viewport viewport;
	int32_t currentSwapchain = -1;
	std::vector<std::unique_ptr<gs_swap_chain>> loadedSwapchains;

	vk::RenderPass renderPass;
	std::vector<vk::Framebuffer> framebuffers;

	vk::CommandPool commandPool, instantPool;
	std::vector<vk::CommandBuffer> commandBuffers;
	vk::CommandBuffer instantBuffer;
	vk::Fence instantFence;

	uint32_t currentFrame = 0;
	std::vector<vk::Fence> inFlightFences;
	vk::Semaphore imageAvailableSemaphore, renderFinishedSemaphore;

	std::vector<std::unique_ptr<gs_buffer>> loadedBuffers;
	int32_t currentVertexBuffer = -1, currentIndexBuffer = -1;

	std::unique_ptr<gs_shader_t> lastVertexShader = nullptr;
	std::vector<std::unique_ptr<VulkanShader>> loadedShaders;
	int32_t currentShader = -1;

	std::vector<std::unique_ptr<VulkanRenderable>> loadedRenderables;
	int32_t currentRenderable = -1, lastRenderable = -1;

	std::unique_ptr<gs_samplerstate_t> defaultSampler = nullptr;
	std::vector<std::unique_ptr<gs_texture_t>> loadedTextures;
	int32_t currentTexture = -1;

	vec4 *clearColor = nullptr;

	void CreateLogicalDevice();
	void CreateRenderPasses(vk::Format format);
	void CreateCommandPool();
	void CreateDescriptorPool();
	void CreateFramebuffers();
	void CreateCommandBuffers();
	void CreateSyncObjects();

	void Resize(uint32_t cx, uint32_t cy);

	gs_swap_chain *CreateSwapchain(const gs_init_data *data);
	inline gs_swapchain_t *GetCurrentSwapchain() {
		if (loadedSwapchains.empty() || currentSwapchain == -1)
			return nullptr;

		return loadedSwapchains[currentSwapchain].get();
	}

	inline gs_swapchain_t *GetPresentSwapchain() {
		if (loadedSwapchains.empty())
			return nullptr;

		return loadedSwapchains.front().get();
	}

	vk::DescriptorSetLayout CreateDescriptorSetLayout(
		const std::vector<vk::DescriptorSetLayoutBinding> &bindings);

	std::vector<vk::DescriptorSet> CreateDescriptorSets(VulkanShader *shader);

	vk::PipelineLayout
	CreatePipelineLayout(vk::DescriptorSetLayout descriptorSetLayout);

	vk::ShaderModule CreateShaderModule(gs_shader_t *shader);
	vk::Pipeline CreateGraphicsPipeline(gs_vertex_shader *vertShader, gs_fragment_shader *fragShader, vk::PipelineLayout pipelineLayout);
	std::tuple<vk::Image, vk::DeviceMemory> CreateImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties);
	vk::ImageView CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);

	void BeginCommandBuffer();
	void EndCommandBuffer();

	void GarbageCollect();
	gs_shader_t *SubmitShader(std::unique_ptr<gs_shader_t> shader);
	void SetShader(gs_shader_t *shader);
	gs_buffer *SubmitBuffer(std::unique_ptr<gs_buffer> buffer);
	void SetBuffer(gs_buffer *buffer);
	gs_texture_t *SubmitTexture(std::unique_ptr<gs_texture_t> texture);
	void SetTexture(gs_texture_t *texture);
	void SubmitRenderable(VulkanShader *shader, gs_vertex_buffer *vertexBuffer,
			      gs_index_buffer *indexBuffer);
	void SetClearColor(const struct vec4 *color);
	void UpdateDraw(uint32_t startVert, uint32_t nVert);
	void RecreateCommandBuffers();

	vk::Format FindSupportedFormat(const std::vector<vk::Format> &formats,
				       vk::ImageTiling tiling,
				       vk::FormatFeatureFlags featureFlags);

	std::tuple<int32_t, vk_type> GetLoadedBufferIdx(gs_buffer *buffer);
	std::tuple<int32_t, vk_type> GetLoadedShaderIdx(gs_shader_t *shader);
	int32_t GetLoadedRenderableIdx(vk_object *buffer);
	int32_t GetLoadedTextureIdx(gs_texture_t *texture);
	int32_t GetLoadedSwapchainIdx(const gs_init_data *data);

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
	vk::DebugUtilsMessengerEXT debugMessenger;

	vulkan_instance(std::vector<const char *> requestedLayers,
			std::vector<const char *> requestedExtensions);

	~vulkan_instance();
};
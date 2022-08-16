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

/* General Vulkan instance */
inline vulkan_instance *sharedInstance = nullptr;

static inline void LogVulkanDevices()
{
	const auto instance = sharedInstance->vulkanInstance;

	blog(LOG_INFO, "Available Vulkan Devices: ");

	uint32_t i = 0;
	for (const auto physicalDevice : instance.enumeratePhysicalDevices()) {
		const auto properties = physicalDevice.getProperties();
		const auto memoryProperties =
			physicalDevice.getMemoryProperties();

		blog(LOG_INFO, "\tDevice %u: %s", i, properties.deviceName);

		/* device dedicated and shared vram */
		for (uint32_t j = 0; j < memoryProperties.memoryHeapCount;
		     j++) {
			const auto memoryHeap =
				memoryProperties.memoryHeaps[j];

			if (memoryHeap.flags &
			    vk::MemoryHeapFlagBits::eDeviceLocal) {
				blog(LOG_INFO, "\t  Dedicated VRAM: %u",
				     memoryHeap.size);
			} else if (memoryHeap.size > 0) {
				blog(LOG_INFO, "\t  Shared VRAM: %u",
				     memoryHeap.size);
			}
		}

		/* device vulkan version */
		blog(LOG_INFO, "\t  Vulkan Version: %u.%u.%u",
		     properties.apiVersion >> 22,
		     (properties.apiVersion >> 12) & 0x3ff,
		     properties.apiVersion & 0xfff);

		/* device information */
		blog(LOG_INFO, "\t  VendorID: %u", properties.vendorID);
		blog(LOG_INFO, "\t  Vendor: %s",
		     GetVulkanVendor(properties.vendorID));

		blog(LOG_INFO, "\t  DeviceID: %u", properties.deviceID);
		blog(LOG_INFO, "\t  Device Type: %s",
		     GetVulkanDeviceType(properties.deviceType));

		/* driver version */
		blog(LOG_INFO, "\t  Driver Version: %s",
		     GetVulkanDriverVersion(properties.driverVersion,
					    properties.vendorID));

		i++;
	}
}

static inline vk::PhysicalDevice GetVulkanDevice(uint32_t adapter)
{
	const auto instance = sharedInstance->vulkanInstance;
	const auto physicalDevices = instance.enumeratePhysicalDevices();
	if (adapter >= physicalDevices.size())
		throw std::runtime_error("Invalid adapter index");

	return physicalDevices[adapter];
}

static inline int InitializeInstance()
{
	blog(LOG_INFO, "---------------------------------");
	blog(LOG_INFO, "Initializing Vulkan...");
	try {
		sharedInstance = new vulkan_instance(
			{
#ifdef _DEBUG
				"VK_LAYER_KHRONOS_validation"
#endif
			},
			{
#ifdef VK_USE_PLATFORM_WIN32_KHR
				VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
				VK_KHR_SURFACE_EXTENSION_NAME});
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "InitializeInstance: %s", e.what());
		return GS_ERROR_FAIL;
	}

	LogVulkanDevices();

	return GS_SUCCESS;
}

const char *device_get_name(void)
{
	return "Vulkan";
}

int device_get_type(void)
{
	return GS_DEVICE_VULKAN;
}

const char *device_preprocessor_name(void)
{
	return "_VULKAN";
}

int device_create(gs_device_t **p_device, uint32_t adapter)
{
	int result = GS_SUCCESS;
	if (sharedInstance == nullptr &&
	    (result = InitializeInstance()) != GS_SUCCESS)
		return result;

	try {
		sharedInstance->devices.emplace_back(
			std::make_unique<gs_device>(sharedInstance,
						    GetVulkanDevice(adapter)));
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_create (Vulkan): %s", e.what());
		return GS_ERROR_FAIL;
	}

	*p_device = sharedInstance->devices.back().get();
	return result;
}

void device_destroy(gs_device_t *device)
{
	delete device;
}

void *device_get_device_obj(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return nullptr;
}

gs_swapchain_t *device_swapchain_create(gs_device_t *device,
					const struct gs_init_data *data)
{
	gs_swap_chain *swapchain = nullptr;

	try {
		swapchain =
			static_cast<gs_device *>(device)->CreateSwapchain(data);
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_swapchain_create (Vulkan): %s",
		     e.what());
		return nullptr;
	}

	return swapchain;
}

static void device_resize_internal(gs_device_t *device, uint32_t cx,
				   uint32_t cy, gs_color_space space)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
	UNUSED_PARAMETER(space);
}

void device_resize(gs_device_t *device, uint32_t cx, uint32_t cy)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
}

enum gs_color_space device_get_color_space(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return GS_CS_SRGB;
}

void device_update_color_space(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_get_size(const gs_device_t *device, uint32_t *cx, uint32_t *cy)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
}

uint32_t device_get_width(const gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return 0;
}

uint32_t device_get_height(const gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return 0;
}

gs_texture_t *device_texture_create(gs_device_t *device, uint32_t width,
				    uint32_t height,
				    enum gs_color_format color_format,
				    uint32_t levels, const uint8_t **data,
				    uint32_t flags)
{
	gs_texture *texture = nullptr;

	try {
		texture = new gs_texture_2d(device, width, height, color_format,
					    data, flags);
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_texture_create (Vulkan): %s", e.what());
	}

	return texture;
}

gs_texture_t *device_cubetexture_create(gs_device_t *device, uint32_t size,
					enum gs_color_format color_format,
					uint32_t levels, const uint8_t **data,
					uint32_t flags)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(size);
	UNUSED_PARAMETER(color_format);
	UNUSED_PARAMETER(levels);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(flags);
	return nullptr;
}

gs_texture_t *device_voltexture_create(gs_device_t *device, uint32_t width,
				       uint32_t height, uint32_t depth,
				       enum gs_color_format color_format,
				       uint32_t levels,
				       const uint8_t *const *data,
				       uint32_t flags)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);
	UNUSED_PARAMETER(depth);
	UNUSED_PARAMETER(color_format);
	UNUSED_PARAMETER(levels);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(flags);
	return nullptr;
}

gs_zstencil_t *device_zstencil_create(gs_device_t *device, uint32_t width,
				      uint32_t height,
				      enum gs_zstencil_format format)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);
	UNUSED_PARAMETER(format);
	return nullptr;
}

gs_samplerstate_t *
device_samplerstate_create(gs_device_t *device,
			   const struct gs_sampler_info *info)
{
	gs_sampler_state *ss = nullptr;

	try {
		ss = new gs_sampler_state(device, info);
	} catch (const std::runtime_error &err) {
		blog(LOG_ERROR, "device_samplerstate_create (Vulkan): %s ",
		     err.what());
	}

	return ss;
}

gs_timer_t *device_timer_create(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return nullptr;
}

gs_timer_range_t *device_timer_range_create(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return nullptr;
}

enum gs_texture_type device_get_texture_type(const gs_texture_t *texture)
{
	UNUSED_PARAMETER(texture);
	return GS_TEXTURE_2D;
}

void device_load_vertexbuffer(gs_device_t *device, gs_vertbuffer_t *vertbuffer)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(vertbuffer);
}

void device_load_indexbuffer(gs_device_t *device, gs_indexbuffer_t *indexbuffer)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(indexbuffer);
}

void device_load_texture(gs_device_t *device, gs_texture_t *tex, int unit)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(tex);
	UNUSED_PARAMETER(unit);
}

void device_load_texture_srgb(gs_device_t *device, gs_texture_t *tex, int unit)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(tex);
	UNUSED_PARAMETER(unit);
}

void device_load_samplerstate(gs_device_t *device,
			      gs_samplerstate_t *samplerstate, int unit)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(samplerstate);
	UNUSED_PARAMETER(unit);
}

void device_load_vertexshader(gs_device_t *device, gs_shader_t *vertshader)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(vertshader);
}

void device_load_pixelshader(gs_device_t *device, gs_shader_t *pixelshader)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(pixelshader);
}

void device_load_default_samplerstate(gs_device_t *device, bool b_3d, int unit)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(b_3d);
	UNUSED_PARAMETER(unit);
}

gs_shader_t *device_get_vertex_shader(const gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return nullptr;
}

gs_shader_t *device_get_pixel_shader(const gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return nullptr;
}

gs_texture_t *device_get_render_target(const gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return nullptr;
}

gs_zstencil_t *device_get_zstencil_target(const gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return nullptr;
}

static void device_set_render_target_internal(gs_device_t *device,
					      gs_texture_t *tex,
					      gs_zstencil_t *zstencil,
					      enum gs_color_space space)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(tex);
	UNUSED_PARAMETER(zstencil);
	UNUSED_PARAMETER(space);
}

void device_set_render_target(gs_device_t *device, gs_texture_t *tex,
			      gs_zstencil_t *zstencil)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(tex);
	UNUSED_PARAMETER(zstencil);
}

void device_set_render_target_with_color_space(gs_device_t *device,
					       gs_texture_t *tex,
					       gs_zstencil_t *zstencil,
					       enum gs_color_space space)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(tex);
	UNUSED_PARAMETER(zstencil);
	UNUSED_PARAMETER(space);
}

void device_set_cube_render_target(gs_device_t *device, gs_texture_t *tex,
				   int side, gs_zstencil_t *zstencil)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(tex);
	UNUSED_PARAMETER(side);
	UNUSED_PARAMETER(zstencil);
}

void device_enable_framebuffer_srgb(gs_device_t *device, bool enable)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

bool device_framebuffer_srgb_enabled(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return false;
}

void device_copy_texture_region(gs_device_t *device, gs_texture_t *dst,
				uint32_t dst_x, uint32_t dst_y,
				gs_texture_t *src, uint32_t src_x,
				uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(dst);
	UNUSED_PARAMETER(dst_x);
	UNUSED_PARAMETER(dst_y);
	UNUSED_PARAMETER(src);
	UNUSED_PARAMETER(src_x);
	UNUSED_PARAMETER(src_y);
	UNUSED_PARAMETER(src_w);
	UNUSED_PARAMETER(src_h);
}

void device_copy_texture(gs_device_t *device, gs_texture_t *dst,
			 gs_texture_t *src)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(dst);
	UNUSED_PARAMETER(src);
}

void device_stage_texture(gs_device_t *device, gs_stagesurf_t *dst,
			  gs_texture_t *src)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(dst);
	UNUSED_PARAMETER(src);
}

void device_enter_context(gs_device_t *device)
{
	/* unused */
	UNUSED_PARAMETER(device);
}

void device_leave_context(gs_device_t *device)
{
	/* unused */
	UNUSED_PARAMETER(device);
}

void device_begin_frame(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_begin_scene(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_clear(gs_device_t *device, uint32_t clear_flags,
		  const struct vec4 *color, float depth, uint8_t stencil)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(clear_flags);
	UNUSED_PARAMETER(color);
	UNUSED_PARAMETER(depth);
}

void device_draw(gs_device_t *device, enum gs_draw_mode draw_mode,
		 uint32_t start_vert, uint32_t num_verts)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(draw_mode);
	UNUSED_PARAMETER(start_vert);
	UNUSED_PARAMETER(num_verts);
}

bool device_is_present_ready(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return true;
}

void device_present(gs_device_t *device)
{
	if (!device->currentSwapchain)
		return;

	const auto logicalDevice = device->GetLogicalDevice();

	auto result = logicalDevice.waitForFences(
		1, &device->inFlightFences[device->currentFrame], true,
		std::numeric_limits<uint64_t>::max());

	if (result != vk::Result::eSuccess) {
		blog(LOG_ERROR, "waitForFences failed: %s",
		     vk::to_string(result).c_str());

		return;
	}

	result = logicalDevice.acquireNextImageKHR(
		device->currentSwapchain->swapchainKHR,
		std::numeric_limits<uint64_t>::max(),
		device->imageAvailableSemaphore, nullptr,
		&device->currentFrame);

	if (result == vk::Result::eErrorOutOfDateKHR ||
	    result == vk::Result::eSuboptimalKHR) {
		// TODO: Recreate swapchains
		return;
	} else if (result != vk::Result::eSuccess) {
		blog(LOG_ERROR, "%s: acquireNextImageKHR failed: %s",
		     device->deviceName.c_str(), vk::to_string(result).c_str());
		return;
	}

	vk::PipelineStageFlags waitStage =
		vk::PipelineStageFlagBits::eColorAttachmentOutput;

	const vk::SubmitInfo submitInfo(
		1, &device->imageAvailableSemaphore, &waitStage, 1,
		&device->commandBuffers[device->currentFrame], 1,
		&device->renderFinishedSemaphore);

	result = logicalDevice.resetFences(
		1, &device->inFlightFences[device->currentFrame]);

	if (result != vk::Result::eSuccess) {
		blog(LOG_ERROR, "%s: Failed to reset fence",
		     device->deviceName.c_str());
		return;
	}

	if (device->queue.submit(1, &submitInfo,
				 device->inFlightFences[device->currentFrame]) !=
	    vk::Result::eSuccess) {
		blog(LOG_ERROR, "%s: Failed to submit draw command buffer",
		     device->deviceName.c_str());
		return;
	}

	const vk::PresentInfoKHR presentInfo(
		1, &device->renderFinishedSemaphore, 1,
		&device->currentSwapchain->swapchainKHR, &device->currentFrame);

	try {
		const auto result = device->queue.presentKHR(presentInfo);
		if (result == vk::Result::eErrorOutOfDateKHR ||
		    result == vk::Result::eSuboptimalKHR) {
			// TODO: Recreate swapchains
			return;
		}
	} catch (const vk::Error &e) {
		blog(LOG_ERROR, "%s: Failed to present swapchain: %s",
		     device->deviceName.c_str(), e.what());
	}

	device->currentFrame = (device->currentFrame + 1) %
			       device->currentSwapchain->imageCount;
}

void device_end_scene(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swapchain)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(swapchain);
}

void device_flush(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_set_cull_mode(gs_device_t *device, enum gs_cull_mode mode)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(mode);
}

enum gs_cull_mode device_get_cull_mode(const gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return GS_NEITHER;
}

void device_enable_blending(gs_device_t *device, bool enable)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

void device_enable_depth_test(gs_device_t *device, bool enable)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

void device_enable_stencil_test(gs_device_t *device, bool enable)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

void device_enable_stencil_write(gs_device_t *device, bool enable)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(enable);
}

void device_enable_color(gs_device_t *device, bool red, bool green, bool blue,
			 bool alpha)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(red);
	UNUSED_PARAMETER(green);
	UNUSED_PARAMETER(blue);
	UNUSED_PARAMETER(alpha);
}

void device_blend_function(gs_device_t *device, enum gs_blend_type src,
			   enum gs_blend_type dest)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(src);
	UNUSED_PARAMETER(dest);
}

void device_blend_function_separate(gs_device_t *device,
				    enum gs_blend_type src_c,
				    enum gs_blend_type dest_c,
				    enum gs_blend_type src_a,
				    enum gs_blend_type dest_a)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(src_c);
	UNUSED_PARAMETER(dest_c);
	UNUSED_PARAMETER(src_a);
	UNUSED_PARAMETER(dest_a);
}

void device_blend_op(gs_device_t *device, enum gs_blend_op_type op)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(op);
}

void device_depth_function(gs_device_t *device, enum gs_depth_test test)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(test);
}

void device_stencil_function(gs_device_t *device, enum gs_stencil_side side,
			     enum gs_depth_test test)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(side);
	UNUSED_PARAMETER(test);
}

void device_stencil_op(gs_device_t *device, enum gs_stencil_side side,
		       enum gs_stencil_op_type fail,
		       enum gs_stencil_op_type zfail,
		       enum gs_stencil_op_type zpass)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(side);
	UNUSED_PARAMETER(fail);
	UNUSED_PARAMETER(zfail);
	UNUSED_PARAMETER(zpass);
}

void device_set_viewport(gs_device_t *device, int x, int y, int width,
			 int height)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(x);
	UNUSED_PARAMETER(y);
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);
}

void device_get_viewport(const gs_device_t *device, struct gs_rect *rect)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(rect);
}

void device_set_scissor_rect(gs_device_t *device, const struct gs_rect *rect)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(rect);
}

void device_ortho(gs_device_t *device, float left, float right, float top,
		  float bottom, float zNear, float zFar)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(left);
	UNUSED_PARAMETER(right);
	UNUSED_PARAMETER(top);
	UNUSED_PARAMETER(bottom);
	UNUSED_PARAMETER(zNear);
	UNUSED_PARAMETER(zFar);
}

void device_frustum(gs_device_t *device, float left, float right, float top,
		    float bottom, float zNear, float zFar)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(left);
	UNUSED_PARAMETER(right);
	UNUSED_PARAMETER(top);
	UNUSED_PARAMETER(bottom);
	UNUSED_PARAMETER(zNear);
	UNUSED_PARAMETER(zFar);
}

void device_projection_push(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_projection_pop(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void gs_swapchain_destroy(gs_swapchain_t *swapchain)
{
	UNUSED_PARAMETER(swapchain);
}

void gs_texture_destroy(gs_texture_t *tex)
{
	UNUSED_PARAMETER(tex);
}

uint32_t gs_texture_get_width(const gs_texture_t *tex)
{
	UNUSED_PARAMETER(tex);
	return 0;
}

uint32_t gs_texture_get_height(const gs_texture_t *tex)
{
	UNUSED_PARAMETER(tex);
	return 0;
}

enum gs_color_format gs_texture_get_color_format(const gs_texture_t *tex)
{
	UNUSED_PARAMETER(tex);
	return GS_UNKNOWN;
}

bool gs_texture_map(gs_texture_t *tex, uint8_t **ptr, uint32_t *linesize)
{
	UNUSED_PARAMETER(tex);
	UNUSED_PARAMETER(ptr);
	UNUSED_PARAMETER(linesize);
	return false;
}

void gs_texture_unmap(gs_texture_t *tex)
{
	UNUSED_PARAMETER(tex);
}

void *gs_texture_get_obj(gs_texture_t *tex)
{
	UNUSED_PARAMETER(tex);
	return nullptr;
}

void gs_cubetexture_destroy(gs_texture_t *cubetex)
{
	UNUSED_PARAMETER(cubetex);
}

uint32_t gs_cubetexture_get_size(const gs_texture_t *cubetex)
{
	UNUSED_PARAMETER(cubetex);
	return 0;
}

enum gs_color_format
gs_cubetexture_get_color_format(const gs_texture_t *cubetex)
{
	UNUSED_PARAMETER(cubetex);
	return GS_UNKNOWN;
}

void gs_voltexture_destroy(gs_texture_t *voltex)
{
	UNUSED_PARAMETER(voltex);
}

uint32_t gs_voltexture_get_width(const gs_texture_t *voltex)
{
	UNUSED_PARAMETER(voltex);
	return 0;
}

uint32_t gs_voltexture_get_height(const gs_texture_t *voltex)
{
	UNUSED_PARAMETER(voltex);
	return 0;
}

uint32_t gs_voltexture_get_depth(const gs_texture_t *voltex)
{
	UNUSED_PARAMETER(voltex);
	return 0;
}

enum gs_color_format gs_voltexture_get_color_format(const gs_texture_t *voltex)
{
	UNUSED_PARAMETER(voltex);
	return GS_UNKNOWN;
}

void gs_stagesurface_destroy(gs_stagesurf_t *stagesurf)
{
	UNUSED_PARAMETER(stagesurf);
}

uint32_t gs_stagesurface_get_width(const gs_stagesurf_t *stagesurf)
{
	UNUSED_PARAMETER(stagesurf);
	return 0;
}

uint32_t gs_stagesurface_get_height(const gs_stagesurf_t *stagesurf)
{
	UNUSED_PARAMETER(stagesurf);
	return 0;
}

enum gs_color_format
gs_stagesurface_get_color_format(const gs_stagesurf_t *stagesurf)
{
	UNUSED_PARAMETER(stagesurf);
	return GS_UNKNOWN;
}

bool gs_stagesurface_map(gs_stagesurf_t *stagesurf, uint8_t **data,
			 uint32_t *linesize)
{
	UNUSED_PARAMETER(stagesurf);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(linesize);
	return false;
}

void gs_stagesurface_unmap(gs_stagesurf_t *stagesurf)
{
	UNUSED_PARAMETER(stagesurf);
}

void gs_zstencil_destroy(gs_zstencil_t *zstencil)
{
	UNUSED_PARAMETER(zstencil);
}

void gs_samplerstate_destroy(gs_samplerstate_t *samplerstate)
{
	UNUSED_PARAMETER(samplerstate);
}

void gs_vertexbuffer_destroy(gs_vertbuffer_t *vertbuffer)
{
	UNUSED_PARAMETER(vertbuffer);
}

static inline void gs_vertexbuffer_flush_internal(gs_vertbuffer_t *vertbuffer,
						  const gs_vb_data *data)
{
	UNUSED_PARAMETER(vertbuffer);
	UNUSED_PARAMETER(data);
}

void gs_vertexbuffer_flush(gs_vertbuffer_t *vertbuffer)
{
	UNUSED_PARAMETER(vertbuffer);
}

void gs_vertexbuffer_flush_direct(gs_vertbuffer_t *vertbuffer,
				  const gs_vb_data *data)
{
	UNUSED_PARAMETER(vertbuffer);
	UNUSED_PARAMETER(data);
}

void gs_indexbuffer_destroy(gs_indexbuffer_t *indexbuffer)
{
	UNUSED_PARAMETER(indexbuffer);
}

static inline void gs_indexbuffer_flush_internal(gs_indexbuffer_t *indexbuffer,
						 const void *data)
{
	UNUSED_PARAMETER(indexbuffer);
	UNUSED_PARAMETER(data);
}

void gs_indexbuffer_flush(gs_indexbuffer_t *indexbuffer)
{
	UNUSED_PARAMETER(indexbuffer);
}

void gs_indexbuffer_flush_direct(gs_indexbuffer_t *indexbuffer,
				 const void *data)
{
	UNUSED_PARAMETER(indexbuffer);
	UNUSED_PARAMETER(data);
}

size_t gs_indexbuffer_get_num_indices(const gs_indexbuffer_t *indexbuffer)
{
	UNUSED_PARAMETER(indexbuffer);
	return 0;
}

enum gs_index_type gs_indexbuffer_get_type(const gs_indexbuffer_t *indexbuffer)
{
	UNUSED_PARAMETER(indexbuffer);
	return GS_UNSIGNED_SHORT;
}

void gs_timer_destroy(gs_timer_t *timer)
{
	UNUSED_PARAMETER(timer);
}

void gs_timer_begin(gs_timer_t *timer)
{
	UNUSED_PARAMETER(timer);
}

void gs_timer_end(gs_timer_t *timer)
{
	UNUSED_PARAMETER(timer);
}

bool gs_timer_get_data(gs_timer_t *timer, uint64_t *ticks)
{
	UNUSED_PARAMETER(timer);
	UNUSED_PARAMETER(ticks);
	return false;
}

void gs_timer_range_destroy(gs_timer_range_t *range)
{
	UNUSED_PARAMETER(range);
}

void gs_timer_range_begin(gs_timer_range_t *range)
{
	UNUSED_PARAMETER(range);
}

void gs_timer_range_end(gs_timer_range_t *range)
{
	UNUSED_PARAMETER(range);
}

bool gs_timer_range_get_data(gs_timer_range_t *range, bool *disjoint,
			     uint64_t *frequency)
{
	UNUSED_PARAMETER(range);
	UNUSED_PARAMETER(disjoint);
	UNUSED_PARAMETER(frequency);
	return false;
}

extern "C" EXPORT bool device_is_monitor_hdr(gs_device_t *device, void *monitor)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(monitor);
	return false;
}

extern "C" EXPORT void device_debug_marker_begin(gs_device_t *,
						 const char *markername,
						 const float color[4])
{
	/* TODO: Implement */
}

extern "C" EXPORT void device_debug_marker_end(gs_device_t *)
{
	/* TODO: Implement */
}

extern "C" EXPORT bool device_gdi_texture_available(void)
{
	return false;
}

extern "C" EXPORT bool device_shared_texture_available(void)
{
	return false;
}
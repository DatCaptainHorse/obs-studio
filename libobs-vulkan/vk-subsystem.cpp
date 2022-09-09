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

/* General Vulkan instance */
inline std::unique_ptr<vulkan_instance> sharedInstance = nullptr;

static inline void LogVulkanDevices()
{
	const auto instance = sharedInstance->vulkanInstance;

	blog(LOG_INFO, "Available Vulkan Devices: ");

	uint32_t i = 0;
	for (const auto physicalDevice : instance.enumeratePhysicalDevices()) {
		vk::PhysicalDeviceProperties2 properties;
		vk::PhysicalDeviceDriverProperties driverProperties;
		properties.pNext = &driverProperties;
		physicalDevice.getProperties2(&properties);
		const auto memoryProperties =
			physicalDevice.getMemoryProperties();

		blog(LOG_INFO, "\tDevice %u: %s", i,
		     properties.properties.deviceName);

		/* device dedicated and shared vram */
		for (uint32_t j = 0; j < memoryProperties.memoryHeapCount;
		     j++) {
			const auto memoryHeap = memoryProperties.memoryHeaps[j];

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
		     properties.properties.apiVersion >> 22,
		     (properties.properties.apiVersion >> 12) & 0x3ff,
		     properties.properties.apiVersion & 0xfff);

		/* device information */
		blog(LOG_INFO, "\t  VendorID: %u",
		     properties.properties.vendorID);
		blog(LOG_INFO, "\t  Vendor: %s",
		     GetVulkanVendor(properties.properties.vendorID));

		blog(LOG_INFO, "\t  DeviceID: %u",
		     properties.properties.deviceID);
		blog(LOG_INFO, "\t  Device Type: %s",
		     GetVulkanDeviceType(properties.properties.deviceType));

		/* driver information */
		blog(LOG_INFO, "\t  DriverID: %u",
		     GetVulkanDriverID(driverProperties.driverID));

		blog(LOG_INFO, "\t  Driver: %s", driverProperties.driverName);
		blog(LOG_INFO, "\t  Driver Version: %s",
		     driverProperties.driverInfo);

		const auto conformance = driverProperties.conformanceVersion;
		blog(LOG_INFO, "\t  Driver Conformance: %u.%u.%u.%u",
		     conformance.major, conformance.minor, conformance.subminor,
		     conformance.patch);

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
		/* make_unique doesn't support initializer lists,
		 * so doing it this way */
		sharedInstance =
			std::unique_ptr<vulkan_instance>(new vulkan_instance(
				{
#ifdef _DEBUG
					"VK_LAYER_KHRONOS_validation"
#endif
				},
				{
#ifdef VK_USE_PLATFORM_WIN32_KHR
					VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef _DEBUG
					VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
					VK_KHR_SURFACE_EXTENSION_NAME}));
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
			std::make_unique<gs_device>(sharedInstance.get(),
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
	device->GetLogicalDevice().waitIdle();
	for (auto it = sharedInstance->devices.begin();
	     it != sharedInstance->devices.end(); ++it) {
		if (it->get() == device) {
			sharedInstance->devices.erase(it);
			return;
		}
	}
}

void *device_get_device_obj(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
	return nullptr;
}

void device_resize(gs_device_t *device, uint32_t cx, uint32_t cy)
{
	try {
		device->Resize(cx, cy);
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "%s: device_resize (Vulkan): %s", device->deviceName.c_str(), e.what());
	}
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

void device_load_texture_srgb(gs_device_t *device, gs_texture_t *tex, int unit)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(tex);
	UNUSED_PARAMETER(unit);
}

void device_load_default_samplerstate(gs_device_t *device, bool b_3d, int unit)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(b_3d);
	UNUSED_PARAMETER(unit);
}

gs_shader_t *device_get_vertex_shader(const gs_device_t *device)
{
	return device->loadedShaders[device->currentShader]->vertexShader.get();
}

gs_shader_t *device_get_pixel_shader(const gs_device_t *device)
{
	return device->loadedShaders[device->currentShader]->fragmentShader.get();
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
	if (!src)
		return;
	if (!dst)
		return;
	if (src->textureType != GS_TEXTURE_2D || dst->textureType != GS_TEXTURE_2D)
		return;
	if (dst->format != src->format)
		return;

	auto src2d = static_cast<gs_texture_2d *>(src);
	auto dst2d = static_cast<gs_texture_2d *>(dst);


	auto copyWidth = src_w ? src_w : (src2d->width - src_x);
	auto copyHeight = src_h ? src_h : (src2d->height - src_y);

	auto dstWidth = dst2d->width - dst_x;
	auto dstHeight = dst2d->height - dst_y;

	if (dstWidth < copyWidth || dstHeight < copyHeight)
		return;

	if (dst_x == 0 && dst_y == 0 && src_x == 0 && src_y == 0 &&
	    src_w == 0 && src_h == 0) {
		copyWidth = 0;
		copyHeight = 0;
	}

	vk_copyImagetoImage(device, dst->image, src->image, copyWidth, copyHeight);
}

void device_copy_texture(gs_device_t *device, gs_texture_t *dst,
			 gs_texture_t *src)
{
	device_copy_texture_region(device, dst, 0, 0, src, 0, 0, 0, 0);
}

void device_stage_texture(gs_device_t *device, gs_stagesurf_t *dst,
			  gs_texture_t *src)
{
	if (!src)
		return;
	if (!dst)
		return;
	if (src->textureType != GS_TEXTURE_2D)
		return;
	if (dst->format != src->format)
		return;

	auto src2d = static_cast<gs_texture_2d *>(src);

	vk_copyImageToBuffer(device, src2d->image, dst->packBuffer->buffer, src2d->width, src2d->height);
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
	UNUSED_PARAMETER(depth);
	UNUSED_PARAMETER(stencil);
	if (clear_flags & GS_CLEAR_COLOR)
		device->SetClearColor(color);
}

void device_draw(gs_device_t *device, enum gs_draw_mode draw_mode,
		 uint32_t start_vert, uint32_t num_verts)
{
	device->UpdateDraw(start_vert, num_verts);
}

bool device_is_present_ready(gs_device_t *device)
{
	const auto result = device->GetLogicalDevice().waitForFences(
		1, &device->inFlightFences[device->currentFrame], true, 0);

	if (result == vk::Result::eSuccess)
		return true;
	else if (result == vk::Result::eTimeout)
		return false;
	else {
		blog(LOG_ERROR, "vk::Device::waitForFences failed: %s",
		     vk::to_string(result).c_str());

		return false;
	}
}

void device_present(gs_device_t *device)
{
	if (device->currentSwapchain == -1)
		return;

	const auto logicalDevice = device->GetLogicalDevice();

	auto result = logicalDevice.acquireNextImageKHR(
		device->GetPresentSwapchain()->swapchainKHR,
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
		&device->GetPresentSwapchain()->swapchainKHR, &device->currentFrame);

	try {
		result = device->queue.presentKHR(presentInfo);
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
			       device->GetPresentSwapchain()->imageCount;
}

void device_end_scene(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
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
	if (width <= 0 || height <= 0)
		return;

	device->viewport = vk::Viewport(static_cast<float>(x),
					static_cast<float>(y),
					static_cast<float>(width),
					static_cast<float>(height), 0.0f, 1.0f);
}

void device_get_viewport(const gs_device_t *device, struct gs_rect *rect)
{
	rect->x = static_cast<int>(device->viewport.x);
	rect->y = static_cast<int>(device->viewport.y);
	rect->cx = static_cast<int>(device->viewport.width);
	rect->cy = static_cast<int>(device->viewport.height);
}

void device_set_scissor_rect(gs_device_t *device, const struct gs_rect *rect)
{
	if (rect->cx <= 0 || rect->cy <= 0)
		return;

	device->scissor = vk::Rect2D({rect->x, rect->y},
				     {static_cast<uint32_t>(rect->cx),
				      static_cast<uint32_t>(rect->cy)});
}

void device_ortho(gs_device_t *device, float left, float right, float top,
		  float bottom, float near, float far)
{
	matrix4 *dst = &device->currentProjection;

	float rml = right - left;
	float bmt = bottom - top;
	float fmn = far - near;

	vec4_zero(&dst->x);
	vec4_zero(&dst->y);
	vec4_zero(&dst->z);
	vec4_zero(&dst->t);

	dst->x.x = 2.0f / rml;
	dst->t.x = (left + right) / -rml;

	dst->y.y = 2.0f / -bmt;
	dst->t.y = (bottom + top) / bmt;

	dst->z.z = -2.0f / fmn;
	dst->t.z = (far + near) / -fmn;

	dst->t.w = 1.0f;
}

void device_frustum(gs_device_t *device, float left, float right, float top,
		    float bottom, float near, float far)
{
	matrix4 *dst = &device->currentProjection;

	float rml = right - left;
	float tmb = top - bottom;
	float nmf = near - far;
	float nearx2 = 2.0f * near;

	vec4_zero(&dst->x);
	vec4_zero(&dst->y);
	vec4_zero(&dst->z);
	vec4_zero(&dst->t);

	dst->x.x = nearx2 / rml;
	dst->z.x = (left + right) / rml;

	dst->y.y = nearx2 / tmb;
	dst->z.y = (bottom + top) / tmb;

	dst->z.z = (far + near) / nmf;
	dst->t.z = 2.0f * (near * far) / nmf;

	dst->z.w = -1.0f;
}

void device_projection_push(gs_device_t *device)
{
	matrix4 mat;
	memcpy(&mat, &device->currentProjection, sizeof(matrix4));
	device->projectionStack.emplace_back(mat);
}

void device_projection_pop(gs_device_t *device)
{
	if (device->projectionStack.empty())
		return;

	const matrix4 &mat = device->projectionStack.back();
	memcpy(&device->currentProjection, &mat, sizeof(matrix4));
	device->projectionStack.pop_back();
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

void gs_zstencil_destroy(gs_zstencil_t *zstencil)
{
	UNUSED_PARAMETER(zstencil);
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
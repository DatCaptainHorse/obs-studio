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

gs_stage_surface::gs_stage_surface(gs_device_t *_device, gs_color_format _format,
				   uint32_t _width, uint32_t _height)
	: device(_device), format(_format), width(_width), height(_height), bytes_per_pixel(gs_get_format_bpp(_format) / 8)
{
	vk::DeviceSize size = width * bytes_per_pixel;
	size = (size + 3) & 0xFFFFFFFC;
	size *= height;

	packBuffer = std::make_unique<gs_buffer>(device, vk_type::VK_TEXTUREBUFFER);
	packBuffer->CreateBuffer(size, vk::BufferUsageFlagBits::eTransferSrc,
				 vk::MemoryPropertyFlagBits::eHostVisible |
					 vk::MemoryPropertyFlagBits::eHostCoherent);

	packBuffer->Map();
	memset(packBuffer->mapped, 0, size);
}

gs_stage_surface::~gs_stage_surface()
{
	packBuffer.reset();
}

gs_stagesurf_t *device_stagesurface_create(gs_device_t *device, uint32_t width,
					   uint32_t height,
					   enum gs_color_format color_format)
{
	gs_stage_surface *stageSurf = nullptr;

	try {
		stageSurf = new gs_stage_surface(device, color_format, width, height);
	} catch (const std::bad_alloc &) {
		blog(LOG_ERROR, "device_stagesurface_create (Vulkan) failed");
	}

	return stageSurf;
}

void gs_stagesurface_destroy(gs_stagesurf_t *stagesurf)
{
	delete stagesurf;
}

uint32_t gs_stagesurface_get_width(const gs_stagesurf_t *stagesurf)
{
	if (stagesurf)
		return stagesurf->width;

	return 0;
}

uint32_t gs_stagesurface_get_height(const gs_stagesurf_t *stagesurf)
{
	if (stagesurf)
		return stagesurf->height;

	return 0;
}

enum gs_color_format
gs_stagesurface_get_color_format(const gs_stagesurf_t *stagesurf)
{
	if (stagesurf)
		return stagesurf->format;

	return GS_UNKNOWN;
}

bool gs_stagesurface_map(gs_stagesurf_t *stagesurf, uint8_t **data,
			 uint32_t *linesize)
{
	if (!stagesurf)
		return false;

	auto surf = static_cast<gs_stage_surface *>(stagesurf);

	*data = static_cast<uint8_t *>(surf->packBuffer->mapped);

	*linesize = surf->width * gs_get_format_bpp(surf->format) / 8;
	*linesize = (*linesize + 3) & 0xFFFFFFFC;

	return true;
}

void gs_stagesurface_unmap(gs_stagesurf_t *stagesurf)
{
	// No need to unmap for Vulkan
	UNUSED_PARAMETER(stagesurf);
}
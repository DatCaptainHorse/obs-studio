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

	packBuffer = std::make_unique<gs_buffer>(device, size, vk::BufferUsageFlagBits::eTransferSrc,
				   vk::MemoryPropertyFlagBits::eHostVisible |
				   vk::MemoryPropertyFlagBits::eHostCoherent);

	packBuffer->Map();
	memset(packBuffer->mapped, 0, size);
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
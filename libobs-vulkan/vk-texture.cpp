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

gs_texture_2d::gs_texture_2d(gs_device_t *_device, uint32_t _width,
			     uint32_t _height, gs_color_format colorFormat,
			     const uint8_t *const *_data, uint32_t _flags)
	: gs_texture(_device, GS_TEXTURE_2D, colorFormat, _flags),
	  height(_height),
	  width(_width)
{
	auto texSize = width * gs_get_format_bpp(colorFormat);
	if (!gs_is_compressed_format(colorFormat)) {
		texSize /= 8;
		texSize = (texSize + 3) & 0xFFFFFFFC;
		texSize *= height;
	} else {
		texSize *= height;
		texSize /= 8;
	}

	const auto texFormat = convert_gs_format(colorFormat);
	if (texFormat == vk::Format::eUndefined)
		throw std::runtime_error("Unsupported texture format");

	buffer = std::make_unique<gs_buffer>(device, texSize,
			   vk::BufferUsageFlagBits::eTransferSrc,
			   vk::MemoryPropertyFlagBits::eHostVisible |
				   vk::MemoryPropertyFlagBits::eHostCoherent);

	if ((flags & GS_GL_DUMMYTEX) == 0 && (flags & GS_DYNAMIC) == 0 && _data) {
		buffer->Map();
		// TODO: Num levels
		const uint8_t *const *data = *&_data;
		for (uint32_t i = 0; i < 1; i++) {
			memcpy(buffer->mapped, *data, texSize);

			if (data)
				data++;
		}

		buffer->Unmap();
	}

	image = device->CreateImage(width, height, texFormat,
				    vk::ImageTiling::eOptimal,
				    vk::ImageUsageFlagBits::eTransferDst |
					    vk::ImageUsageFlagBits::eSampled,
				    vk::MemoryPropertyFlagBits::eDeviceLocal);

	vk_transitionImageLayout(device, image.image, texFormat,
				 vk::ImageLayout::eUndefined,
				 vk::ImageLayout::eTransferDstOptimal);
	vk_copyBufferToImage(device, buffer->buffer, image.image, width, height);
	vk_transitionImageLayout(device, image.image, texFormat,
				 vk::ImageLayout::eTransferDstOptimal,
				 vk::ImageLayout::eShaderReadOnlyOptimal);

	const vk::SamplerCreateInfo samplerCreateInfo(
		{}, vk::Filter::eNearest, vk::Filter::eNearest,
		vk::SamplerMipmapMode::eNearest,
		vk::SamplerAddressMode::eClampToBorder,
		vk::SamplerAddressMode::eClampToBorder,
		vk::SamplerAddressMode::eClampToBorder, 0.0f, false, 1, false,
		vk::CompareOp::eAlways, 0.0f, VK_LOD_CLAMP_NONE,
		vk::BorderColor::eIntTransparentBlack, false);

	sampler = gs_sampler_state(device, samplerCreateInfo);
	image.imageView = device->CreateImageView(
		image.image, vk::ImageViewType::e2D, texFormat,
		vk::ImageAspectFlagBits::eColor);
}
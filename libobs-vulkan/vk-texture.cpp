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

void gs_texture_2d::UpdateFromMapped()
{
	/* Update the texture from mapped memory */
	if (buffer->mapped) {
		const auto texFormat = ConvertGSFormat(format);
		vk_transitionImageLayout(device, image, texFormat,
					 vk::ImageLayout::eUndefined,
					 vk::ImageLayout::eTransferDstOptimal);
		vk_copyBufferToImage(device, buffer->buffer, image, width, height);
		vk_transitionImageLayout(device, image, texFormat,
					 vk::ImageLayout::eTransferDstOptimal,
					 vk::ImageLayout::eShaderReadOnlyOptimal);
	}
}

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

	const auto texFormat = ConvertGSFormat(colorFormat);
	if (texFormat == vk::Format::eUndefined)
		throw std::runtime_error("Unsupported texture format");

	buffer = std::make_unique<gs_buffer>(device, vk_type::VK_TEXTUREBUFFER);
	buffer->CreateBuffer(texSize,
			     vk::BufferUsageFlagBits::eTransferSrc,
			     vk::MemoryPropertyFlagBits::eHostVisible |
				     vk::MemoryPropertyFlagBits::eHostCoherent);

	buffer->Map();
	if ((flags & GS_GL_DUMMYTEX) == 0 && (flags & GS_DYNAMIC) == 0 && _data) {
		// TODO: Num levels
		const uint8_t *const *data = *&_data;
		for (uint32_t i = 0; i < 1; i++) {
			memcpy(buffer->mapped, *data, texSize);

			if (data)
				data++;
		}
	}

	/* creation of texture image and memory */
	std::tie(image, deviceMemory) = device->CreateImage(
		width, height, texFormat, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferDst |
			vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eSampled,
		vk::MemoryPropertyFlagBits::eDeviceLocal);

	/* copying buffer data to image and transitioning layouts */
	vk_transitionImageLayout(device, image, texFormat,
				 vk::ImageLayout::eUndefined,
				 vk::ImageLayout::eTransferDstOptimal);
	vk_copyBufferToImage(device, buffer->buffer, image, width, height);
	vk_transitionImageLayout(device, image, texFormat,
				 vk::ImageLayout::eTransferDstOptimal,
				 vk::ImageLayout::eShaderReadOnlyOptimal);

	/* image view creation */
	imageView = device->CreateImageView(image, texFormat,
					    vk::ImageAspectFlagBits::eColor);
}

gs_texture::~gs_texture()
{
	buffer.reset();
	const auto logicalDevice = device->GetLogicalDevice();
	if (imageView)
		logicalDevice.destroyImageView(imageView);
	if (image)
		logicalDevice.destroyImage(image);
	if (deviceMemory)
		logicalDevice.freeMemory(deviceMemory);
}

gs_texture_t *device_texture_create(gs_device_t *device, uint32_t width,
				    uint32_t height,
				    enum gs_color_format color_format,
				    uint32_t levels, const uint8_t **data,
				    uint32_t flags)
{
	gs_texture *texture = nullptr;

	try {
		texture = device->SubmitTexture(std::make_unique<gs_texture_2d>(
			device, width, height, color_format, data, flags));
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_texture_create (Vulkan): %s", e.what());
	}

	return texture;
}

void gs_texture_destroy(gs_texture_t *tex)
{
	tex->markedForDeletion = true;
}

uint32_t gs_texture_get_width(const gs_texture_t *tex)
{
	if (tex->textureType == GS_TEXTURE_2D)
		return static_cast<const gs_texture_2d *>(tex)->width;

	return 0;
}

uint32_t gs_texture_get_height(const gs_texture_t *tex)
{
	if (tex->textureType == GS_TEXTURE_2D)
		return static_cast<const gs_texture_2d *>(tex)->height;

	return 0;
}

enum gs_color_format gs_texture_get_color_format(const gs_texture_t *tex)
{
	return tex->format;
}

bool gs_texture_map(gs_texture_t *tex, uint8_t **ptr, uint32_t *linesize)
{
	if (tex->textureType != GS_TEXTURE_2D)
		return false;

	auto tex2d = static_cast<gs_texture_2d *>(tex);

	*ptr = static_cast<uint8_t *>(tex2d->buffer->mapped);

	*linesize = tex2d->width * gs_get_format_bpp(tex->format) / 8;
	*linesize = (*linesize + 3) & 0xFFFFFFFC;

	return true;
}

void gs_texture_unmap(gs_texture_t *tex)
{
	if (tex->textureType != GS_TEXTURE_2D)
		return;

	tex->UpdateFromMapped();
}

void *gs_texture_get_obj(gs_texture_t *tex)
{
	UNUSED_PARAMETER(tex);
	return nullptr;
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

void device_load_texture(gs_device_t *device, gs_texture_t *tex, int unit)
{
	device->SetTexture(tex);
}

enum gs_texture_type device_get_texture_type(const gs_texture_t *texture)
{
	return texture->textureType;
}
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

#include "vk-includes.hpp"

static inline vk::Format convert_gs_format(gs_color_format format)
{
	vk::Format result;

	switch (format) {
	case GS_A8:
		result = vk::Format::eR8Unorm;
		break;
	case GS_R8:
		result = vk::Format::eR8Unorm;
		break;
	case GS_RGBA:
		result = vk::Format::eR8G8B8A8Unorm;
		break;
	case GS_BGRX:
		result = vk::Format::eB8G8R8A8Unorm;
		break;
	case GS_BGRA:
		result = vk::Format::eB8G8R8A8Unorm;
		break;
	case GS_R10G10B10A2:
		result = vk::Format::eA2R10G10B10UnormPack32;
		break;
	case GS_RGBA16:
		result = vk::Format::eR16G16B16A16Unorm;
		break;
	case GS_R16:
		result = vk::Format::eR16Unorm;
		break;
	case GS_RGBA16F:
		result = vk::Format::eR16G16B16A16Sfloat;
		break;
	case GS_RGBA32F:
		result = vk::Format::eR32G32B32A32Sfloat;
		break;
	case GS_RG16F:
		result = vk::Format::eR16G16Sfloat;
		break;
	case GS_RG32F:
		result = vk::Format::eR32G32Sfloat;
		break;
	case GS_R8G8:
		result = vk::Format::eR8G8Unorm;
		break;
	case GS_R16F:
		result = vk::Format::eR16Sfloat;
		break;
	case GS_R32F:
		result = vk::Format::eR32Sfloat;
		break;
	case GS_DXT1:
		result = vk::Format::eD16Unorm;
		break;
	case GS_DXT3:
		result = vk::Format::eD16Unorm;
		break;
	case GS_DXT5:
		result = vk::Format::eD16Unorm;
		break;
	case GS_RGBA_UNORM:
		result = vk::Format::eR8G8B8A8Unorm;
		break;
	case GS_BGRX_UNORM:
		result = vk::Format::eB8G8R8A8Unorm;
		break;
	case GS_BGRA_UNORM:
		result = vk::Format::eB8G8R8A8Unorm;
		break;
	case GS_RG16:
		result = vk::Format::eR16G16Unorm;
		break;
	case GS_UNKNOWN:
		result = vk::Format::eUndefined;
		break;
	}

	return result;
}

inline uint32_t
vk_findMemoryType(const vk::PhysicalDeviceMemoryProperties &memoryProperties,
		  uint32_t filter, vk::MemoryPropertyFlags requirements)
{
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((filter & 1) &&
		    (memoryProperties.memoryTypes[i].propertyFlags &
		     requirements) == requirements) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}

inline void vk_copyBuffer(gs_device_t *device, const vk::Buffer &src, vk::Buffer &dst,
			  vk::DeviceSize size)
{
	const auto commandBuffer = device->BeginCommandBuffer();

	const vk::BufferCopy bufferCopy(0, 0, size);
	commandBuffer.copyBuffer(src, dst, bufferCopy);

	device->EndCommandBuffer(commandBuffer);
}

inline bool vk_hasStencilComponent(vk::Format format)
{
	return format == vk::Format::eD32SfloatS8Uint ||
	       format == vk::Format::eD24UnormS8Uint;
}

inline void vk_transitionImageLayout(gs_device_t *device, vk::Image &image,
				     vk::Format format,
				     vk::ImageLayout oldLayout,
				     vk::ImageLayout newLayout)
{
	const auto commandBuffer = device->BeginCommandBuffer();
	const auto logicalDevice = device->GetLogicalDevice();
	const auto physicalDevice = device->GetPhysicalDevice();

	vk::ImageMemoryBarrier barrier(
		{}, {}, oldLayout, newLayout, VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED, image,
		vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1,
					  0, 1));

	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.subresourceRange.aspectMask =
			vk::ImageAspectFlagBits::eDepth;
		if (vk_hasStencilComponent(format))
			barrier.subresourceRange.aspectMask |=
				vk::ImageAspectFlagBits::eStencil;
	} else
		barrier.subresourceRange.aspectMask =
			vk::ImageAspectFlagBits::eColor;

	vk::PipelineStageFlags source, destination;
	if (oldLayout == vk::ImageLayout::eUndefined &&
	    newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
		source = vk::PipelineStageFlagBits::eTopOfPipe;
		destination = vk::PipelineStageFlagBits::eTransfer;
	} else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
		   newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		source = vk::PipelineStageFlagBits::eTransfer;
		destination = vk::PipelineStageFlagBits::eFragmentShader;
	} else if (oldLayout == vk::ImageLayout::eUndefined &&
		   newLayout ==
			   vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.dstAccessMask =
			vk::AccessFlagBits::eDepthStencilAttachmentRead |
			vk::AccessFlagBits::eDepthStencilAttachmentWrite;
		source = vk::PipelineStageFlagBits::eTopOfPipe;
		destination = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	} else
		throw std::runtime_error("Unsupported layout transition");

	commandBuffer.pipelineBarrier(source, destination, {}, 0, 0, barrier);
	device->EndCommandBuffer(commandBuffer);
}

inline void vk_copyBufferToImage(gs_device_t *device, vk::Buffer buffer,
			  vk::Image &image, uint32_t width, uint32_t height)
{
	const auto commandBuffer = device->BeginCommandBuffer();

	const vk::BufferImageCopy bufferImageCopy(
		0, width, height, {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
		{0, 0, 0}, {width, height, 1});

	commandBuffer.copyBufferToImage(buffer, image,
					vk::ImageLayout::eTransferDstOptimal,
					bufferImageCopy);

	device->EndCommandBuffer(commandBuffer);
}

inline void vk_copyImageToBuffer(gs_device_t *device, vk::Image image,
			  vk::Buffer &buffer, uint32_t width, uint32_t height)
{
	const auto commandBuffer = device->BeginCommandBuffer();

	const vk::BufferImageCopy bufferImageCopy(
		0, width, height, {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
		{0, 0, 0}, {width, height, 1});

	commandBuffer.copyImageToBuffer(image,
					vk::ImageLayout::eTransferSrcOptimal,
					buffer, bufferImageCopy);

	device->EndCommandBuffer(commandBuffer);
}
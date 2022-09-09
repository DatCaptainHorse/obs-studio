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

static inline vk::Format ConvertGSFormat(gs_color_format format)
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

static inline vk::SamplerAddressMode
ConvertGSAddressMode(gs_address_mode mode)
{
	vk::SamplerAddressMode samplerMode;

	switch (mode) {
	case GS_ADDRESS_WRAP:
		samplerMode = vk::SamplerAddressMode::eRepeat;
		break;
	case GS_ADDRESS_CLAMP:
		samplerMode = vk::SamplerAddressMode::eClampToEdge;
		break;
	case GS_ADDRESS_MIRROR:
		samplerMode = vk::SamplerAddressMode::eMirroredRepeat;
		break;
	case GS_ADDRESS_BORDER:
		samplerMode = vk::SamplerAddressMode::eClampToBorder;
		break;
	case GS_ADDRESS_MIRRORONCE:
		samplerMode = vk::SamplerAddressMode::eMirrorClampToEdge;
		break;
	default:
		samplerMode = vk::SamplerAddressMode::eRepeat;
		break;
	}

	return samplerMode;
}

static inline gs_address_mode ConvertVkAddressMode(vk::SamplerAddressMode mode)
{
	gs_address_mode result;

	switch (mode) {
	case vk::SamplerAddressMode::eRepeat:
		result = GS_ADDRESS_WRAP;
		break;
	case vk::SamplerAddressMode::eClampToEdge:
		result = GS_ADDRESS_CLAMP;
		break;
	case vk::SamplerAddressMode::eMirroredRepeat:
		result = GS_ADDRESS_MIRROR;
		break;
	case vk::SamplerAddressMode::eClampToBorder:
		result = GS_ADDRESS_BORDER;
		break;
	case vk::SamplerAddressMode::eMirrorClampToEdge:
		result = GS_ADDRESS_MIRRORONCE;
		break;
	default:
		result = GS_ADDRESS_WRAP;
		break;
	}

	return result;
}

static inline std::tuple<vk::Filter, vk::Filter, vk::SamplerMipmapMode> ConvertGSFilter(gs_sample_filter filter)
{
	vk::Filter minFilter, magFilter;
	vk::SamplerMipmapMode mipMode;

	switch (filter) {
	case GS_FILTER_POINT:
		minFilter = magFilter = vk::Filter::eNearest;
		mipMode = vk::SamplerMipmapMode::eNearest;
		break;
	case GS_FILTER_LINEAR:
		minFilter = magFilter = vk::Filter::eLinear;
		mipMode = vk::SamplerMipmapMode::eLinear;
		break;
	case GS_FILTER_MIN_MAG_POINT_MIP_LINEAR:
		minFilter = vk::Filter::eNearest;
		magFilter = vk::Filter::eNearest;
		mipMode = vk::SamplerMipmapMode::eLinear;
		break;
	case GS_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
		minFilter = vk::Filter::eNearest;
		magFilter = vk::Filter::eLinear;
		mipMode = vk::SamplerMipmapMode::eNearest;
		break;
	case GS_FILTER_MIN_POINT_MAG_MIP_LINEAR:
		minFilter = vk::Filter::eNearest;
		magFilter = vk::Filter::eLinear;
		mipMode = vk::SamplerMipmapMode::eLinear;
		break;
	case GS_FILTER_MIN_LINEAR_MAG_MIP_POINT:
		minFilter = vk::Filter::eLinear;
		magFilter = vk::Filter::eNearest;
		mipMode = vk::SamplerMipmapMode::eNearest;
		break;
	case GS_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		minFilter = vk::Filter::eLinear;
		magFilter = vk::Filter::eNearest;
		mipMode = vk::SamplerMipmapMode::eLinear;
		break;
	case GS_FILTER_MIN_MAG_LINEAR_MIP_POINT:
		minFilter = vk::Filter::eLinear;
		magFilter = vk::Filter::eLinear;
		mipMode = vk::SamplerMipmapMode::eNearest;
		break;
	case GS_FILTER_ANISOTROPIC:
		minFilter = magFilter = vk::Filter::eLinear;
		mipMode = vk::SamplerMipmapMode::eLinear;
		break;
	default:
		minFilter = magFilter = vk::Filter::eNearest;
		mipMode = vk::SamplerMipmapMode::eNearest;
		break;
	}

	return std::make_tuple(minFilter, magFilter, mipMode);
}

static inline gs_sample_filter ConvertVkFilter(
	std::tuple<vk::Filter, vk::Filter, vk::SamplerMipmapMode> filter)
{
	auto [min, mag, mip] = filter;
	gs_sample_filter result;

	if (min == vk::Filter::eNearest && mag == vk::Filter::eNearest &&
	    mip == vk::SamplerMipmapMode::eNearest)
		result = GS_FILTER_POINT;
	else if (min == vk::Filter::eLinear && mag == vk::Filter::eLinear &&
		 mip == vk::SamplerMipmapMode::eLinear)
		result = GS_FILTER_LINEAR;
	else if (min == vk::Filter::eNearest && mag == vk::Filter::eNearest &&
		 mip == vk::SamplerMipmapMode::eLinear)
		result = GS_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	else if (min == vk::Filter::eNearest && mag == vk::Filter::eLinear &&
		 mip == vk::SamplerMipmapMode::eNearest)
		result = GS_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
	else if (min == vk::Filter::eNearest && mag == vk::Filter::eLinear &&
		 mip == vk::SamplerMipmapMode::eLinear)
		result = GS_FILTER_MIN_POINT_MAG_MIP_LINEAR;
	else if (min == vk::Filter::eLinear && mag == vk::Filter::eNearest &&
		 mip == vk::SamplerMipmapMode::eNearest)
		result = GS_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	else if (min == vk::Filter::eLinear && mag == vk::Filter::eNearest &&
		 mip == vk::SamplerMipmapMode::eLinear)
		result = GS_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
	else if (min == vk::Filter::eLinear && mag == vk::Filter::eLinear &&
		 mip == vk::SamplerMipmapMode::eNearest)
		result = GS_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	else if (min == vk::Filter::eLinear && mag == vk::Filter::eLinear &&
		 mip == vk::SamplerMipmapMode::eLinear)
		result = GS_FILTER_ANISOTROPIC;
	else
		result = GS_FILTER_POINT;

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
	device->BeginCommandBuffer();

	const vk::BufferCopy bufferCopy(0, 0, size);
	device->instantBuffer.copyBuffer(src, dst, bufferCopy);

	device->EndCommandBuffer();
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
	device->BeginCommandBuffer();
	const auto logicalDevice = device->GetLogicalDevice();

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

	device->instantBuffer.pipelineBarrier(source, destination, {}, {}, {}, barrier);
	device->EndCommandBuffer();
}

inline void vk_copyBufferToImage(gs_device_t *device, vk::Buffer buffer,
			  vk::Image &image, uint32_t width, uint32_t height)
{
	device->BeginCommandBuffer();

	const vk::BufferImageCopy bufferImageCopy(
		0, width, height, {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
		{0, 0, 0}, {width, height, 1});

	device->instantBuffer.copyBufferToImage(buffer, image,
					vk::ImageLayout::eTransferDstOptimal,
					bufferImageCopy);

	device->EndCommandBuffer();
}

inline void vk_copyImageToBuffer(gs_device_t *device, vk::Image image,
			  vk::Buffer &buffer, uint32_t width, uint32_t height)
{
	device->BeginCommandBuffer();

	const vk::BufferImageCopy bufferImageCopy(
		0, width, height, {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
		{0, 0, 0}, {width, height, 1});

	device->instantBuffer.copyImageToBuffer(image,
					vk::ImageLayout::eTransferSrcOptimal,
					buffer, bufferImageCopy);

	device->EndCommandBuffer();
}

inline void vk_copyImagetoImage(gs_device_t *device, vk::Image srcImage,
			  vk::Image &dstImage, uint32_t width, uint32_t height)
{
	device->BeginCommandBuffer();

	const vk::ImageCopy imageCopy(
		{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, {0, 0, 0},
		{vk::ImageAspectFlagBits::eColor, 0, 0, 1}, {0, 0, 0},
		{width, height, 1});
	device->instantBuffer.copyImage(srcImage, vk::ImageLayout::eTransferSrcOptimal,
					dstImage, vk::ImageLayout::eTransferDstOptimal,
					imageCopy);
	device->EndCommandBuffer();
}

inline size_t vk_padUniformBuffer(gs_device_t *device, size_t size)
{
	const auto minimumAlignment = device->deviceProperties.limits.minUniformBufferOffsetAlignment;
	auto alignedSize = size;
	if (minimumAlignment > 0)
		alignedSize = (size + minimumAlignment - 1) & ~(minimumAlignment - 1);

	return alignedSize;
}

inline std::string GetStringBetween(const std::string &str, const std::string &start,
			     const std::string &end)
{
	std::size_t start_pos = str.find_last_of(start);
	if (start_pos == std::string::npos)
		return "";

	start_pos += start.length();
	std::size_t end_pos = str.find(end, start_pos);
	if (end_pos == std::string::npos)
		return "";

	return str.substr(start_pos, end_pos - start_pos);
}

inline std::string GetStringBetweenT(const std::string &line,
				     const std::string &start,
				     const std::string &end)
{
	size_t start_pos = line.find(start);
	if (start_pos == std::string::npos)
		return "";

	start_pos += start.length();
	size_t end_pos = line.find(end, start_pos);
	if (end_pos == std::string::npos)
		return "";

	return line.substr(start_pos, end_pos - start_pos);
}

inline void AddToStringBetween(std::string &str, const std::string &start,
			       const std::string &end, const std::string &add)
{
	std::size_t start_pos = str.find_last_of(start);
	if (start_pos == std::string::npos)
		return;

	start_pos += start.length();
	std::size_t end_pos = str.find(end, start_pos);
	if (end_pos == std::string::npos)
		return;

	str.insert(end_pos, add);
}

inline void PrependToStringBefore(std::string &str, const std::string &start,
				  const std::string &add)
{
	size_t start_pos = str.find(start);
	if (start_pos == std::string::npos)
		return;

	str.insert(start_pos, add);
}

inline void AppendToStringAfter(std::string &str, const std::string &start,
				const std::string &add)
{
	size_t start_pos = str.find(start);
	if (start_pos == std::string::npos)
		return;

	start_pos += start.length();
	str.insert(start_pos, add);
}

inline void ReplaceAllInString(std::string &str, const std::string &find,
			       const std::string &replace)
{
	size_t pos = 0;
	while ((pos = str.find(find, pos)) != std::string::npos) {
		str.replace(pos, find.length(), replace);
		pos += replace.length();
	}
}
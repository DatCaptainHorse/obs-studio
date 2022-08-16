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

#include <graphics/vec4.h>

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

gs_sampler_state::gs_sampler_state(gs_device_t *device,
				   const gs_sampler_info *_info)
	: device(device), info(*_info)
{
	auto [min, mag, mip] = ConvertGSFilter(info.filter);
	const vk::SamplerCreateInfo samplerCreateInfo(
		{}, min, mag, mip, ConvertGSAddressMode(info.address_u),
		ConvertGSAddressMode(info.address_v),
		ConvertGSAddressMode(info.address_w), 0.0f,
		info.max_anisotropy > 1, static_cast<float>(info.max_anisotropy), true,
		vk::CompareOp::eAlways, 0.0f, VK_LOD_CLAMP_NONE,
		vk::BorderColor::eIntTransparentBlack);

	// TODO: Use extension?
	//  VK_BORDER_COLOR_FLOAT_CUSTOM_EXT requires
	//  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER to be used
	/*vec4 v4;
	vec4_from_rgba(&v4, info->border_color);*/

	try {
		sampler = device->GetLogicalDevice().createSampler(
			samplerCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

gs_sampler_state::gs_sampler_state(gs_device_t *device,
				   vk::SamplerCreateInfo _info)
	: device(device)
{
	info.address_u = ConvertVkAddressMode(_info.addressModeU);
	info.address_v = ConvertVkAddressMode(_info.addressModeV);
	info.address_w = ConvertVkAddressMode(_info.addressModeW);
	info.border_color = 0; // TODO: This
	info.max_anisotropy = static_cast<int>(_info.maxAnisotropy);
	info.filter = ConvertVkFilter(std::make_tuple(
		_info.minFilter, _info.magFilter, _info.mipmapMode));

	try {
		sampler = device->GetLogicalDevice().createSampler(_info);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}
}

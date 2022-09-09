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

gs_sampler_state::gs_sampler_state(gs_device_t *device,
				   const gs_sampler_info *_info)
	: vk_object(device, vk_type::VK_SAMPLER)
{
	auto [min, mag, mip] = ConvertGSFilter(_info->filter);
	info = vk::SamplerCreateInfo(
		{}, min, mag, mip, ConvertGSAddressMode(_info->address_u),
		ConvertGSAddressMode(_info->address_v),
		ConvertGSAddressMode(_info->address_w), 0.0f,
		_info->max_anisotropy > 1, static_cast<float>(_info->max_anisotropy), true,
		vk::CompareOp::eAlways, 0.0f, VK_LOD_CLAMP_NONE,
		vk::BorderColor::eIntTransparentBlack);

	// TODO: Use extension?
	//  VK_BORDER_COLOR_FLOAT_CUSTOM_EXT requires
	//  VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER to be used
	/*vec4 v4;
	vec4_from_rgba(&v4, info->border_color);*/

	try {
		sampler = device->GetLogicalDevice().createSampler(info);
	} catch (const vk::Error &e) {
		throw std::runtime_error(e.what());
	}
}

gs_samplerstate_t *
device_samplerstate_create(gs_device_t *device,
			   const gs_sampler_info *info)
{
	if (!info)
		return nullptr;

	try {
		return new gs_sampler_state(device, info);
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_samplerstate_create (Vulkan): %s", e.what());
		return nullptr;
	}
}

void device_load_samplerstate(gs_device_t *device,
			      gs_samplerstate_t *samplerstate, int unit)
{
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(samplerstate);
	UNUSED_PARAMETER(unit);
}


void gs_samplerstate_destroy(gs_samplerstate_t *samplerstate)
{
	samplerstate->markedForDeletion = true;
}
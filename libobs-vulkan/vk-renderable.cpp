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

VulkanRenderable::VulkanRenderable(gs_device_t *_device, VulkanShader *_shader, gs_vertex_buffer *_vertexBuffer, gs_index_buffer *_indexBuffer)
	: vk_object(_device, vk_type::VK_RENDERABLE),
	  shader(_shader),
	  vertexBuffer(_vertexBuffer),
	  indexBuffer(_indexBuffer)
{
}

VulkanRenderable::~VulkanRenderable()
{
	const auto logicalDevice = device->GetLogicalDevice();
	if (!descriptorSets.empty())
		logicalDevice.freeDescriptorSets(device->descriptorPool, descriptorSets);
}
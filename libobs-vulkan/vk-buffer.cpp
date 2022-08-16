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

#include <graphics/vec3.h>

void gs_buffer::Map()
{
	if (!mapped){
		mapped = device->GetLogicalDevice().mapMemory(deviceMemory, 0, deviceSize);
	}
}

void gs_buffer::Unmap()
{
	if (mapped) {
		device->GetLogicalDevice().unmapMemory(deviceMemory);
		mapped = nullptr;
	}
}

gs_buffer::gs_buffer(gs_device_t *_device, vk::DeviceSize size,
		     vk::BufferUsageFlags usage,
		     vk::MemoryPropertyFlags properties)
	: device(_device), deviceSize(size), bufferUsageFlags(usage),
	  memoryPropertyFlags(properties), mapped(nullptr)
{
	const auto physicalDevice = device->GetPhysicalDevice();
	const auto logicalDevice = device->GetLogicalDevice();

	const vk::BufferCreateInfo bufferCreateInfo(
		{}, deviceSize, bufferUsageFlags, vk::SharingMode::eExclusive);

	try {
		buffer = logicalDevice.createBuffer(bufferCreateInfo);
	} catch (const vk::Error &e) {
		throw std::runtime_error(e.what());
	}

	const auto memoryRequirements =
		logicalDevice.getBufferMemoryRequirements(buffer);

	uint32_t memoryTypeIndex = vk_findMemoryType(
		physicalDevice.getMemoryProperties(),
		memoryRequirements.memoryTypeBits, memoryPropertyFlags);

	const vk::MemoryAllocateInfo memoryAllocateInfo(memoryRequirements.size,
							memoryTypeIndex);

	deviceMemory = logicalDevice.allocateMemory(memoryAllocateInfo);
	logicalDevice.bindBufferMemory(buffer, deviceMemory, 0);
}

gs_buffer::~gs_buffer()
{
	const auto logicalDevice = device->GetLogicalDevice();
	Unmap();
	logicalDevice.freeMemory(deviceMemory);
	logicalDevice.destroyBuffer(buffer);
}

gs_vertex_buffer::gs_vertex_buffer(gs_device_t *_device, vk::DeviceSize _size,
				   const void *data)
	: gs_buffer(_device, _size,
		    vk::BufferUsageFlagBits::eVertexBuffer |
			    vk::BufferUsageFlagBits::eTransferDst,
		    vk::MemoryPropertyFlagBits::eDeviceLocal)
{
	const auto logicalDevice = device->GetLogicalDevice();

	/* host buffer for copying data to */
	const auto hostBuffer =
		std::make_unique<gs_buffer>(device, deviceSize, vk::BufferUsageFlagBits::eTransferSrc,
			  vk::MemoryPropertyFlagBits::eHostVisible |
				  vk::MemoryPropertyFlagBits::eHostCoherent);


	hostBuffer->Map();
	memcpy(hostBuffer->mapped, data, deviceSize);
	hostBuffer->Unmap();

	/* copy data to device buffer */
	vk_copyBuffer(device, hostBuffer->buffer, buffer, deviceSize);
}

gs_index_buffer::gs_index_buffer(gs_device_t *_device, vk::DeviceSize _size,
				 const void *data)
	: gs_buffer(_device, _size,
		    vk::BufferUsageFlagBits::eIndexBuffer |
			    vk::BufferUsageFlagBits::eTransferDst,
		    vk::MemoryPropertyFlagBits::eDeviceLocal)
{
	const auto logicalDevice = device->GetLogicalDevice();

	/* host buffer for copying data to */
	const auto hostBuffer =
		std::make_unique<gs_buffer>(device, deviceSize, vk::BufferUsageFlagBits::eTransferSrc,
			  vk::MemoryPropertyFlagBits::eHostVisible |
				  vk::MemoryPropertyFlagBits::eHostCoherent);

	hostBuffer->Map();
	memcpy(hostBuffer->mapped, data, deviceSize);
	hostBuffer->Unmap();

	/* copy data to device buffer */
	vk_copyBuffer(device, hostBuffer->buffer, buffer, deviceSize);
}

gs_uniform_buffer::gs_uniform_buffer(gs_device_t *_device, vk::DeviceSize _size,
				     const void *data)
	: gs_buffer(_device, _size,
		    vk::BufferUsageFlagBits::eUniformBuffer |
			    vk::BufferUsageFlagBits::eTransferDst,
		    vk::MemoryPropertyFlagBits::eHostVisible |
			    vk::MemoryPropertyFlagBits::eHostCoherent)
{
	if (data) {
		const auto logicalDevice = device->GetLogicalDevice();

		Map();
		memcpy(mapped, data, deviceSize);
		Unmap();
	}
}

gs_vertbuffer_t *device_vertexbuffer_create(gs_device_t *device,
					    gs_vb_data *data,
					    uint32_t flags)
{
	gs_vertex_buffer *vb = nullptr;

	try {
		vb = new gs_vertex_buffer(device, data->num * sizeof(vec3),
					  data->points);

		vb->vbd = data;
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_vertexbuffer_create (Vulkan): %s",
		     e.what());
	}

	return vb;
}

gs_indexbuffer_t *device_indexbuffer_create(gs_device_t *device,
					    gs_index_type type,
					    void *indices, size_t num,
					    uint32_t flags)
{
	gs_index_buffer *ib = nullptr;

	const size_t width = type == GS_UNSIGNED_LONG ? 4 : 2;

	try {
		ib = new gs_index_buffer(device, width * num, indices);
		ib->indices = indices;
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_indexbuffer_create (Vulkan): %s",
		     e.what());
	}

	return ib;
}

gs_vb_data *gs_vertexbuffer_get_data(const gs_vertbuffer_t *vertbuffer)
{
	return vertbuffer->vbd;
}

void *gs_indexbuffer_get_data(const gs_indexbuffer_t *indexbuffer)
{
	return indexbuffer->indices;
}
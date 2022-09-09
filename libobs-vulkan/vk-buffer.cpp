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
	if (!mapped) {
		mapped = device->GetLogicalDevice().mapMemory(deviceMemory, 0,
							      deviceSize);
	}
}

void gs_buffer::Unmap()
{
	if (mapped) {
		device->GetLogicalDevice().unmapMemory(deviceMemory);
		mapped = nullptr;
	}
}

void gs_buffer::CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
		     vk::MemoryPropertyFlags properties)
{
	deviceSize = size;
	bufferUsageFlags = usage;
	memoryPropertyFlags = properties;

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

gs_buffer::gs_buffer(gs_device_t *_device, vk_type _type)
	: vk_object(_device, _type)
{
}

gs_buffer::~gs_buffer()
{
	if (device) {
		Unmap();
		const auto logicalDevice = device->GetLogicalDevice();
		logicalDevice.freeMemory(deviceMemory);
		logicalDevice.destroyBuffer(buffer);
	}
}

std::tuple<std::vector<vk_vb_data>, vk::DeviceSize, vk_vb_info> ConvertVertexData(const gs_vb_data *data)
{
	vk_vb_info info;
	std::vector<vk_vb_data> vbd;
	vbd.resize(data->num);
	vk::DeviceSize totalSize = 0;

	if (data->points) {
		info.hasPoints = true;
		totalSize += data->num * sizeof(vec3);
		for (size_t i = 0; i < data->num; i++)
			vbd[i].point = data->points[i];
	}

	if (data->normals) {
		info.hasNormals = true;
		totalSize += data->num * sizeof(vec3);
		for (size_t i = 0; i < data->num; i++)
			vbd[i].normal = data->normals[i];
	}

	if (data->tangents) {
		info.hasTangents = true;
		totalSize += data->num * sizeof(vec3);
		for (size_t i = 0; i < data->num; i++)
			vbd[i].tangent = data->tangents[i];
	}

	if (data->colors) {
		info.hasColors = true;
		totalSize += data->num * sizeof(uint32_t);
		for (size_t i = 0; i < data->num; i++)
			vbd[i].color = data->colors[i];
	}

	for (size_t i = 0; i < data->num_tex; i++) {
		info.hasUVs = true;
		gs_tvertarray *tv = &data->tvarray[i];
		totalSize += data->num * sizeof(float) * tv->width;
		for (size_t j = 0; j < data->num; j++) {
			//if (tv->width == 1)
			//	vbd[j].uvF = *static_cast<float *>(tv->array);
			if (tv->width == 2)
				vbd[j].uv = *static_cast<vec2 *>(tv->array);
			//else if (tv->width == 4)
			//	vbd[j].uvF3 = *static_cast<vec3 *>(tv->array);
		}
	}

	return std::make_tuple(vbd, totalSize, info);
}

/* converts vk_vb_data struct to void* data, removing empty values from it */
void *ConvertToData(const std::vector<vk_vb_data> &vbd, const vk_vb_info &info)
{
	size_t totalSize = 0;
	for (const auto &v : vbd) {
		if (info.hasPoints)
			totalSize += sizeof(vec3);
		if (info.hasNormals)
			totalSize += sizeof(vec3);
		if (info.hasTangents)
			totalSize += sizeof(vec3);
		if (info.hasColors)
			totalSize += sizeof(uint32_t);
		if (info.hasUVs)
			totalSize += sizeof(vec2);
	}

	void *data = malloc(totalSize);
	size_t offset = 0;
	for (const auto &v : vbd) {
		if (info.hasPoints) {
			memcpy(static_cast<char *>(data) + offset, &v.point, sizeof(vec3));
			offset += sizeof(vec3);
		}
		if (info.hasNormals) {
			memcpy(static_cast<char *>(data) + offset, &v.normal, sizeof(vec3));
			offset += sizeof(vec3);
		}
		if (info.hasTangents) {
			memcpy(static_cast<char *>(data) + offset, &v.tangent, sizeof(vec3));
			offset += sizeof(vec3);
		}
		if (info.hasColors) {
			memcpy(static_cast<char *>(data) + offset, &v.color, sizeof(uint32_t));
			offset += sizeof(uint32_t);
		}
		if (info.hasUVs) {
			memcpy(static_cast<char *>(data) + offset, &v.uv, sizeof(vec2));
			offset += sizeof(vec2);
		}
	}
	return data;
}

void gs_vertex_buffer::Update(const void *_data, size_t size)
{
	auto [vertexData, totalSize, vertexInfo] = ConvertVertexData(static_cast<const gs_vb_data *>(_data));
	if (totalSize > deviceSize)
		throw std::runtime_error("Given vertex data is too large for the buffer");

	vbd = vertexData;

	memcpy(hostBuffer->mapped, ConvertToData(vbd, vertexInfo), deviceSize);

	/* copy data to device buffer */
	vk_copyBuffer(device, hostBuffer->buffer, buffer, deviceSize);
}

gs_vertex_buffer::gs_vertex_buffer(gs_device_t *_device, gs_vb_data *_data)
	: gs_buffer(_device, vk_type::VK_VERTEXBUFFER), data(_data)
{
	auto [vertexData, totalSize, vertexInfo] = ConvertVertexData(data);
	vbd = vertexData;

	CreateBuffer(totalSize,
		     vk::BufferUsageFlagBits::eVertexBuffer |
			     vk::BufferUsageFlagBits::eTransferDst,
		     vk::MemoryPropertyFlagBits::eDeviceLocal);

	/* host buffer for copying data to */
	hostBuffer = std::make_unique<gs_buffer>(device, vk_type::VK_GENERICBUFFER);
	hostBuffer->CreateBuffer(
		deviceSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent);

	hostBuffer->Map();
	memcpy(hostBuffer->mapped, ConvertToData(vbd, vertexInfo), deviceSize);

	/* copy data to device buffer */
	vk_copyBuffer(device, hostBuffer->buffer, buffer, deviceSize);
}

gs_index_buffer::gs_index_buffer(gs_device_t *_device, gs_index_type type,
				 size_t _nIndices,
				 vk::DeviceSize _size, void *data)
	: gs_buffer(_device, vk_type::VK_INDEXBUFFER),
	  indexType(type),
	  indices(data),
	  nIndices(_nIndices)
{
	CreateBuffer(_size,
		     vk::BufferUsageFlagBits::eIndexBuffer |
			     vk::BufferUsageFlagBits::eTransferDst,
		     vk::MemoryPropertyFlagBits::eDeviceLocal);

	/* host buffer for copying data to */
	hostBuffer = std::make_unique<gs_buffer>(device, vk_type::VK_GENERICBUFFER);
	hostBuffer->CreateBuffer(
		deviceSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent);

	hostBuffer->Map();
	memcpy(hostBuffer->mapped, data, deviceSize);

	/* copy data to device buffer */
	vk_copyBuffer(device, hostBuffer->buffer, buffer, deviceSize);
}

void gs_uniform_buffer::Update(const void *data, size_t size)
{
	memcpy(hostBuffer->mapped, data, size);
	/* copy data to device buffer */
	vk_copyBuffer(device, hostBuffer->buffer, buffer, size);
}

gs_uniform_buffer::gs_uniform_buffer(gs_device_t *_device, vk::DeviceSize _size)
	: gs_buffer(_device, vk_type::VK_UNIFORMBUFFER)
{
	CreateBuffer(vk_padUniformBuffer(device, _size),
		     vk::BufferUsageFlagBits::eUniformBuffer |
			     vk::BufferUsageFlagBits::eTransferDst,
		     vk::MemoryPropertyFlagBits::eDeviceLocal);

	/* host buffer for copying data to */
	hostBuffer = std::make_unique<gs_buffer>(device, vk_type::VK_GENERICBUFFER);
	hostBuffer->CreateBuffer(
		deviceSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent);

	hostBuffer->Map();
}

gs_vertbuffer_t *device_vertexbuffer_create(gs_device_t *device,
					    gs_vb_data *data, uint32_t flags)
{
	gs_vertex_buffer *vb = nullptr;

	try {
		vb = static_cast<gs_vertex_buffer *>(device->SubmitBuffer(
			std::make_unique<gs_vertex_buffer>(device, data)));
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_vertexbuffer_create (Vulkan): %s",
		     e.what());
	}

	return vb;
}

gs_indexbuffer_t *device_indexbuffer_create(gs_device_t *device,
					    gs_index_type type, void *indices,
					    size_t num, uint32_t flags)
{
	gs_index_buffer *ib = nullptr;

	const size_t width = type == GS_UNSIGNED_LONG ? 4 : 2;

	try {
		ib = static_cast<gs_index_buffer *>(
			device->SubmitBuffer(std::make_unique<gs_index_buffer>(
				device, type, num, width * num, indices)));
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_indexbuffer_create (Vulkan): %s",
		     e.what());
	}

	return ib;
}

gs_vb_data *gs_vertexbuffer_get_data(const gs_vertbuffer_t *vertbuffer)
{
	return vertbuffer->data;
}

void *gs_indexbuffer_get_data(const gs_indexbuffer_t *indexbuffer)
{
	return indexbuffer->indices;
}

void device_load_vertexbuffer(gs_device_t *device, gs_vertbuffer_t *vertbuffer)
{
	device->SetBuffer(vertbuffer);
}

void device_load_indexbuffer(gs_device_t *device, gs_indexbuffer_t *indexbuffer)
{
	device->SetBuffer(indexbuffer);
}

void gs_vertexbuffer_destroy(gs_vertbuffer_t *vertbuffer)
{
	vertbuffer->markedForDeletion = true;
}

void gs_vertexbuffer_flush(gs_vertbuffer_t *vertbuffer)
{
	vertbuffer->Update(vertbuffer->data, 0);
}

void gs_vertexbuffer_flush_direct(gs_vertbuffer_t *vertbuffer,
				  const gs_vb_data *data)
{
	vertbuffer->Update(data, 0);
}

void gs_indexbuffer_destroy(gs_indexbuffer_t *indexbuffer)
{
	indexbuffer->markedForDeletion = true;
}

void gs_indexbuffer_flush(gs_indexbuffer_t *indexbuffer)
{
	UNUSED_PARAMETER(indexbuffer);
}

void gs_indexbuffer_flush_direct(gs_indexbuffer_t *indexbuffer,
				 const void *data)
{
	UNUSED_PARAMETER(indexbuffer);
	UNUSED_PARAMETER(data);
}

size_t gs_indexbuffer_get_num_indices(const gs_indexbuffer_t *indexbuffer)
{
	return indexbuffer->nIndices;
}

enum gs_index_type gs_indexbuffer_get_type(const gs_indexbuffer_t *indexbuffer)
{
	return indexbuffer->indexType;
}
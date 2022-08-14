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

#include <tuple>
#include <vector>
#include <cstdint>

/* TODO: Other platforms */
#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.hpp>

#include <util/base.h>
#include <graphics/graphics.h>
#include <graphics/device-exports.h>

static inline const char *GetVulkanVendor(std::uint32_t vendorID)
{
	switch (vendorID) {
	case 4098:
		return "AMD";
	case 4318:
		return "NVIDIA";
	case 32902:
		return "Intel";
	case 20803:
		return "Qualcomm";
	case 5045:
		return "ARM";
	case 4112:
		return "ImgTec";
	default:
		return "Unknown";
	}
}

static inline const char *GetVulkanDeviceType(vk::PhysicalDeviceType type)
{
	switch (type) {
	case vk::PhysicalDeviceType::eDiscreteGpu:
		return "Discrete GPU";
	case vk::PhysicalDeviceType::eIntegratedGpu:
		return "Integrated GPU";
	case vk::PhysicalDeviceType::eCpu:
		return "CPU";
	case vk::PhysicalDeviceType::eVirtualGpu:
		return "Virtual GPU";
	case vk::PhysicalDeviceType::eOther:
		return "Other";
	default:
		return "Unknown";
	}
}

static inline const char *GetVulkanDriverVersion(std::uint32_t driverVersion,
						 std::uint32_t vendorID)
{
	char *version = new char[32];
	switch (vendorID) {
	case 4318:
		std::snprintf(version, 32, "%u.%u.%u.%u",
			      (driverVersion >> 22) & 0x3ff,
			      (driverVersion >> 14) & 0x0ff,
			      (driverVersion >> 6) & 0x0ff,
			      driverVersion & 0x003f);
#ifdef _WIN32
	case 32902:
		std::snprintf(version, 32, "%u.%u", driverVersion >> 14,
			      driverVersion & 0x3fff);
#endif
	default:
		std::snprintf(version, 32, "%u.%u.%u", driverVersion >> 22,
			      (driverVersion >> 12) & 0x3ff,
			      driverVersion & 0xfff);
	}
	return version;
}

struct gs_device {
	char *deviceName;
	std::uint32_t vendorID;
	std::tuple<vk::PhysicalDevice, vk::Device> device;
	vk::Queue queue;
	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;
	vk::DescriptorPool descriptorPool;
	vk::DescriptorSetLayout descriptorSetLayout;

	gs_device(const vk::PhysicalDevice &physicalDevice);
	~gs_device();
};

struct vulkan_instance {
	vk::Instance instance;
	std::vector<gs_device>
		devices; /* REM: Doing opposite of previous implementations, as there might be future use-cases for multi-GPU support */
	std::vector<const char *> layers, extensions;

	vulkan_instance(std::vector<const char *> requestedLayers,
			std::vector<const char *> requestedExtensions);

	~vulkan_instance();
};

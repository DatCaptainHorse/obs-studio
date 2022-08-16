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

vulkan_instance::vulkan_instance(std::vector<const char *> requestedLayers,
				 std::vector<const char *> requestedExtensions)
{
	/* TODO: Hardcoded versions for now */
	vk::ApplicationInfo applicationInfo(
		"OBS-Studio",
		VK_MAKE_API_VERSION(0, 28, 0, 0),
		"libobs-vulkan",
		VK_MAKE_API_VERSION(0, 1, 0, 0),
		VK_API_VERSION_1_2
	);

	vk::InstanceCreateInfo instanceCreateInfo(
		{},
		&applicationInfo,
		static_cast<uint32_t>(requestedLayers.size()),
		requestedLayers.data(),
		static_cast<uint32_t>(requestedExtensions.size()),
		requestedExtensions.data()
	);

	blog(LOG_INFO, "\tCreating Vulkan instance");
	blog(LOG_INFO, "\t  API version: %u.%u.%u",
	     VK_VERSION_MAJOR(applicationInfo.apiVersion),
	     VK_VERSION_MINOR(applicationInfo.apiVersion),
	     VK_VERSION_PATCH(applicationInfo.apiVersion));

	blog(LOG_INFO, "\t  Requested layers:");
	for (const auto &layer : requestedLayers)
		blog(LOG_INFO, "\t    %s", layer);

	blog(LOG_INFO, "\t  Requested extensions:");
	for (const auto &extension : requestedExtensions)
		blog(LOG_INFO, "\t    %s", extension);

	try {
		vulkanInstance = vk::createInstance(instanceCreateInfo);
	} catch (const vk::SystemError &e) {
		throw std::runtime_error(e.what());
	}
}

vulkan_instance::~vulkan_instance()
{
	vulkanInstance.destroy();
}

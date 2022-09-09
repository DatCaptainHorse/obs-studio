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

#include <obs-config.h>

#include <iostream>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	      VkDebugUtilsMessageTypeFlagsEXT messageType,
	      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	      void *pUserData)
{

	std::string severity;
	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		severity = "VERBOSE";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		severity = "INFO";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		severity = "WARNING";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		severity = "ERROR";
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
		severity = "UNKNOWN";
		break;
	}

	std::string type;
	switch (messageType) {
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		type = "GENERAL";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		type = "VALIDATION";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		type = "PERFORMANCE";
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT:
		type = "UNKNOWN";
		break;
	}

	bool isError = false;
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		isError = true;

	std::cout << "Vulkan [" << type << "::" << severity << "] "
		  << pCallbackData->pMessage << std::endl;

	blog(isError ? LOG_ERROR : LOG_INFO, "Vulkan [%s::%s]: %s",
	     type.c_str(), severity.c_str(), pCallbackData->pMessage);

	return false;
}

vulkan_instance::vulkan_instance(std::vector<const char *> requestedLayers,
				 std::vector<const char *> requestedExtensions)
{
	if (volkInitialize() != VK_SUCCESS)
		throw std::runtime_error("Failed to initialize volk");

	const vk::DynamicLoader dl;
	const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddress = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddress);

	/* TODO: Hardcoded version for now */
	vk::ApplicationInfo applicationInfo(
		"OBS-Studio",
		VK_MAKE_API_VERSION(0, LIBOBS_API_MAJOR_VER,
				    LIBOBS_API_MINOR_VER, LIBOBS_API_PATCH_VER),
		"libobs-vulkan", VK_MAKE_API_VERSION(0, 1, 0, 0),
		VK_API_VERSION_1_2);

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

	VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkanInstance);

#if _DEBUG
	debugMessenger = vulkanInstance.createDebugUtilsMessengerEXT(
		vk::DebugUtilsMessengerCreateInfoEXT(
			{},
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
			vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
			vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
			&debugCallback
		)
	);
#endif
}

vulkan_instance::~vulkan_instance()
{
	devices.clear();
	surfaces.clear();
	if (debugMessenger)
		vulkanInstance.destroyDebugUtilsMessengerEXT(debugMessenger);

	vulkanInstance.destroy();
}

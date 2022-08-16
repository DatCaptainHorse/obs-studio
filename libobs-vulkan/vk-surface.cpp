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

vulkan_surface::vulkan_surface(vulkan_instance *_instance, const gs_init_data *data)
	: instance(_instance), width(data->cx), height(data->cy)
{
	const auto inst = instance->vulkanInstance;
#ifdef VK_USE_PLATFORM_WIN32_KHR
	blog(LOG_INFO, "Creating Vulkan surface: WIN32");
	const vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo(
		{}, GetModuleHandle(nullptr),
		static_cast<HWND>(data->window.hwnd));

	surfaceKHR = inst.createWin32SurfaceKHR(surfaceCreateInfo);
#endif
}

vulkan_surface::~vulkan_surface()
{
	const auto inst = instance->vulkanInstance;
	inst.destroySurfaceKHR(surfaceKHR);
}
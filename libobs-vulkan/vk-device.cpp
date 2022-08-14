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

gs_device::gs_device(const vk::PhysicalDevice &physicalDevice)
{
	const auto &properties = physicalDevice.getProperties();
	deviceName = const_cast<char *>(properties.deviceName.data());
	vendorID = properties.vendorID;
}

gs_device::~gs_device()
{
	/* TODO: Implement */
}
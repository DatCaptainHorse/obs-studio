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

void createSwapchain(gs_swapchain_t *sc)
{
	const auto physicalDevice = sc->device->GetPhysicalDevice();
	const auto logicalDevice = sc->device->GetLogicalDevice();

	const auto surfaceCapabilities =
		physicalDevice.getSurfaceCapabilitiesKHR(
			sc->surface->surfaceKHR);

	if (surfaceCapabilities.currentExtent.width ==
		    std::numeric_limits<uint32_t>::max() ||
	    surfaceCapabilities.currentExtent.height ==
		    std::numeric_limits<uint32_t>::max()) {
		sc->extent.width = std::max(
			surfaceCapabilities.minImageExtent.width,
			std::min(surfaceCapabilities.maxImageExtent.width,
				 sc->initData->cx));

		sc->extent.height = std::max(
			surfaceCapabilities.minImageExtent.height,
			std::min(surfaceCapabilities.maxImageExtent.height,
				 sc->initData->cy));
	} else
		sc->extent = surfaceCapabilities.currentExtent;

	const auto surfaceFormats =
		physicalDevice.getSurfaceFormatsKHR(sc->surface->surfaceKHR);

	if (surfaceFormats.empty())
		throw std::runtime_error("Could not get surface formats");

	/* TODO: Get wanted color space */
	if (surfaceFormats.size() == 1 &&
	    surfaceFormats[0].format == vk::Format::eUndefined) {
		sc->format = vk::Format::eB8G8R8A8Unorm;
		sc->colorSpaceKHR = vk::ColorSpaceKHR ::eSrgbNonlinear;
	} else {
		for (const auto &surfaceFormat : surfaceFormats) {
			/*if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb) {
				sc->format = surfaceFormat.format;
				sc->colorSpaceKHR = surfaceFormat.colorSpace;
				break;
			} else*/ if (surfaceFormat.format ==
				   vk::Format::eB8G8R8A8Unorm) {
				sc->format = surfaceFormat.format;
				sc->colorSpaceKHR = surfaceFormat.colorSpace;
				break;
			}
		}
	}

	/* present mode, mailbox is preferred */
	const auto presentModes = physicalDevice.getSurfacePresentModesKHR(
		sc->surface->surfaceKHR);

	for (const auto &presentMode : presentModes) {
		if (presentMode == vk::PresentModeKHR::eMailbox) {
			sc->presentModeKHR = presentMode;
			break;
		} else if (presentMode == vk::PresentModeKHR::eFifo) {
			sc->presentModeKHR = presentMode;
		}
	}

	const auto preTransform =
		(surfaceCapabilities.supportedTransforms &
		 vk::SurfaceTransformFlagBitsKHR::eIdentity)
			? vk::SurfaceTransformFlagBitsKHR::eIdentity
			: surfaceCapabilities.currentTransform;

	const auto compositeAlpha =
		(surfaceCapabilities.supportedCompositeAlpha &
		 vk::CompositeAlphaFlagBitsKHR::ePreMultiplied)
			? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
		: (surfaceCapabilities.supportedCompositeAlpha &
		   vk::CompositeAlphaFlagBitsKHR::ePostMultiplied)
			? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
		: (surfaceCapabilities.supportedCompositeAlpha &
		   vk::CompositeAlphaFlagBitsKHR::eInherit)
			? vk::CompositeAlphaFlagBitsKHR::eInherit
			: vk::CompositeAlphaFlagBitsKHR::eOpaque;

	if (!(surfaceCapabilities.supportedUsageFlags &
	      vk::ImageUsageFlagBits::eTransferDst))
		throw std::runtime_error(
			"Swapchain eTransferDst not supported");

	const std::array queueFamilyIndices = {sc->usedFamilyIndex};

	sc->imageCount = surfaceCapabilities.minImageCount;

	const auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR(
		{}, sc->surface->surfaceKHR, sc->imageCount, sc->format,
		sc->colorSpaceKHR, sc->extent, 1,
		vk::ImageUsageFlagBits::eColorAttachment |
			vk::ImageUsageFlagBits::eTransferDst,
		vk::SharingMode::eExclusive, 1, queueFamilyIndices.data(),
		preTransform, compositeAlpha, sc->presentModeKHR, true,
		nullptr);

	try {
		sc->swapchainKHR =
			logicalDevice.createSwapchainKHR(swapchainCreateInfo);
	} catch (const vk::Error &error) {
		throw std::runtime_error(error.what());
	}

	const auto images =
		logicalDevice.getSwapchainImagesKHR(sc->swapchainKHR);

	/* color resources */
	for (auto &image : images) {
		try {
			vk_image colorImage;
			colorImage.image = image;
			colorImage.imageView = sc->device->CreateImageView(
				colorImage.image, sc->format,
				vk::ImageAspectFlagBits::eColor);

			sc->colorImages.emplace_back(colorImage);
		} catch (const std::runtime_error &error) {
			throw error;
		}
	}

	/* depth resources */
	for (uint32_t i = 0; i < images.size(); i++) {
		try {
			vk_image depthImage;
			const auto depthFormat =
				sc->device->FindSupportedFormat(
					{vk::Format::eD32Sfloat,
					 vk::Format::eD32SfloatS8Uint,
					 vk::Format::eD24UnormS8Uint},
					vk::ImageTiling::eOptimal,
					vk::FormatFeatureFlagBits::
						eDepthStencilAttachment);

			std::tie(depthImage.image, depthImage.deviceMemory) = sc->device->CreateImage(
				sc->extent.width, sc->extent.height,
				depthFormat, vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eDepthStencilAttachment,
				vk::MemoryPropertyFlagBits::eDeviceLocal);

			depthImage.imageView = sc->device->CreateImageView(
				depthImage.image,
				depthFormat, vk::ImageAspectFlagBits::eDepth);

			vk_transitionImageLayout(
				sc->device, depthImage.image, depthFormat,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eDepthStencilAttachmentOptimal);

			sc->depthImages.emplace_back(depthImage);
		} catch (const std::runtime_error &error) {
			throw error;
		}
	}
}

void cleanSwapchain(gs_swapchain_t *swapchain)
{
	const auto logicalDevice = swapchain->device->GetLogicalDevice();

	for (auto &depthImage : swapchain->depthImages) {
		logicalDevice.destroyImage(depthImage.image);
		logicalDevice.destroyImageView(depthImage.imageView);
		logicalDevice.freeMemory(depthImage.deviceMemory);
	}

	swapchain->depthImages.clear();

	for (auto &colorImage : swapchain->colorImages)
		logicalDevice.destroyImageView(colorImage.imageView);

	swapchain->colorImages.clear();

	logicalDevice.destroySwapchainKHR(swapchain->swapchainKHR);
}

gs_swap_chain::gs_swap_chain(gs_device_t *_device, const gs_init_data *data,
			     std::unique_ptr<vulkan_surface> _surface,
			     uint32_t queueFamilyIndex)
	: vk_object(_device, vk_type::VK_SWAPCHAIN),
	  surface(std::move(_surface)),
	  usedFamilyIndex(queueFamilyIndex)
{
	initData = std::make_unique<gs_init_data>();
	initData->cx = data->cx;
	initData->cy = data->cy;
	initData->format = data->format;
	initData->window = data->window;
	initData->adapter = data->adapter;
	initData->zsformat = data->zsformat;
	initData->num_backbuffers = data->num_backbuffers;
	createSwapchain(this);
}

void gs_swap_chain::Recreate(uint32_t cx, uint32_t cy)
{
	if (cx == initData->cx && cy == initData->cy)
		return;

	initData->cx = cx;
	initData->cy = cy;
	cleanSwapchain(this);
	createSwapchain(this);
}

void gs_swap_chain::Recreate()
{
	Recreate(initData->cx, initData->cy);
}

gs_swap_chain::~gs_swap_chain()
{
	cleanSwapchain(this);
}

gs_swapchain_t *device_swapchain_create(gs_device_t *device,
					const struct gs_init_data *data)
{
	gs_swap_chain *swapchain = nullptr;

	try {
		swapchain = device->CreateSwapchain(data);
	} catch (const std::runtime_error &e) {
		blog(LOG_ERROR, "device_swapchain_create (Vulkan): %s",
		     e.what());
		return nullptr;
	}

	return swapchain;
}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swapchain)
{
	int32_t scIdx = device->GetLoadedSwapchainIdx(swapchain->initData.get());
	if (scIdx != -1)
		device->currentSwapchain = scIdx;
}

void device_get_size(const gs_device_t *device, uint32_t *cx, uint32_t *cy)
{
	if (device->currentSwapchain != -1) {
		const auto &sc = device->loadedSwapchains[device->currentSwapchain];
		cx = &sc->extent.width;
		cy = &sc->extent.height;
	} else {
		*cx = 0;
		*cy = 0;
	}
}

uint32_t device_get_width(const gs_device_t *device)
{
	if (device->currentSwapchain != -1)
		return device->loadedSwapchains[device->currentSwapchain]->extent.width;

	return 0;
}

uint32_t device_get_height(const gs_device_t *device)
{
	if (device->currentSwapchain != -1)
		return device->loadedSwapchains[device->currentSwapchain]->extent.height;

	return 0;
}

void gs_swapchain_destroy(gs_swapchain_t *swapchain)
{
	swapchain->markedForDeletion = true;
}
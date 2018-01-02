#pragma once

#include <algorithm>

#include "VulkanFormatsHandler.hpp"
#include "VulkanHandler.hpp"

namespace magma
{
  class Surface
  {
  public:
    /// TODO: fix imperfection :( -- later, is this an imprefection?
    Instance &instance;
    vk::SurfaceKHR vkSurface;

    Surface(Instance &instance, vk::SurfaceKHR vkSurface)
      : instance(instance)
      , vkSurface(vkSurface)
    {
    }

    ~Surface()
    {
      instance.vkInstance.destroySurfaceKHR(vkSurface);
    }

    bool isQueueFamilySuitable(vk::PhysicalDevice physicalDevice, uint32_t queueIndex)
    {
      return physicalDevice.getSurfaceSupportKHR(queueIndex, vkSurface);
    }
  };

  class SwapChain
  {
  public:
    Device &device;
    vk::SwapchainKHR vkSwapchain;

    template<class CONTAINER>
    static auto chooseImageFormat(Surface &surface, vk::PhysicalDevice physicalDevice, CONTAINER &&formatChoices)
    {
      auto formats(physicalDevice.getSurfaceFormatsKHR(surface.vkSurface));

      if (formats[0].format == vk::Format::eUndefined)
	{
	  formats[0].format = vk::Format::eR8G8B8Srgb; // yay!
	  return formats[0];
	}
      FormatGroup possibillities = ([&formatChoices](FormatGroup possibillities) {
	  for (auto const &pick : formatChoices)
	    {
	      if (auto best = (possibillities & pick))
		return best;
	    }
	  throw std::runtime_error("Vulkan Swapchain: No suitable format found.");
	})(FormatGroup{containerView(formats.begin(), formats.end(), [](auto const &format)
				     {
				       return format.format;
				     })});
      return *std::find_if(formats.begin(), formats.end(), [&possibillities](auto const &format)
			   {
			     return possibillities[format.format];
			   });
    }

    // TODO: refactor the fuck out of this.
    SwapChain(Surface &surface, Device &device, vk::PhysicalDevice physicalDevice)
      : device(device)
    {
      constexpr auto preferedFormatRanking =
	{
	  ((vulkanFormatGroups::R8G8B8 | vulkanFormatGroups::B8G8R8) & vulkanFormatGroups::Srgb),
	  ((vulkanFormatGroups::R8G8B8A8 | vulkanFormatGroups::B8G8R8A8) & vulkanFormatGroups::Srgb),
	  (vulkanFormatGroups::R8G8B8A8 | vulkanFormatGroups::B8G8R8A8 | vulkanFormatGroups::R8G8B8A8 | vulkanFormatGroups::B8G8R8A8)
	};
      auto format(chooseImageFormat(surface, physicalDevice, preferedFormatRanking));      
      auto capabilities(physicalDevice.getSurfaceCapabilitiesKHR(surface.vkSurface));
      auto presentModes(physicalDevice.getSurfacePresentModesKHR(surface.vkSurface));

      constexpr auto order = {vk::PresentModeKHR::eMailbox,
			      vk::PresentModeKHR::eFifoRelaxed,
			      vk::PresentModeKHR::eFifo};
      auto resultIt(std::find_if(order.begin(), order.end(), [&presentModes](auto presentMode)
				 {
				   return (std::find(presentModes.begin(), presentModes.end(), presentMode) != presentModes.end());
				 }));
      if (resultIt == order.end())
	throw std::runtime_error("Vulkan Swapchain: No suitable presentation mode found.");
      auto presentMode(*resultIt);

      if (!(capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst))
	throw std::runtime_error("Vulkan Swapchain: Mising eTransferDst for clear operation.");

      vk::SwapchainCreateInfoKHR createInfo({},
					    surface.vkSurface,
					    std::min(std::max((std::size_t)capabilities.minImageCount, 3ul), (std::size_t)capabilities.maxImageCount - (std::size_t)!capabilities.maxImageCount),
					    format.format,
					    format.colorSpace,
					    (capabilities.currentExtent == vk::Extent2D{0xFFFFFFFFu, 0xFFFFFFFFu} ? capabilities.maxImageExtent : capabilities.currentExtent), // choose current or biggest extent possible
					    1,
					    vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment,
					    vk::SharingMode::eExclusive, // nest 2 params unused because eExclusive
					    0, // unused
					    nullptr, // unused
					    capabilities.currentTransform,
					    vk::CompositeAlphaFlagBitsKHR::eOpaque,
					    presentMode,
					    true,
					    nullptr); // old swapchain

      vkSwapchain = device.createSwapchainKHR(createInfo);
    }

    auto getImageIndex(vk::Semaphore sem, vk::Fence fence)
    {
      return device.acquireNextImageKHR(vkSwapchain, 20, sem, fence);
    }

    ~SwapChain()
    {
      device.destroySwapchainKHR(vkSwapchain);
    }
  };
};

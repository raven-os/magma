#pragma once

#include "magma/Device.hpp"

namespace magma
{
  struct DeviceMemoryDeleter
  {
    Device<claws::no_delete> device;

    void operator()(vk::DeviceMemory const &fence) const
    {
      if (device)
        device.freeMemory(fence);
    }
  };

  template<class Deleter = DeviceMemoryDeleter>
  using DeviceMemory = claws::handle<vk::DeviceMemory, Deleter>;

  inline auto impl::Device::createDeviceMemory(vk::DeviceSize size, uint32_t typeIndex) const
  {
    return DeviceMemory<>(DeviceMemoryDeleter{magma::Device<claws::no_delete>(*this)}, vk::Device::allocateMemory({size, typeIndex}));
  }

  inline auto selectDeviceMemoryType(vk::PhysicalDevice physicalDevice,
				     vk::DeviceSize size,
				     vk::MemoryPropertyFlags memoryFlags,
				     uint32_t memoryTypeIndexMask)
  {
    auto const memProperties(physicalDevice.getMemoryProperties());

    for (uint32_t i(0u); i < memProperties.memoryTypeCount; ++i)
      {
        auto const &type(memProperties.memoryTypes[i]);

        if (((memoryTypeIndexMask >> i) & 1u) && (type.propertyFlags & memoryFlags) && memProperties.memoryHeaps[type.heapIndex].size >= size)
          return i;
      }
    throw std::runtime_error("Couldn't find proper memory type");
  }

  inline auto impl::Device::selectAndCreateDeviceMemory(vk::PhysicalDevice physicalDevice,
                                                        vk::DeviceSize size,
                                                        vk::MemoryPropertyFlags memoryFlags,
                                                        uint32_t memoryTypeIndexMask) const
  {
    return createDeviceMemory(size, selectDeviceMemoryType(physicalDevice, size, memoryFlags, memoryTypeIndexMask));
  }
};

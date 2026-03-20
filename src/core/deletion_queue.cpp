module;
#include <cstdint>
#include <vulkan/vulkan_core.h>

module core:deletion_queue;
import std;

namespace Magma {

export class DeletionQueue {
public:
  static void push(std::function<void(VkDevice)> &&function){
    queues[currentSlot()].emplace_back(std::move(function));
  }

  static void flushForFrame(uint32_t frameIndex){
    VkDevice device = Device::get().device();
    auto &q = queues[frameIndex];
    for (auto &fn: q) fn(device);
    q.clear();
  }

  static void flushAll(){
    for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++)
      flushForFrame(i);
  }

private:
  static uint32_t currentSlot(){
    return static_cast<uint32_t>(FrameInfo::frameIndex);
  }
  inline static std::array<std::vector<std::function<void(VkDevice)>>, SwapChain::MAX_FRAMES_IN_FLIGHT> queues;
};

}


module;
#include <vulkan/vulkan_core.h>
#include <cassert>

module core:descriptors;
import std;

namespace Magma {

export class DescriptorSetLayout {
public:
  class Builder {
  public:
    Builder &addBinding(
        uint32_t binding, VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags, uint32_t count) {
      VkDescriptorSetLayoutBinding layoutBinding{};
      layoutBinding.binding = binding;
      layoutBinding.stageFlags = stageFlags;
      layoutBinding.descriptorType = descriptorType;
      layoutBinding.descriptorCount = count;
      bindings[binding] = layoutBinding;
      return *this;
    }

    std::unique_ptr<DescriptorSetLayout> build() const {
      return std::make_unique<DescriptorSetLayout>(bindings);
    }

  private:
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
  };

  DescriptorSetLayout(
      std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
      : bindings{bindings} {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    for (const auto &binding : bindings)
      layoutBindings.push_back(binding.second);

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.pBindings = layoutBindings.data();
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());

    VkDevice device = Device::get().device();
    if (vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr,
                                    &descriptorSetLayout) != VK_SUCCESS)
      throw std::runtime_error("Failed to create descriptor set layout!");
  }

  ~DescriptorSetLayout() {
    VkDevice device = Device::get().device();
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  }

  VkDescriptorSetLayout getDescriptorSetLayout() const {
    return descriptorSetLayout;
  }

private:
  VkDescriptorSetLayout descriptorSetLayout;
  std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};

  friend class DescriptorWriter;
};

export class DescriptorPool {
public:
  class Builder {
  public:
    Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count) {
      poolSizes.push_back({descriptorType, count});
      return *this;
    }
    Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags) {
      poolFlags = flags;
      return *this;
    }
    Builder &setMaxSets(uint32_t count) {
      maxSets = count;
      return *this;
    }
    std::unique_ptr<DescriptorPool> ::build() const {
      return std::make_unique<DescriptorPool>(maxSets, poolFlags,
                                              poolSizes);
    }

  private:
    std::vector<VkDescriptorPoolSize> poolSizes{};
    VkDescriptorPoolCreateFlags poolFlags{};
    uint32_t maxSets;
  };

  DescriptorPool(
      uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,
      const std::vector<VkDescriptorPoolSize> &poolSizes){
    VkDescriptorPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    createInfo.pPoolSizes = poolSizes.data();
    createInfo.maxSets = maxSets;
    createInfo.flags = poolFlags;

    VkDevice device = Device::get().device();
    if(vkCreateDescriptorPool(device, &createInfo, nullptr, &descriptorPool) != VK_SUCCESS) 
      throw std::runtime_error("Failed to create descriptor pool!");
  }

  ~DescriptorPool() {
    VkDevice device = Device::get().device();
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    descriptorPool = nullptr;
  }

  bool allocateDescriptor(VkDescriptorSetLayout setLayout,
                                          VkDescriptorSet &set) {
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.pSetLayouts = &setLayout;
    allocInfo.descriptorSetCount = 1;

    VkDevice device = Device::get().device();
    if (vkAllocateDescriptorSets(device, &allocInfo, &set) != VK_SUCCESS) {
      return false;
    } else {
      return true;
    }
  }

  VkDescriptorPool getDescriptorPool() const {
    if (descriptorPool == VK_NULL_HANDLE)
      throw std::runtime_error("Descriptor Pool is not created!");
    return descriptorPool;
  }

private:
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

  friend class DescriptorWriter;
};

class DescriptorWriter {
public:
  DescriptorWriter(DescriptorSetLayout &setLayout, DescriptorPool &pool)
      : setLayout{setLayout}, pool{pool} {};
  DescriptorWriter &writeBuffer(
      uint32_t binding, VkDescriptorBufferInfo *bufferInfo,
      VkDescriptorType descriptorType, VkDescriptorImageInfo *imageInfo) {
    assert(setLayout.bindings.count(binding) == 1 &&
           "Layout does not cotain specified binding");

    auto &bindingDescription = setLayout.bindings[binding];

    assert(bindingDescription.descriptorCount == 1 &&
           "Binding single descriptor info, but "
           "multiple expexcted");

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = descriptorType;
    write.dstBinding = binding;
    write.pBufferInfo = bufferInfo;
    write.descriptorCount = 1;
    write.pImageInfo = imageInfo;

    writes.push_back(write);

    return *this;
  }
  DescriptorWriter &writeImage(uint32_t binding,
                               VkDescriptorImageInfo *imageInfo) {
    assert(setLayout.bindings.count(binding) == 1 &&
           "Layout does not cotain specified binding");

    auto &bindingDescription = setLayout.bindings[binding];

    assert(bindingDescription.descriptorCount == 1 &&
           "Binding single descriptor info, but "
           "multiple expexcted");

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.dstBinding = binding;
    write.pImageInfo = imageInfo;
    write.descriptorCount = 1;
    write.dstArrayElement = 0;

    writes.push_back(write);

    return *this;
  }

  bool build(VkDescriptorSet &set) {
    bool success =
        pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
    if (!success) {
      return false;
    } else {
      overwrite(set);
      return true;
    }
  }

  void overwrite(VkDescriptorSet &set) {
    for (auto &write : writes) {
      write.dstSet = set;
    }

    VkDevice device = Device::get().device();
    vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
  }

private:
  DescriptorSetLayout &setLayout;
  DescriptorPool &pool;
  std::vector<VkWriteDescriptorSet> writes;
};

} // namespace Magma

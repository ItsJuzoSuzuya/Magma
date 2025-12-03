#include "descriptors.hpp"
#include "device.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <sys/types.h>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace Magma {

//                      Descriptor Set Layout Builder                         //

DescriptorSetLayout::Builder &DescriptorSetLayout::Builder::addBinding(
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

std::unique_ptr<DescriptorSetLayout>
DescriptorSetLayout::Builder::build() const {
  return std::make_unique<DescriptorSetLayout>(bindings);
}

//                      Descriptor Set Layout                                 //

DescriptorSetLayout::DescriptorSetLayout(
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

DescriptorSetLayout::~DescriptorSetLayout() {
  VkDevice device = Device::get().device();
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

//                      Descriptor Pool Builder                             //

DescriptorPool::Builder &
DescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType,
                                     uint32_t count) {
  poolSizes.push_back({descriptorType, count});
  return *this;
}

DescriptorPool::Builder &DescriptorPool::Builder::setMaxSets(uint32_t count) {
  maxSets = count;
  return *this;
}

DescriptorPool::Builder &
DescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
  poolFlags = flags;
  return *this;
}

std::unique_ptr<DescriptorPool> DescriptorPool::Builder::build() const {
  return std::make_unique<DescriptorPool>(maxSets, poolFlags,
                                          poolSizes);
}

//                      Descriptor Pool                                       //

DescriptorPool::DescriptorPool(
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

DescriptorPool::~DescriptorPool() {
  VkDevice device = Device::get().device();
  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  descriptorPool = nullptr;
}

bool DescriptorPool::allocateDescriptor(VkDescriptorSetLayout setLayout,
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

//                      Descriptor Writer                               //

DescriptorWriter &DescriptorWriter::writeBuffer(
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

DescriptorWriter &
DescriptorWriter::writeImage(uint32_t binding,
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

bool DescriptorWriter::build(VkDescriptorSet &set) {
  bool success =
      pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
  if (!success) {
    return false;
  } else {
    overwrite(set);
    return true;
  }
}

void DescriptorWriter::overwrite(VkDescriptorSet &set) {
  for (auto &write : writes) {
    write.dstSet = set;
  }

  VkDevice device = Device::get().device();
  vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
}

} // namespace Magma

// namespace engine

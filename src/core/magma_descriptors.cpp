#include "magma_descriptors.hpp"

// std
#include <cassert>
#include <stdexcept>

namespace magma {

// *************** Descriptor Set Layout Builder *********************

MagmaDescriptorSetLayout::Builder &
MagmaDescriptorSetLayout::Builder::addBinding(uint32_t binding,
                                              VkDescriptorType descriptorType,
                                              VkShaderStageFlags stageFlags,
                                              uint32_t count) {
  assert(bindings.count(binding) == 0 && "Binding already in use");
  VkDescriptorSetLayoutBinding layoutBinding{};
  layoutBinding.binding = binding;
  layoutBinding.descriptorType = descriptorType;
  layoutBinding.descriptorCount = count;
  layoutBinding.stageFlags = stageFlags;
  bindings[binding] = layoutBinding;
  return *this;
}

std::unique_ptr<MagmaDescriptorSetLayout>
MagmaDescriptorSetLayout::Builder::build() const {
  return std::make_unique<MagmaDescriptorSetLayout>(magmaDevice, bindings);
}

// *************** Descriptor Set Layout *********************

MagmaDescriptorSetLayout::MagmaDescriptorSetLayout(
    MagmaDevice &magmaDevice,
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
    : magmaDevice{magmaDevice}, bindings{bindings} {
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
  for (auto kv : bindings) {
    setLayoutBindings.push_back(kv.second);
  }

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
  descriptorSetLayoutInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutInfo.bindingCount =
      static_cast<uint32_t>(setLayoutBindings.size());
  descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

  if (vkCreateDescriptorSetLayout(magmaDevice.device(),
                                  &descriptorSetLayoutInfo, nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

MagmaDescriptorSetLayout::~MagmaDescriptorSetLayout() {
  vkDestroyDescriptorSetLayout(magmaDevice.device(), descriptorSetLayout,
                               nullptr);
}

// *************** Descriptor Pool Builder *********************

MagmaDescriptorPool::Builder &
MagmaDescriptorPool::Builder::addPoolSize(VkDescriptorType descriptorType,
                                          uint32_t count) {
  poolSizes.push_back({descriptorType, count});
  return *this;
}

MagmaDescriptorPool::Builder &
MagmaDescriptorPool::Builder::setPoolFlags(VkDescriptorPoolCreateFlags flags) {
  poolFlags = flags;
  return *this;
}
MagmaDescriptorPool::Builder &
MagmaDescriptorPool::Builder::setMaxSets(uint32_t count) {
  maxSets = count;
  return *this;
}

std::unique_ptr<MagmaDescriptorPool>
MagmaDescriptorPool::Builder::build() const {
  return std::make_unique<MagmaDescriptorPool>(magmaDevice, maxSets, poolFlags,
                                               poolSizes);
}

// *************** Descriptor Pool *********************

MagmaDescriptorPool::MagmaDescriptorPool(
    MagmaDevice &magmaDevice, uint32_t maxSets,
    VkDescriptorPoolCreateFlags poolFlags,
    const std::vector<VkDescriptorPoolSize> &poolSizes)
    : magmaDevice{magmaDevice} {
  VkDescriptorPoolCreateInfo descriptorPoolInfo{};
  descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  descriptorPoolInfo.pPoolSizes = poolSizes.data();
  descriptorPoolInfo.maxSets = maxSets;
  descriptorPoolInfo.flags = poolFlags;

  if (vkCreateDescriptorPool(magmaDevice.device(), &descriptorPoolInfo, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

MagmaDescriptorPool::~MagmaDescriptorPool() {
  vkDestroyDescriptorPool(magmaDevice.device(), descriptorPool, nullptr);
}

bool MagmaDescriptorPool::allocateDescriptor(
    const VkDescriptorSetLayout descriptorSetLayout,
    VkDescriptorSet &descriptor) const {
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.pSetLayouts = &descriptorSetLayout;
  allocInfo.descriptorSetCount = 1;

  // Might want to create a "DescriptorPoolManager" class that handles this
  // case, and builds a new pool whenever an old pool fills up. But this is
  // beyond our current scope
  if (vkAllocateDescriptorSets(magmaDevice.device(), &allocInfo, &descriptor) !=
      VK_SUCCESS) {
    return false;
  }
  return true;
}

void MagmaDescriptorPool::freeDescriptors(
    std::vector<VkDescriptorSet> &descriptors) const {
  vkFreeDescriptorSets(magmaDevice.device(), descriptorPool,
                       static_cast<uint32_t>(descriptors.size()),
                       descriptors.data());
}

void MagmaDescriptorPool::resetPool() {
  vkResetDescriptorPool(magmaDevice.device(), descriptorPool, 0);
}

// *************** Descriptor Writer *********************

MagmaDescriptorWriter::MagmaDescriptorWriter(
    MagmaDescriptorSetLayout &setLayout, MagmaDescriptorPool &pool)
    : setLayout{setLayout}, pool{pool} {}

MagmaDescriptorWriter &
MagmaDescriptorWriter::writeBuffer(uint32_t binding,
                                   VkDescriptorBufferInfo *bufferInfo) {
  assert(setLayout.bindings.count(binding) == 1 &&
         "Layout does not contain specified binding");

  auto &bindingDescription = setLayout.bindings[binding];

  assert(bindingDescription.descriptorCount == 1 &&
         "Binding single descriptor info, but binding expects multiple");

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorType = bindingDescription.descriptorType;
  write.dstBinding = binding;
  write.pBufferInfo = bufferInfo;
  write.descriptorCount = 1;

  writes.push_back(write);
  return *this;
}

MagmaDescriptorWriter &
MagmaDescriptorWriter::writeImage(uint32_t binding,
                                  VkDescriptorImageInfo *imageInfo) {
  assert(setLayout.bindings.count(binding) == 1 &&
         "Layout does not contain specified binding");

  auto &bindingDescription = setLayout.bindings[binding];

  assert(bindingDescription.descriptorCount == 1 &&
         "Binding single descriptor info, but binding expects multiple");

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.descriptorType = bindingDescription.descriptorType;
  write.dstBinding = binding;
  write.pImageInfo = imageInfo;
  write.descriptorCount = 1;

  writes.push_back(write);
  return *this;
}

bool MagmaDescriptorWriter::build(VkDescriptorSet &set) {
  bool success =
      pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
  if (!success) {
    return false;
  }
  overwrite(set);
  return true;
}

void MagmaDescriptorWriter::overwrite(VkDescriptorSet &set) {
  for (auto &write : writes) {
    write.dstSet = set;
  }
  vkUpdateDescriptorSets(pool.magmaDevice.device(), writes.size(),
                         writes.data(), 0, nullptr);
}

} // namespace magma

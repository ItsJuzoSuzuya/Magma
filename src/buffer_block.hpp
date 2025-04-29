#include <cstddef>
#include <cstdint>
#include <ostream>
namespace magma {

struct BlockSize {
  static constexpr size_t Tiny = 1024;
  static constexpr size_t Small = 2048;
  static constexpr size_t Medium = 4096;
  static constexpr size_t Large = 8192;
  static constexpr size_t Huge = 16384;
  static constexpr size_t Max = 24576;
};

struct BufferBlock {
  uint32_t drawCallIndex;
  uint32_t vertexOffset;
  size_t vertexCount;
  uint32_t firstIndex;
  size_t indexCount;
  uint32_t vertexBufferOffset;
  uint32_t indexBufferOffset;
  size_t vertexBlockSize;
  size_t indexBlockSize;
};

inline std::ostream &operator<<(std::ostream &os,
                                const BufferBlock &bufferBlock) {
  os << "BufferBlock { "
     << "drawCallIndex: " << bufferBlock.drawCallIndex
     << ", vertexOffset: " << bufferBlock.vertexOffset
     << ", vertexCount: " << bufferBlock.vertexCount
     << ", indexOffset: " << bufferBlock.firstIndex
     << ", indexCount: " << bufferBlock.indexCount << " }";
  return os;
}
} // namespace magma

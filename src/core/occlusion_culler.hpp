#include "../collision.hpp"
#include "camera.hpp"
#include "game_object.hpp"

#include <array>
#include <memory>
#include <vector>

namespace magma {

class Octree;

class OctreeNode {
public:
  OctreeNode(const BoxCollider boundingBox, int depth);

  BoxCollider boundingBox;
  std::array<std::unique_ptr<OctreeNode>, 8> children;

  friend class Octree;
  friend class OcclusionCuller3D;

private:
  void print(const Octree *octree);
};

class Octree {
public:
  Octree();
  Octree(const BoxCollider rootBoundingBox, int depth);

  std::unique_ptr<OctreeNode> root;
  bool isOccluded = true;

  void print();
}; // namespace engine

class OcclusionCuller3D {
public:
  OcclusionCuller3D(Camera &camera, std::vector<float> &depthBuffer);
  void generateOctree(const BoxCollider root, int depth);
  bool checkAgainstDepthBuffer();
  void checkOcclusion(const std::unique_ptr<OctreeNode> &node);

private:
  Octree octree;
  Camera &camera;
  std::vector<float> &depthData;
};
} // namespace magma

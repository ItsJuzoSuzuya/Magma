#include "occlusion_culler.hpp"
#include "../magma.hpp"
#include "camera.hpp"
#include "game_object.hpp"
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <ostream>

namespace magma {

Octree::Octree() { root = nullptr; }

Octree::Octree(const BoxCollider boundingBox, int depth) {
  root = std::make_unique<OctreeNode>(boundingBox, depth);
}

void Octree::print() { root->print(this); }

OctreeNode::OctreeNode(const BoxCollider boundingBox, int depth)
    : boundingBox{boundingBox} {
  if (depth == 0)
    return;

  glm::vec3 min = boundingBox.collisionBox.min;
  glm::vec3 center = boundingBox.collisionBox.center();
  glm::vec3 max = boundingBox.collisionBox.max;

  BoxCollider childBox1 = BoxCollider(min, center);
  children[0] = std::make_unique<OctreeNode>(childBox1, depth - 1);

  glm::vec3 bottomLeftCenter(min.x, min.y, center.z);
  glm::vec3 backCenter(center.x, center.y, max.z);
  BoxCollider childBox2 = BoxCollider(bottomLeftCenter, backCenter);
  children[1] = std::make_unique<OctreeNode>(childBox2, depth - 1);

  glm::vec3 bottomFrontCenter(center.x, min.y, min.z);
  glm::vec3 rightCenter(max.x, center.y, center.z);
  BoxCollider childBox3 = BoxCollider(bottomFrontCenter, rightCenter);
  children[2] = std::make_unique<OctreeNode>(childBox3, depth - 1);

  glm::vec3 bottomCenter(center.x, min.y, center.z);
  glm::vec3 backRightCenter(max.x, center.y, max.z);
  BoxCollider childBox4 = BoxCollider(bottomCenter, backRightCenter);
  children[3] = std::make_unique<OctreeNode>(childBox4, depth - 1);

  glm::vec3 frontLeftCenter(min.x, center.y, min.z);
  glm::vec3 topCenter(center.x, max.y, center.z);
  BoxCollider childBox5 = BoxCollider(frontLeftCenter, topCenter);
  children[4] = std::make_unique<OctreeNode>(childBox5, depth - 1);

  glm::vec3 leftCenter(min.x, center.y, center.z);
  glm::vec3 topBackCenter(center.x, max.y, max.z);
  BoxCollider childBox6 = BoxCollider(leftCenter, topBackCenter);
  children[5] = std::make_unique<OctreeNode>(childBox6, depth - 1);

  glm::vec3 frontCenter(center.x, center.y, min.z);
  glm::vec3 topRightCenter(max.x, max.y, center.z);
  BoxCollider childBox7 = BoxCollider(frontCenter, topRightCenter);
  children[6] = std::make_unique<OctreeNode>(childBox7, depth - 1);

  BoxCollider childBox8 = BoxCollider(center, max);
  children[7] = std::make_unique<OctreeNode>(childBox8, depth - 1);
}

void OctreeNode::print(const Octree *octree) {
  std::cout << "Octree: " << octree << std::endl;
  std::cout << "OctreeNode: " << this << std::endl;
  std::cout << "   Bounding Box: " << &boundingBox << std::endl;
  std::cout << "      Min: " << boundingBox.collisionBox.min.x << " "
            << boundingBox.collisionBox.min.y << " "
            << boundingBox.collisionBox.min.z << std::endl;
  std::cout << "      Max: " << boundingBox.collisionBox.max.x << " "
            << boundingBox.collisionBox.max.y << " "
            << boundingBox.collisionBox.max.z << std::endl;

  for (std::unique_ptr<OctreeNode> &child : children)
    if (child)
      child->print(octree);
}

OcclusionCuller3D::OcclusionCuller3D(Camera &camera,
                                     std::vector<float> &depthData)
    : camera{camera}, depthData{depthData} {};

void OcclusionCuller3D::generateOctree(const BoxCollider root, int depth) {
  octree = Octree(root, depth);
}

bool OcclusionCuller3D::checkAgainstDepthBuffer() {
  if (octree.root == nullptr)
    throw std::runtime_error(
        "Octree root is null! \n Call generateOctree() first!");

  checkOcclusion(octree.root);

  return octree.isOccluded;
}

void OcclusionCuller3D::checkOcclusion(
    const std::unique_ptr<OctreeNode> &node) {
  if (node == nullptr)
    throw std::runtime_error("Node is null!");

  glm::mat4 projectionView = camera.getProjection() * camera.getView();

  for (glm::vec3 corner : node->boundingBox.collisionBox.corners()) {
    glm::vec4 worldSpacePosition = glm::vec4(corner, 1.f);
    glm::vec4 clipSpacePosition = projectionView * worldSpacePosition;
    glm::vec3 ndcPos = clipSpacePosition / clipSpacePosition.w;

    if (ndcPos.x < -1 || ndcPos.x >= 1 || ndcPos.y < -1 || ndcPos.y >= 1 ||
        ndcPos.z < 0 || ndcPos.z >= 1)
      continue;

    int screenX = ((ndcPos.x + 1) / 2) * int(WIDTH / 4.f);
    int screenY = ((ndcPos.y + 1) / 2) * int(HEIGHT / 4.f);

    int depthIndex = screenX + screenY * int(WIDTH / 4);

    if (ndcPos.z <= depthData[depthIndex] || depthData[depthIndex] == 1.f) {
      octree.isOccluded = false;
      return;
    }

    if (node->children[0]) {
      for (const std::unique_ptr<OctreeNode> &child : node->children)
        if (child != nullptr)
          checkOcclusion(child);
    }
  }
}

} // namespace magma

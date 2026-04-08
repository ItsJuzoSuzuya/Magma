module;
#include <cstdint>

export module components:component;
import core;

namespace Magma {

/**
 * Abstract class for all components.
 * Components are used to add functionality to entities.
 * @note All components must implement the Component interface.
 * @note Scripts are also considered components.
 */
export class Component {
public:
  Component(uint64_t *ownerID) : ownerID(ownerID) {}
  virtual ~Component() = default;

  // --- Lifecycle ---
  virtual void onAwake() = 0;
  virtual void onUpdate() = 0;
  virtual void collectProxy(RenderProxy& proxy) = 0;

  #if defined(MAGMA_WITH_EDITOR)
    // Inspector
    virtual void onInspector() = 0;
    virtual const char *inspectorName() const = 0;
    virtual const float inspectorHeight() const = 0;
  #endif

  uint64_t *ownerID = nullptr;
};

} // namespace Magma

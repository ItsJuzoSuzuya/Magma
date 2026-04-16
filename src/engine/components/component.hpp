#pragma once
#include "core/render_proxy.hpp"
#include <cstdint>

namespace Magma {

/**
 * Abstract class for all components.
 * Components are used to add functionality to entities.
 */
class Component {
public:
  Component() {}
  virtual ~Component() = default;

  // --- Lifecycle ---
  virtual void onAwake() {}
  virtual void onUpdate() = 0;
  virtual void collectProxy(RenderProxy &proxy) = 0;

  #if defined(MAGMA_WITH_EDITOR)
    virtual void onInspector() = 0;
    virtual const char *inspectorName() const = 0;
    virtual const float inspectorHeight() const = 0;
  #endif
};

} // namespace Magma

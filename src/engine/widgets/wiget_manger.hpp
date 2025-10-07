#pragma once

#include "dock_layout.hpp"
namespace Magma {

class WidgetManager {
public:
  WidgetManager() = default;
  ~WidgetManager() = default;

private:
  // Layout
  DockLayout dockLayout;
  bool dockBuilt = false;
};

} // namespace Magma

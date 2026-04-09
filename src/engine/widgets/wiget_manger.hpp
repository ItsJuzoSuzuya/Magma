#include "engine/widgets/widget.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <memory>
#include <vector>

namespace Magma {

class WidgetManager {
public:
  WidgetManager() = default;
  ~WidgetManager() = default;

  void addWidget(std::unique_ptr<Widget> widget);

  void dock();

  void drawWidgets();

private:
  std::vector<std::unique_ptr<Widget>> widgets = {};

  bool dockBuilt = false;
};

} // namespace Magma

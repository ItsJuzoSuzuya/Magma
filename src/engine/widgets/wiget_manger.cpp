module widgets:widget_manager;

namespace Magma {

export class WidgetManager {
public:
  WidgetManager() = default;
  ~WidgetManager() = default;

private:
  // Layout
  DockLayout dockLayout;
  bool dockBuilt = false;
};

} // namespace Magma

module widgets:file_browser;
import ImGui;

namespace Magma {

class FileBrowser: public Widget {
public:
  const char *name() const override { return "File Manager"; }

  void FileBrowser::draw() override {
    UIContext::ensureInit();

    ImGui::SetNextWindowClass(&UIContext::AppDockClass);
    ImGui::Begin(name());

    for (File &file: FileManager::getFiles(currentPath)) {
      ImGui::Selectable(file.name.c_str());


      if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        if (file.isDirectory())
          currentPath = file.path;
      }
    }

    ImGui::End();
  }

  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Down, 0.3f};
  }

private:
  std::string currentPath = std::filesystem::current_path();
};

} // namespace Magma

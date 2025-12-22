#include "file_browser.hpp"
#include "engine/systems/file_manager.hpp"
#include "engine/widgets/ui_context.hpp"
#include "imgui.h"

namespace Magma {

void FileBrowser::draw() {
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

} // namespace Magma

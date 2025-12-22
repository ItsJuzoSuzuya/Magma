#include "file_manager.hpp"
#include "engine/widgets/ui_context.hpp"
#include "imgui.h"

namespace Magma {

void FileManager::draw() {
  UIContext::ensureInit();

  ImGui::SetNextWindowClass(&UIContext::AppDockClass);
  ImGui::Begin(name());

  ImGui::Text("This is the File Manager widget.");

  ImGui::End();
}

} // namespace Magma

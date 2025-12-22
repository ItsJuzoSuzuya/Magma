#include "file_manager.hpp"
#include <filesystem>
#include <print>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
namespace Magma {

std::vector<File> FileManager::getFiles(std::string_view path){
  std::vector<File> files{};
  for (const auto &entry: fs::directory_iterator(path)){
    const std::string path = entry.path().string();

    const auto posLast = path.find_last_of("/");
    const std::string name = path.substr(posLast + 1);

    const FileType type = entry.is_directory() ? FileType::DIRECTORY :
                      (entry.path().extension() == ".hpp" ? FileType::SCRIPT_HPP :
                       (entry.path().extension() == ".cpp" ? FileType::SCRIPT_CPP :
                        FileType::SCRIPT_CPP)); 

    const File file{name, path, type};
    files.emplace_back(std::move(file));
  }

  return files;
}

}

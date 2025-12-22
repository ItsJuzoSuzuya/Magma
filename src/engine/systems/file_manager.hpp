#include <string>
#include <string_view>
#include <vector>
namespace Magma {

enum FileType {
  DIRECTORY,
  SCRIPT_HPP,
  SCRIPT_CPP,
};

struct File {
  std::string name;
  std::string path;
  FileType type;

  bool isDirectory(){ return type == DIRECTORY; }
};

/** @brief A singleton for managing files within the application.
 *
 * The FileManager class provides a user interface for browsing,
 * organizing, and manipulating files. It 
 */
class FileManager {
public:
  static std::vector<File> getFiles(std::string_view path);
};

}

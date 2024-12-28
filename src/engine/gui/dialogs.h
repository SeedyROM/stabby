#include "portable-file-dialogs.h"

#include <string>
#include <vector>

namespace ste::dialogs {

static std::vector<std::string>
openFile(std::string const &title, std::string const &defaultPath = "",
         std::vector<std::string> const &filters = {"All Files", "*"},
         pfd::opt options = pfd::opt::none) {
  auto selection = pfd::open_file(title, defaultPath, filters, options);

  return selection.result();
}

}; // namespace ste::dialogs
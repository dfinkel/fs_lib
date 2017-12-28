#include "strings.h"

namespace spin_2_fs {

std::list<std::string> SplitStrings(const std::string &in, char sep) {
  std::list<std::string> components;
  auto sep_iter = in.begin();
  for (auto it = in.begin(); it != in.end(); it++) {
    if (*it == sep) {
      components.push_back({(sep_iter), it});
      sep_iter = it + 1;
    }
  }
  if ((sep_iter) != in.end()) {
    components.push_back({(sep_iter), in.end()});
  } else {
    components.push_back(std::string());
  }
  return components;
}

} // namespace spin_2_fs

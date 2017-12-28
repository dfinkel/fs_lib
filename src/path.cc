#include "path.h"
#include "strings.h"

#include <unistd.h>

namespace spin_2_fs {
using namespace std::string_literals;

namespace {

bool IsAbsolute(const std::string &path) {
  if (path.empty()) {
    return false;
  }
  if (path.front() == '/') {
    return true;
  }
  return false;
}

bool IsDirectory(const std::string &path) {
  if (path.empty()) {
    return false;
  }
  if (path.back() == '/') {
    return true;
  }
  return false;
}

void StripEmptyPrefixes(std::list<std::string> *components, const bool absolute) {
  // remove the empty prefix in case the "absolute" path starts with two or more
  // slashes, a few "." components or ".." components (the last only for
  // absolute paths).
  while (!components->empty() && (components->front().empty() || components->front() == "." ||
                                  (absolute && components->front() == ".."))) {
    components->erase(components->begin());
  }
}
std::list<std::string> CanonicalizePathList(std::list<std::string> components, bool absolute) {
  StripEmptyPrefixes(&components, absolute);
  // Keep track of how far we are into a prefix of non-".." components so far,
  // to handle a "../.." prefix correctly in relative paths.
  // set the non-parent prefix depth to 2^40 (~10^12) if it's an absolute path
  // so we never mode-switch to preserving the ".." components. (I'll be
  // impressed the day someone has enough RAM to construct a 2TiB path string,
  // let alone constructs one and passes it into this function..)
  int64_t non_parent_prefix_depth = absolute ? 1l << 40 : 0;
  for (auto it = components.begin(); it != components.end(); it++) {
    if (!(it->empty() || *it == "." || *it == "..")) {
      non_parent_prefix_depth++;
    }
    if (*it == "." || it->empty()) {
      if (it != components.begin()) {
        components.erase(it--);
      } else {
        StripEmptyPrefixes(&components, absolute);
        it = components.begin();
      }
    }
    if (*it == ".." && (non_parent_prefix_depth > 0)) {
      auto next_component = it;
      next_component++;
      if (it != components.begin()) {
        auto prev_dir = --it;
        --it;
        components.erase(prev_dir, next_component);
        non_parent_prefix_depth--;
      } else {
        StripEmptyPrefixes(&components, absolute);
        it = components.begin();
        non_parent_prefix_depth = absolute ? 1 << 20 : 0;
      }
    }
  }

  return components;
}

std::list<std::string> CanonicalizePathList(const std::string &path) {
  std::list<std::string> components = SplitStrings(path, '/');
  const bool absolute = IsAbsolute(path);
  return CanonicalizePathList(components, absolute);
}

}  // anonymous namespace

Path::Path(const std::string &path)
    : components_(std::make_shared<std::list<std::string>>(CanonicalizePathList(path))),
      absolute_(IsAbsolute(path)),
      directory_(IsDirectory(path)),
      num_components_(components_->size()) {}

Path::Path(std::list<std::string> path, bool abs, bool dir)
    : components_(std::make_shared<std::list<std::string>>(std::move(path))),
      absolute_(abs),
      directory_(dir),
      num_components_(components_->size()) {}

Path::Path(std::shared_ptr<const std::list<std::string>> path, bool abs, bool dir,
           int64_t num_components)
    : components_(path), absolute_(abs), directory_(dir), num_components_(num_components) {}

std::string Path::to_string() const {
  std::string canonical_path;
  int64_t done_dirs = 0;
  if (num_components_ <= 0 || components_->empty()) {
    if (absolute_) {
      return "/"s;
    } else {
      return "."s;
    }
  }
  for (const auto &c : *components_) {
    canonical_path += "/"s + c;
    if (++done_dirs >= num_components_) {
      break;
    }
  }
  if (!absolute_) {
    canonical_path.erase(0, 1);
  }
  if (directory_) {
    canonical_path += "/"s;
  }

  return canonical_path;
}

std::list<std::string> Path::get_list() const {
  std::list<std::string> components = *components_;
  components.resize(num_components_);
  return components;
}

Path Path::parent() const {
  std::shared_ptr<const std::list<std::string>> components = components_;
  int64_t new_components = std::max<int64_t>(num_components_ - 1, 0);
  // special handling for the relative case where we hit the beginning.
  if (!absolute_) {
    bool all_parents = true;
    int64_t scanned_dirs = 0;
    // If there are still non-".." parents, we can continue reducing
    // num_components_
    for (const auto &dir : *components_) {
      // don't try to consume beyond num_components_.
      if (scanned_dirs++ >= num_components_) {
        break;
      }
      if (dir != "..") {
        all_parents = false;
        break;
      }
    }
    // this includes the empty case.
    if (all_parents) {
      std::list<std::string> components_l = *components_;
      components_l.push_front("..");
      components = std::make_shared<const std::list<std::string>>(std::move(components_l));
      new_components = num_components_ + 1;
    }
  }
  Path up(std::move(components), absolute_, /* dir = */ true,
          /* num_components = */ new_components);
  return up;
}

bool Path::is_root() const { return num_components_ == 0 && absolute_; }

bool Path::operator<(const Path &other) const {
  // make relative paths sort after absolute ones.
  if (other.absolute_ != absolute_) {
    if (other.absolute_ && !absolute_) {
      return false;
    }
    return true;
  }
  if (components_ == other.components_) {
    // it's the same list, compare lengths.
    if (num_components_ != other.num_components_) {
      return (num_components_ < other.num_components_);
    }
    if (directory_ != other.directory_) {
      return !directory_;
    }
  }
  auto it = components_->begin();
  auto oit = other.components_->begin();

  for (int i = 0; i < num_components_ && i < other.num_components_ && it != components_->end() &&
                  oit != other.components_->end();
       i++, it++, oit++) {
    if (*it == *oit) {
      continue;
    }
    if (*it < *oit) {
      return true;
    }
    return false;
  }

  // we hit the end of something, so we'll have to disambiguate by lengths.
  return num_components_ < other.num_components_;
}

bool Path::has_parent(const Path &path) const {
  if (absolute_ != path.absolute_) {
    return false;
  }
  if (path.components_ == components_) {
    return path.num_components_ < num_components_;
  }
  auto it = components_->begin();
  auto oit = path.components_->begin();

  for (int i = 0; i < num_components_ && i < path.num_components_ && it != components_->end() &&
                  oit != path.components_->end();
       i++, it++, oit++) {
    if (*it == *oit) {
      continue;
    }
    return false;
  }
  return path.num_components_ < num_components_;
}

Path Path::Join(const Path &suffix) const {
  std::list<std::string> new_components = get_list();
  new_components.splice(new_components.end(), suffix.get_list());

  return Path(CanonicalizePathList(new_components, absolute_), absolute_, suffix.directory_);
}

Path Path::Cwd() {
  std::string cwd;
  cwd.assign(get_current_dir_name());

  return Path(CanonicalizePathList(cwd), true, true);
}

Path Path::absolute() const {
  if (absolute_) {
    return *this;
  }
  return Cwd() / *this;
}

bool Path::operator==(const Path &other) const {
  if (absolute_ != other.absolute_) {
    return false;
  }
  if (directory_ != other.directory_) {
    return false;
  }
  if (num_components_ != other.num_components_) {
    return false;
  }
  if (components_ == other.components_) {
    return true;
  }
  auto it = components_->begin();
  auto oit = components_->begin();
  for (int i = 0; i < num_components_ && it != components_->end(); i++, it++, oit++) {
    if (*it == *oit) {
      continue;
    }
    return false;
  }
  return true;
}

std::optional<Path> Path::make_relative(const Path &parent) const {
  if (!has_parent(parent)) {
    // This is not a parent, so there isn't anything we can do, just return
    // null.
    return std::nullopt;
  }
  if (absolute_ != parent.absolute_) {
    return std::nullopt;
  }
  auto first_preserved = components_->begin();
  std::advance(first_preserved, parent.num_components_);
  std::list<std::string> new_components(first_preserved, components_->end());
  return Path(std::move(new_components), false, directory_);
}

std::string Path::last_component() const {
  if (num_components_ == 0) {
    return std::string();
  }
  return components_->back();
}

Path Path::common_parent(const Path &other) const {
  if (is_absolute() != other.is_absolute()) {
    // if one is absolute and the other isn't, just return the root.
    return Path::Root();
  }
  if (is_absolute()) {
    if (components_ == other.components_) {
      // The backing list of components is represented by the same shared_ptr,
      // so we are guaranteed that these differ only in the directory attribute
      // and/or num_components, making the with the smaller num_components_ the
      // parent (if it's a directory). (otherwise, it's its parent)
      const Path shorter = num_components_ < other.num_components_ ? *this : other;
      // This conditinoal is particlularly important if &other == this.
      return shorter.directory_ ? shorter : shorter.parent();
    }
    Path cur_parent = num_components_ < other.num_components_ ? *this : other;
    while (!cur_parent.is_root()) {
      if (other.has_parent(cur_parent)) {
        return cur_parent;
      }
      cur_parent = cur_parent.parent();
    }
    return Path::Root();
  }
  // These are both relative paths.
  // We hit the boundary condition. Return an empty relative path.
  return Path(std::list<std::string>(), /* abs = */ false, /* dir = */ true);
}

}  // namespace spin_2_fs

#ifndef include_path_h
#define include_path_h

#include <optional>

#include <list>
#include <memory>
#include <ostream>
#include <string>

namespace spin_2_fs {

class Path {
public:
  explicit Path(const std::string &path);
  // Enable the default move and copy constructors.
  Path(const Path &path) = default;
  Path(Path &&path) = default;
  Path &operator=(const Path &) = default;
  Path &operator=(Path &&) = default;

  Path(std::list<std::string> path, bool abs, bool dir);

  static Path Cwd();
  inline static Path Root() {
    return Path(std::list<std::string>{}, true, true);
  }

  std::string to_string() const;

  // returns the parent directory.
  Path parent() const;
  // returns true if the argument is a parent of *this (e.g. "/" is always a
  // parent of an absolute path)
  bool has_parent(const Path &path) const;
  Path common_parent(const Path &other) const;

  bool is_root() const;

  inline bool is_absolute() const { return absolute_; }

  bool operator<(const Path &other) const;

  Path Join(const Path &suffix) const;
  inline Path operator/(const Path &suffix) const { return Join(suffix); }
  bool operator==(const Path &other) const;
  inline bool operator!=(const Path &other) const { return !(*this == other); }
  // Return a new, relative path constructed by removing a parent.
  std::optional<Path> make_relative(const Path &parent) const;

  Path absolute() const;

  std::string last_component() const;

private:
  std::list<std::string> get_list() const;

  Path(std::shared_ptr<const std::list<std::string>> path, bool abs, bool dir,
       int64_t num_components);
  std::shared_ptr<const std::list<std::string>> components_;
  // True if this was an absolute path.
  bool absolute_;
  // True if there was a trailing slash.
  bool directory_;
  // Number of directories in the path, this allows the use of refcounting when
  // constructing a parent.
  int64_t num_components_;
};

// Define an ostream operator so gtest knows how to pretty-print Paths.
inline std::ostream &operator<<(std::ostream &stream, Path path) {
  stream << path.to_string();
  return stream;
}

} // namespace spin_2_fs
#endif // include_path_h

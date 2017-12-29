// Copyright 2017 David Finkel
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef include_path_h
#define include_path_h

#include <optional>

#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace spin_2_fs {

class Path {
 public:
  explicit Path(const std::string &path);
  // Enable the default move and copy constructors.
  Path(const Path &path) = default;
  Path(Path &&path) = default;
  Path &operator=(const Path &) = default;
  Path &operator=(Path &&) = default;

  Path(std::vector<std::string> path, bool abs, bool dir);

  static Path Cwd();
  inline static Path Root() { return Path(std::vector<std::string>{}, true, true); }

  std::string to_string() const;

  // returns the parent directory.
  Path parent() const;
  // returns true if the argument is a parent of *this (e.g. "/" is always a
  // parent of an absolute path)
  bool has_parent(const Path &path) const;

  bool is_root() const;

  constexpr bool is_absolute() const { return absolute_; }

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
  std::vector<std::string> get_components() const;

  Path(std::shared_ptr<const std::vector<std::string>> path, bool abs, bool dir,
       int64_t num_components);
  std::shared_ptr<const std::vector<std::string>> components_;
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

constexpr bool is_canonical(const std::string_view p) {
  // Empty paths are not actually useful, and not really valid as a relative path.
  if (p.empty()) {
    return false;
  }
  // The only valid canonical path with a "." component, eliminating the need to check for it later.
  if (p == "./") {
    return true;
  }
  if (p.size() == 1) {
    // TODO: validate the characters are valid.
    return true;
  }
  const bool absolute = p[0] == '/';
  bool rel_dot_section = !absolute;
  int ndots = 0;
  int nslashes = 0;
  for (const char c : p) {
    switch (c) {
      case '.':
        ndots++;
        nslashes = 0;
        break;
      case '/':
        // once we've passed a prefix of ".."s,
        if ((absolute || !rel_dot_section) && ndots < 3 && ndots != 0) {
          return false;
        }
        if (rel_dot_section && (ndots == 0 || ndots > 2)) {
          rel_dot_section = false;
        }
        // We already handled the only valid cananical paths with a "." component.
        if (ndots == 1) {
          return false;
        }
        if (nslashes > 0) {
          return false;
        }
        ndots = 0;
        nslashes++;
        break;
      default:
        ndots = 0;
        nslashes = 0;
    }
  }
  if ((absolute || !rel_dot_section) && ndots < 3 && ndots != 0) {
    return false;
  }
  // We already handled the only valid cananical paths with a "." component.
  if (ndots == 1) {
    return false;
  }
  if (nslashes > 1) {
    return false;
  }
  return true;
}

}  // namespace spin_2_fs
#endif  // include_path_h

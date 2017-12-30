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

#ifndef include_spin_2_fs_path_h
#define include_spin_2_fs_path_h

#include <optional>

#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace spin_2_fs {

// Path represents a filesystem path. Attempts have been made to make common operations such as
// fetching the parent directory cheap.
//
// The underlying vector containing path-components is reference-counted, so requesting the parent
// directory involves copying 2 bools, 1 int (that gets decremented), a pointer and updating a
// shared_ptr refcount.
class Path {
 public:
  explicit Path(const std::string &path);
  // Enable the default move and copy constructors.
  Path(const Path &path) = default;
  Path(Path &&path) = default;
  Path &operator=(const Path &) = default;
  Path &operator=(Path &&) = default;

  // Raw Constructor intended for bypassing validation and string parsing.
  Path(std::vector<std::string> path, bool abs, bool dir);

  // Get the current working directory as a Path object.
  static Path Cwd();
  // Fast factory for a Path representing "/".
  inline static Path Root() { return Path(std::vector<std::string>{}, true, true); }

  // Since we can't add a std::string constructor for Path, we have to settle for a to_string()
  // method.
  std::string to_string() const;

  // returns the parent directory.
  Path parent() const;
  // returns true if the argument is a parent of *this (e.g. "/" is always a
  // parent of an absolute path)
  bool has_parent(const Path &path) const;

  // As the name implies, returns true iff the path is "/".
  // (useful in the termination condition for loops iterating over parent directories)
  constexpr bool is_root() const { return num_components_ == 0 && absolute_; }

  // Returns false if the path is relative.
  constexpr bool is_absolute() const { return absolute_; }

  // Used for sorting paths. Unlike strict lexical sorting, parent paths always sort immediately
  // before children.
  // Relative paths sort after absolute ones.
  bool operator<(const Path &other) const;

  // Concatenation methods.
  Path Join(const Path &suffix) const;
  inline Path operator/(const Path &suffix) const { return Join(suffix); }

  bool operator==(const Path &other) const;
  inline bool operator!=(const Path &other) const { return !(*this == other); }

  // Return a new, relative path constructed by removing a parent.
  // May return std:nullopt if the parent argument is not an actually a parent of the path
  // make_relative() is operating on.
  std::optional<Path> make_relative(const Path &parent) const;

  // Converts a relative path into an absolute path by applying the relative path to the CWD.
  Path absolute() const;

  // Returns the last component of the path or an empty string if the path is empty or the root.
  std::string last_component() const;

 private:
  // Copies the current vector of components, trimmed down to the correct length. (used to implement
  // a number of methods)
  std::vector<std::string> get_components() const;

  // Internal constructor used to implement the reference-counted components.
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

// constexpr function for verifying if a string is a canonical path. Intended usage is with
// static_assert and string_view compile-time constants to enforce that such strings are in a
// canonical form.
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
#endif  // include_spin_2_fs_path_h
// vim: sw=2:sts=2:tw=100:et:cindent:cinoptions=l1,g1,h1,N-s,E-s,i2s,+2s,(0,w1,W2s

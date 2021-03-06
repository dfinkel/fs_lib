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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "path.h"

namespace spin_2_fs {
namespace {

TEST(TestPath, CanonicalizeAbsolutePaths) {
  EXPECT_EQ("/foo/bar/fim", Path("/foo/bar/fim").to_string());
  EXPECT_EQ("/foo/bar/fim", Path("/foo/bar/./fim").to_string());
  EXPECT_EQ("/foo/bar/fim", Path("/foo/bar/boo/../fim").to_string());
  EXPECT_EQ("/foo/bar/fim", Path("/foo/bar/boo/bim/../../fim").to_string());
  EXPECT_EQ("/fim", Path("/foo/bar/boo/bim/../../../../../fim").to_string());
  EXPECT_EQ("/fim", Path("/foo/bar/boo/bim/../../../../../../../fim").to_string());
  EXPECT_EQ("/fim", Path("/foo/bar/boo/bim/.././.././../../../fim").to_string());
  EXPECT_EQ("/foo/fim", Path("/foo/./boo/../fim").to_string());
  EXPECT_EQ("/", Path("/.././.././../////.././").to_string());
  EXPECT_EQ("/foo/fim", Path("/foo/bar/boo/bim/../../../fim").to_string());
  EXPECT_EQ("/fim", Path("/foo/bar/boo/bim/../../../.././fim").to_string());
  EXPECT_EQ("/foo/bar/fim", Path("/foo/bar/boo/bim/../.././fim").to_string());
}

TEST(TestPath, ConstructParentAbsolute) {
  Path fim("/foo/bar/fim");
  EXPECT_EQ("/foo/bar/", fim.parent().to_string());
  EXPECT_EQ("/foo/", fim.parent().parent().to_string());
  EXPECT_EQ("/", fim.parent().parent().parent().to_string());
  // Verify that continuing to request the parent after hitting the root
  // returns the root.
  EXPECT_EQ("/", fim.parent().parent().parent().parent().to_string());
}
TEST(TestPath, ConstructParentRelative) {
  Path fim("foo/bar/fim");
  EXPECT_EQ("foo/bar/", fim.parent().to_string());
  EXPECT_EQ("foo/", fim.parent().parent().to_string());
  EXPECT_EQ(".", fim.parent().parent().parent().to_string());
  // Verify that we start getting ".." components as we add more parents.
  EXPECT_EQ("../", fim.parent().parent().parent().parent().to_string());
  EXPECT_EQ("../../", fim.parent().parent().parent().parent().parent().to_string());
  EXPECT_EQ("../../../", Path("").parent().parent().parent().to_string());
  EXPECT_EQ("../../../../", Path("..").parent().parent().parent().to_string());
  EXPECT_EQ(".", Path("").to_string());
  EXPECT_EQ(".", Path(".").to_string());
  EXPECT_EQ("../", Path("").parent().to_string());
  EXPECT_EQ("../", Path(".").parent().to_string());
}

TEST(TestPath, IsRoot) {
  Path fim("/foo/bar/fim");
  EXPECT_TRUE(fim.parent().parent().parent().parent().is_root());
  EXPECT_TRUE(fim.parent().parent().parent().is_root());
  EXPECT_FALSE(fim.parent().parent().is_root());
  EXPECT_FALSE(fim.parent().is_root());
  EXPECT_FALSE(fim.is_root());
}

TEST(TestPath, IsAbsolute) {
  Path f("foo/bar/bim");
  EXPECT_EQ("foo/bar/bim", f.to_string());
  EXPECT_FALSE(f.is_absolute());
  Path g("/foo/bar/bim");
  EXPECT_TRUE(g.is_absolute());
  EXPECT_TRUE(is_canonical(g.to_string()));
}

TEST(TestPath, CanonicalizeRelativePath) {
  Path foo("../foo/bar");
  EXPECT_EQ("../foo/bar", foo.to_string());
  EXPECT_EQ("../foo/", foo.parent().to_string());
  EXPECT_EQ("../foo/bar", Path("./.././foo/./fim/../bar").to_string());
  EXPECT_EQ("../foo/bar/", Path("./.././foo/./fim/../bar/").to_string());
  EXPECT_EQ("../foo/fim/bim/bar/", Path("./.././foo/fim/bim/bop/../bar/").to_string());
  EXPECT_EQ("../../foo/fim/bim/bar/", Path("../../foo/fim/bim/bar/").to_string());
  EXPECT_EQ("../../../../../../foo/fim/bim/bar/",
            Path("../../../../../../foo/fim/bim/bar/").to_string());
  EXPECT_EQ("../../../../foo/fim/bim/bar/",
            Path("../../foo/bar/../../../../foo/fim/bim/bar/").to_string());
  EXPECT_TRUE(is_canonical(Path("../../foo/bar/../../../../foo/fim/bim/bar/").to_string()));
}

TEST(TestPath, IsParent) {
  Path bar("/foo/bar/");
  Path foo("/foo/");
  EXPECT_TRUE(bar.has_parent(foo));
  EXPECT_FALSE(foo.has_parent(bar));
  Path bim("/foo/bar/bop/bim");
  EXPECT_TRUE(bim.has_parent(foo));
  Path root("/");
  EXPECT_TRUE(bim.has_parent(root));
  Path bar_foo("/foo/bar/bim/boo/bock");
  Path bar_from_parent = bar_foo.parent().parent().parent();
  EXPECT_EQ(bar_from_parent.to_string(), bar.to_string());
  EXPECT_EQ(bar_from_parent, bar);
  EXPECT_TRUE(bim.has_parent(bar_from_parent));
}

TEST(TestPath, TestJoin) {
  const Path bar("foo/bar");
  const Path bin("/boo/bin/bim");
  Path bar_abs = bin.Join(bar);
  EXPECT_EQ("/boo/bin/bim/foo/bar", bar_abs.to_string());
  Path bar_abs_slash = bin / bar;
  EXPECT_EQ("/boo/bin/bim/foo/bar", bar_abs_slash.to_string());

  EXPECT_EQ("/foo/bar/bim/boo", (Path("/foo/bar/bim/bing") / Path("../boo")).to_string());
  EXPECT_EQ("/foo/bar/boo", (Path("/foo/bar/bim/bing") / Path("../../boo")).to_string());
  EXPECT_EQ("/foo/bar/boo", (Path("/foo/bar/bim/bing") / Path("./../../boo")).to_string());
  EXPECT_EQ("/foo/bar/boo/", (Path("/foo/bar/bim/bing") / Path("./../../boo/")).to_string());

  Path vic("voo/vim/vik");
  {
    Path von("../von");
    EXPECT_EQ("voo/vim/von", (vic / von).to_string());
  }
  {
    Path von("../../vim/von");
    EXPECT_EQ("voo/vim/von", (vic / von).to_string());
  }
  EXPECT_EQ("voo/vim/von", (vic / Path("./../../vim/von")).to_string());
  EXPECT_EQ("voo/vim/von", (vic / Path("../.././vim/von")).to_string());
  EXPECT_TRUE(is_canonical((vic / Path("../.././vim/von")).to_string()));
}

TEST(TestPath, TestCwd) {
  const Path bar = Path::Cwd();
  EXPECT_TRUE(bar.is_absolute());
  ASSERT_TRUE(chdir("/tmp") == 0);
  const Path tmp = Path::Cwd();
  EXPECT_EQ("/tmp/", tmp.to_string());
  EXPECT_TRUE(is_canonical(tmp.to_string()));
}

TEST(TestPath, TestAbsolute) {
  const Path bar = Path::Cwd();
  EXPECT_TRUE(bar.is_absolute());
  ASSERT_TRUE(chdir("/tmp") == 0);
  const Path tmp = Path::Cwd();
  const Path i("foo/bar/bin");
  EXPECT_EQ("foo/bar/bin", i.to_string());
  EXPECT_EQ("/tmp/foo/bar/bin", i.absolute().to_string());
  EXPECT_TRUE(i.absolute().is_absolute());
}

TEST(TestPath, TestMakeRelative) {
  const Path foo("/bim/bar/foo");
  const Path bim("/bim");
  const std::optional<Path> bar_foo = foo.make_relative(bim);
  EXPECT_EQ(bar_foo.value(), Path("bar/foo"));
  const std::optional<Path> dne = bim.make_relative(foo);
  EXPECT_EQ(dne.value_or(Path("/foo/bar/bim/boo/bop/vim")), Path("/foo/bar/bim/boo/bop/vim"));
}

TEST(TestPath, TestRoot) {
  const Path root = Path::Root();
  EXPECT_EQ(root, Path::Root());
  EXPECT_EQ(root, Path("/"));
}

TEST(TestPath, TestLastComponent) {
  const Path foo("/bim/bar/foo");
  const Path bim("/bim");
  const std::string bar_foo = foo.last_component();
  EXPECT_EQ(bar_foo, "foo");
  const std::string dne = bim.last_component();
  EXPECT_EQ(dne, "bim");
}

TEST(TestCanonical, TooManyDots) {
  constexpr bool f = is_canonical("./././");
  static_assert(!f, "`./././` is not a valid path, but is_canonical() returned true");
  EXPECT_FALSE(is_canonical("./././"));
  EXPECT_FALSE(is_canonical("./."));
  EXPECT_FALSE(is_canonical("./.."));
  EXPECT_FALSE(is_canonical("/.."));
  EXPECT_FALSE(is_canonical("/."));
}

TEST(TestCanonical, ManyMoreDots) {
  EXPECT_TRUE(is_canonical(".../...../....../"));
  EXPECT_TRUE(is_canonical("..../..."));
  EXPECT_TRUE(is_canonical("..../..."));
  EXPECT_TRUE(is_canonical("/..."));
}

TEST(TestCanonical, EmptyComponents) {
  EXPECT_FALSE(is_canonical(""));
  EXPECT_FALSE(is_canonical("//"));
  EXPECT_FALSE(is_canonical("//fooo"));
  EXPECT_FALSE(is_canonical("foo//"));
  EXPECT_FALSE(is_canonical("foo/bar//"));
  EXPECT_FALSE(is_canonical("foo//bar/"));
}

TEST(TestCanonical, SpecialCases) {
  EXPECT_FALSE(is_canonical(""));
  EXPECT_TRUE(is_canonical("./"));
  EXPECT_TRUE(is_canonical("."));
  EXPECT_TRUE(is_canonical("/"));
  EXPECT_TRUE(is_canonical("a"));
}

TEST(TestCanonical, RelPrefix) {
  EXPECT_TRUE(is_canonical("../../../a/b/c/d"));
  EXPECT_TRUE(is_canonical("../../../a/b/c/d/"));
}
}  // anonymous namespace
}  // namespace spin_2_fs
// vim: sw=2:sts=2:tw=100:et:cindent:cinoptions=l1,g1,h1,N-s,E-s,i2s,+2s,(0,w1,W2s

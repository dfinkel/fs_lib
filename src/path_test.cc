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
  EXPECT_EQ("/fim",
            Path("/foo/bar/boo/bim/../../../../../../../fim").to_string());
  EXPECT_EQ("/fim",
            Path("/foo/bar/boo/bim/.././.././../../../fim").to_string());
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
  EXPECT_EQ("../../",
            fim.parent().parent().parent().parent().parent().to_string());
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
}

TEST(TestPath, CanonicalizeRelativePath) {
  Path foo("../foo/bar");
  EXPECT_EQ("../foo/bar", foo.to_string());
  EXPECT_EQ("../foo/", foo.parent().to_string());
  EXPECT_EQ("../foo/bar", Path("./.././foo/./fim/../bar").to_string());
  EXPECT_EQ("../foo/bar/", Path("./.././foo/./fim/../bar/").to_string());
  EXPECT_EQ("../foo/fim/bim/bar/",
            Path("./.././foo/fim/bim/bop/../bar/").to_string());
  EXPECT_EQ("../../foo/fim/bim/bar/",
            Path("../../foo/fim/bim/bar/").to_string());
  EXPECT_EQ("../../../../../../foo/fim/bim/bar/",
            Path("../../../../../../foo/fim/bim/bar/").to_string());
  EXPECT_EQ("../../../../foo/fim/bim/bar/",
            Path("../../foo/bar/../../../../foo/fim/bim/bar/").to_string());
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

  EXPECT_EQ("/foo/bar/bim/boo",
            (Path("/foo/bar/bim/bing") / Path("../boo")).to_string());
  EXPECT_EQ("/foo/bar/boo",
            (Path("/foo/bar/bim/bing") / Path("../../boo")).to_string());
  EXPECT_EQ("/foo/bar/boo",
            (Path("/foo/bar/bim/bing") / Path("./../../boo")).to_string());
  EXPECT_EQ("/foo/bar/boo/",
            (Path("/foo/bar/bim/bing") / Path("./../../boo/")).to_string());

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
}

TEST(TestPath, TestCwd) {
  const Path bar = Path::Cwd();
  EXPECT_TRUE(bar.is_absolute());
  ASSERT_TRUE(chdir("/tmp") == 0);
  const Path tmp = Path::Cwd();
  EXPECT_EQ("/tmp/", tmp.to_string());
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
  EXPECT_EQ(dne.value_or(Path("/foo/bar/bim/boo/bop/vim")),
            Path("/foo/bar/bim/boo/bop/vim"));
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

TEST(TestPath, TestCommonParent) {
  const Path foo("/bar/baz/bim/foo/");
  const Path baz = foo.parent().parent();
  EXPECT_EQ(foo.common_parent(baz), baz);
  EXPECT_EQ(baz.common_parent(foo), baz);
  // foo is a directory, so it is its own common parent.
  EXPECT_EQ(foo.common_parent(foo), foo);
  // baz is a directory, so it is its own common parent.
  EXPECT_EQ(baz.common_parent(baz), baz);
  const Path baz_indep("/bar/baz/");
  EXPECT_EQ(foo.common_parent(baz_indep), baz);
  EXPECT_EQ(baz_indep.common_parent(foo), baz);
}
} // anonymous namespace
} // namespace spin_2_fs

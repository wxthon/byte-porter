workspace(name = "com_github_wxthon_byte_porter")

local_repository(
    name = "com_github_nelhage_rules_boost",
    path = "./third_party/rules_boost",
)
load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "hedron_compile_commands",
    urls = [
        "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/d1e95ec162e050b04d0a191826f9bc478de639f7.tar.gz",
    ],
    strip_prefix = "bazel-compile-commands-extractor-d1e95ec162e050b04d0a191826f9bc478de639f7",
    sha256 = "ab6c6b4ceaf12b224e571ec075fd79086c52c3430993140bb2ed585b08dfc552",
)

http_archive(
    name = "com_github_gflags_gflags",
    strip_prefix = "gflags-2.2.2",
    urls = ["https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"],
)

http_archive(
    name = "com_github_google_glog",
    strip_prefix = "glog-0.6.0",
    urls = ["https://github.com/google/glog/archive/v0.6.0.zip"],
)


cc_binary(
    name = "byte-porter-server",
    srcs = glob([
        "*.cc",
        "*.h",
        "http/*.h",
    ]),
    deps = [
        "@boost//:asio",
        "@boost//:log",
        "@boost//:beast",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_google_glog//:glog",
    ],
)

filegroup(
    name = "porter",
    srcs = [
        ":byte-porter-server",
    ]
)

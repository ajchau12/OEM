load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "hdrs",
    hdrs = [
        "api.h",
    ],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "can",
    srcs = [
        "can.c",
    ],
    hdrs = [
        "mob.h",
    ],
    target_compatible_with = select({
        "//bazel/constraints:atmega16m1": [],
        "//bazel/constraints:atmega64m1": [],
        "//conditions:default": ["//bazel/constraints:incompatible_mcu"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":hdrs",
    ],
)

cc_library(
    name = "mock",
    srcs = [
        "mock/can.c",
    ],
    hdrs = [
        "mock/mock.h",
    ],
    target_compatible_with = [
        "@platforms//os:linux",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":hdrs",
        "@cmocka",
    ],
)

cc_library(
    name = "can_legacy",
    srcs = [
        "legacy/can_api.c",
    ],
    hdrs = [
        "legacy/can_api.h",
    ],
    deprecation = "Please transition to the new library: //libs/can",
    target_compatible_with = [
        "//bazel/constraints:atmega16m1",
    ],
    visibility = ["//visibility:public"],
)

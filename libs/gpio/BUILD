cc_library(
    name = "hdrs",
    hdrs = [
        "api.h",
    ],
)

cc_library(
    name = "gpio",
    srcs = [
        "gpio.c",
    ],
    hdrs = [
        "pin_defs.h",
    ],
    #https://docs.bazel.build/versions/main/platforms.html#more-expressive-constraints
    target_compatible_with = select({
        "//bazel/constraints:atmega16m1": [],
        "//bazel/constraints:atmega328p": [],
        "//conditions:default": ["//bazel/constraints:incompatible"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":hdrs",
    ],
)

cc_library(
    name = "mock",
    srcs = [
        "mock/gpio.c",
    ],
    deps = [
        ":hdrs",
    ],
)

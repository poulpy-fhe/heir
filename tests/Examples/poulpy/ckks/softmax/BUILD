load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "softmax",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--softmax-to-cgf-softmax",
        "--torch-linalg-to-ckks=min-slot-count=1024 first-mod-bits=60 greedy-level-budget=20 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:softmax.mlir",
)

rust_test(
    name = "softmax_test",
    srcs = ["softmax_test.rs"],
    deps = [
        ":softmax",
        "@crates//:poulpy-cpu-portable",
    ],
)

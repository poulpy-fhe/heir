load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "pad_dot",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--mlir-to-ckks=min-slot-count=2048 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:pad_dot.mlir",
)

rust_test(
    name = "pad_dot_test",
    srcs = ["pad_dot_test.rs"],
    deps = [
        ":pad_dot",
        "@crates//:poulpy-cpu-portable",
    ],
)

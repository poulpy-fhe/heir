load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "relu_composite",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--torch-linalg-to-ckks=min-slot-count=1024 use-composite-relu=true greedy-level-budget=30 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:relu_composite.mlir",
)

rust_test(
    name = "relu_composite_test",
    srcs = ["relu_composite_test.rs"],
    deps = [
        ":relu_composite",
        "@crates//:poulpy-cpu-portable",
    ],
)

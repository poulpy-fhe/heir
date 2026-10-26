load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "sigmoid",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--torch-linalg-to-ckks=min-slot-count=2048 scaling-mod-bits=55 first-mod-bits=60 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:sigmoid.mlir",
)

rust_test(
    name = "sigmoid_test",
    srcs = ["sigmoid_test.rs"],
    deps = [
        ":sigmoid",
        "@crates//:poulpy-cpu-portable",
    ],
)

load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "linalg_dot",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--mlir-to-ckks=min-slot-count=2048 first-mod-bits=0 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:linalg_dot.mlir",
)

rust_test(
    name = "linalg_dot_test",
    srcs = ["linalg_dot_test.rs"],
    deps = [
        ":linalg_dot",
        "@crates//:poulpy-cpu-portable",
    ],
)

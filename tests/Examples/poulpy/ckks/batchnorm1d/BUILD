load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "batchnorm1d",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--linalg-canonicalizations",
        "--torch-linalg-to-ckks=min-slot-count=2048 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:batchnorm1d.mlir",
)

rust_test(
    name = "batchnorm1d_test",
    srcs = ["batchnorm1d_test.rs"],
    deps = [
        ":batchnorm1d",
        "@crates//:poulpy-cpu-portable",
    ],
)

load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "matvec_batchnorm",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--linalg-canonicalizations",
        "--torch-linalg-to-ckks=min-slot-count=2048 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:matvec_batchnorm.mlir",
)

rust_test(
    name = "matvec_batchnorm_test",
    srcs = ["matvec_batchnorm_test.rs"],
    deps = [
        ":matvec_batchnorm",
        "@crates//:poulpy-cpu-portable",
    ],
)

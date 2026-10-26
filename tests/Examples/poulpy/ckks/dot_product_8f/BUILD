load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "dot_product_8f",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--mlir-to-ckks=min-slot-count=8 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:dot_product_8f.mlir",
)

rust_test(
    name = "dot_product_8f_test",
    srcs = ["dot_product_8f_test.rs"],
    deps = [
        ":dot_product_8f",
        "@crates//:poulpy-cpu-portable",
    ],
)

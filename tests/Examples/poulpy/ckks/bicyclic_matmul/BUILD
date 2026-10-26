load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "bicyclic_matmul",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--mlir-to-ckks=min-slot-count=2048 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:bicyclic_matmul.mlir",
)

rust_test(
    name = "bicyclic_matmul_test",
    srcs = ["bicyclic_matmul_test.rs"],
    deps = [
        ":bicyclic_matmul",
        "@crates//:poulpy-cpu-portable",
    ],
)

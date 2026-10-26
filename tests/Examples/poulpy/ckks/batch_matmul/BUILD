load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "batch_matmul",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--mlir-to-ckks=min-slot-count=4096 greedy-level-budget=40 first-mod-bits=60 scaling-mod-bits=50 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:batch_matmul.mlir",
)

rust_test(
    name = "batch_matmul_test",
    srcs = ["batch_matmul_test.rs"],
    deps = [
        ":batch_matmul",
        "@crates//:poulpy-cpu-portable",
    ],
)

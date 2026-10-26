load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "matvec_tall",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--mlir-to-ckks=min-slot-count=1024 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:matvec_8x4.mlir",
)

rust_test(
    name = "matvec_tall_test",
    srcs = ["matvec_tall_test.rs"],
    deps = [
        ":matvec_tall",
        "@crates//:poulpy-cpu-portable",
    ],
)

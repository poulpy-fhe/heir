load("@heir//tools:heir-poulpy.bzl", "heir_poulpy_lib")
load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["@heir//:license"])

heir_poulpy_lib(
    name = "conv1d_ncw",
    heir_opt_flags = [
        "--annotate-module=backend=poulpy scheme=ckks",
        "--mlir-to-ckks=min-slot-count=1024 use-public-key=false enable-split-preprocessing=false",
        "--scheme-to-poulpy",
    ],
    mlir_src = "@heir//tests/Examples/common:conv1d_ncw.mlir",
)

rust_test(
    name = "conv1d_ncw_test",
    srcs = ["conv1d_ncw_test.rs"],
    deps = [
        ":conv1d_ncw",
        "@crates//:poulpy-cpu-portable",
    ],
)

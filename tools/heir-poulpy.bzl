"""A macro providing a Rust library for Poulpy codegen."""

load("@heir//tools:heir-opt.bzl", "heir_opt")
load("@heir//tools:heir-translate.bzl", "heir_translate")
load("@rules_rust//rust:defs.bzl", "rust_library")

def heir_poulpy_lib(name, mlir_src, heir_opt_flags, tags = [], **kwargs):
    """Generates Poulpy Rust code from MLIR and compiles it to a rust_library.

    The library is generic over the Poulpy backend: its user depends on a
    backend crate such as @crates//:poulpy-cpu-portable and picks one.

    Args:
      name: The name of the generated rust_library, also its crate name.
      mlir_src: The source mlir file to run through heir-opt and heir-translate.
      heir_opt_flags: Flags to pass to heir-opt, ending with --scheme-to-poulpy.
      tags: Tags to pass to the generated targets.
      **kwargs: Keyword arguments to pass to rust_library.
    """
    generated_heir_opt_name = "%s_heir_opt.mlir" % name
    generated_rs_filename = "%s_lib.rs" % name

    heir_opt(
        name = "%s_heir_opt" % name,
        src = mlir_src,
        pass_flags = heir_opt_flags,
        generated_filename = generated_heir_opt_name,
        # The emitter inlines constants; it does not load them from files.
        externalize_constants = False,
        tags = tags,
    )
    heir_translate(
        name = "%s_heir_translate" % name,
        src = generated_heir_opt_name,
        pass_flags = ["--emit-poulpy"],
        generated_filename = generated_rs_filename,
        tags = tags,
    )
    rust_library(
        name = name,
        srcs = [generated_rs_filename],
        deps = [
            "@crates//:anyhow",
            "@crates//:poulpy-ckks",
            "@crates//:poulpy-core",
            "@crates//:poulpy-hal",
        ],
        tags = tags,
        **kwargs
    )

#ifndef LIB_DIALECT_POULPY_TRANSFORMS_CONFIGURECRYPTOCONTEXT_TD_
#define LIB_DIALECT_POULPY_TRANSFORMS_CONFIGURECRYPTOCONTEXT_TD_

include "mlir/Pass/PassBase.td"

def ConfigureCryptoContext : Pass<"poulpy-configure-crypto-context", "ModuleOp"> {
  let summary = "Generate the function that sets up the Poulpy context and keys";
  let description = [{
    For an entry function `@f` lowered by `--lwe-to-poulpy`, generates

    ```mlir
    func.func @f__configure() -> (!poulpy.module, !poulpy.scratch,
        [!poulpy.tensor_key], [!poulpy.automorphism_key_map],
        !poulpy.secret_key)
    ```

    The leading results are the context arguments of `@f`, in order: the
    tensor key only if `@f` multiplies, the automorphism keys only if it
    rotates, with one key per rotation it performs. The secret key encrypts
    and decrypts.

    Parameters come from `ckks.schemeParam`: the ring degree `2^logN`, the
    ciphertext width `k` (the sum of the bit sizes of all moduli `Q`) and the
    scale `2^logDefaultScale`. Keys use digits of one limb of `base2k` bits and
    an auxiliary guard of one digit plus `logN` bits, Poulpy's default, so the
    widest key has `ceil(k / base2k) * base2k + base2k + logN` bits. The pass
    fails if that width is not 128-bit secure at the ring degree.
  }];
  let dependentDialects = [
    "mlir::func::FuncDialect",
    "mlir::heir::poulpy::PoulpyDialect",
  ];
  let options = [
    Option<"entryFunction", "entry-function", "std::string", /*default=*/"",
           "The function to configure; detected if empty.">,
    Option<"base2k", "base2k", "int64_t", /*default=*/"52",
           "Bits per limb, as given to --lwe-to-poulpy.">,
  ];
}

#endif  // LIB_DIALECT_POULPY_TRANSFORMS_CONFIGURECRYPTOCONTEXT_TD_

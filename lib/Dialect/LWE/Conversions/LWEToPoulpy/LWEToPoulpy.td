#ifndef LIB_DIALECT_LWE_CONVERSIONS_LWETOPOULPY_LWETOPOULPY_TD_
#define LIB_DIALECT_LWE_CONVERSIONS_LWETOPOULPY_LWETOPOULPY_TD_

include "mlir/Pass/PassBase.td"

def LWEToPoulpy : Pass<"lwe-to-poulpy", "ModuleOp"> {
  let summary = "Lower `lwe` and `ckks` to the tensor form of the `poulpy` dialect.";
  let description = [{
    Lowers CKKS programs at the `lwe` level to the tensor form of the `poulpy`
    dialect, to be bufferized by `--poulpy-bufferize` and emitted by
    `--emit-poulpy`.

    - Every function that handles a ciphertext or plaintext gets the poulpy
      module and scratch arena as leading arguments, plus the tensor key if it
      multiplies ciphertexts and the automorphism keys if it rotates.
    - A ciphertext multiplication, its relinearization and its rescale become
      one `poulpy.mul`: Poulpy's multiplication relinearizes, and rescale is a
      no-op in Poulpy since a product already comes out at its operands' scale.
      The pass therefore requires each multiplication to be relinearized and
      rescaled right away, as the poulpy target arranges.
    - `ckks.level_reduce` is dropped: Poulpy aligns the operands of an addition
      itself.
    - A ciphertext at level `l` has width `k(l)`, the sum of the bit sizes of
      the first `l + 1` moduli of `ckks.schemeParam`.

    Not supported yet: public-key encryption, bootstrapping, kernel and
    preprocessing ops, and tensors of more than one ciphertext.
  }];
  let dependentDialects = [
    "mlir::arith::ArithDialect",
    "mlir::heir::poulpy::PoulpyDialect",
    "mlir::tensor::TensorDialect",
  ];
  let options = [
    Option<"base2k", "base2k", "int64_t", /*default=*/"52",
           "Bits per limb of the ciphertexts and plaintexts.">,
  ];
}

#endif  // LIB_DIALECT_LWE_CONVERSIONS_LWETOPOULPY_LWETOPOULPY_TD_

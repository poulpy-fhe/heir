// RUN: heir-opt --poulpy-configure-crypto-context=entry-function=add %s | FileCheck %s

!ct = tensor<!poulpy.ciphertext>
!module = !poulpy.module
!scratch = !poulpy.scratch

// Without multiplications or rotations, no evaluation keys are generated.
module attributes {ckks.schemeParam = #ckks.scheme_param<logN = 14, Q = [36028797017456641, 35184371138561, 35184372121601], P = [1152921504607338497, 1152921504608747521], logDefaultScale = 45>} {
  func.func @add(%m: !module, %s: !scratch, %a: !ct, %b: !ct) -> !ct {
    %dst = tensor.empty() : !ct
    %0 = poulpy.add %m, %dst, %a, %b, %s : (!module, !ct, !ct, !ct, !scratch) -> !ct
    return %0 : !ct
  }
}

// CHECK: func.func @add__configure() -> (!poulpy.module, !poulpy.scratch, !poulpy.secret_key)
// CHECK-NOT: poulpy.tensor_key_generate
// CHECK-NOT: poulpy.automorphism_keys_generate
// CHECK: poulpy.secret_key_prepare

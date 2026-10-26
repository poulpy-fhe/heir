// RUN: heir-opt --poulpy-configure-crypto-context %s | FileCheck %s

!ct = tensor<!poulpy.ciphertext>
!module = !poulpy.module
!scratch = !poulpy.scratch
!tsk = !poulpy.tensor_key
!akm = !poulpy.automorphism_key_map

module attributes {ckks.schemeParam = #ckks.scheme_param<logN = 14, Q = [36028797017456641, 35184371138561, 35184372121601], P = [1152921504607338497, 1152921504608747521], logDefaultScale = 45>} {
  func.func @rotate(%m: !module, %s: !scratch, %akm: !akm, %ct: !ct) -> !ct {
    %dst = tensor.empty() : !ct
    %0 = poulpy.rotate %m, %dst, %ct, %akm, %s <k = -1> : (!module, !ct, !ct, !akm, !scratch) -> !ct
    %dst1 = tensor.empty() : !ct
    %1 = poulpy.rotate %m, %dst1, %0, %akm, %s <k = 4> : (!module, !ct, !ct, !akm, !scratch) -> !ct
    return %1 : !ct
  }

  // The entry function: the rotations of its callee count.
  func.func @main(%m: !module, %s: !scratch, %tsk: !tsk, %akm: !akm, %a: !ct, %b: !ct) -> !ct {
    %dst = tensor.empty() : !ct
    %0 = poulpy.mul %m, %dst, %a, %b, %tsk, %s : (!module, !ct, !ct, !ct, !tsk, !scratch) -> !ct
    %dst1 = tensor.empty() : !ct
    %1 = poulpy.rotate %m, %dst1, %0, %akm, %s <k = 4> : (!module, !ct, !ct, !akm, !scratch) -> !ct
    %2 = func.call @rotate(%m, %s, %akm, %1) : (!module, !scratch, !akm, !ct) -> !ct
    return %2 : !ct
  }
}

// CHECK: func.func @main__configure() -> (!poulpy.module, !poulpy.scratch, !poulpy.tensor_key, !poulpy.automorphism_key_map, !poulpy.secret_key)
// CHECK-NEXT: %[[M:.*]] = poulpy.module_create <N = 16384>
// k = 55 + 45 + 45; kAux = 52 + 14
// CHECK-NEXT: %[[S:.*]] = poulpy.scratch_alloc_for_all_ops %[[M]] <base2k = 52, k = 145, kAux = 66, dsize = 1, logDelta = 45>
// CHECK-NEXT: %[[SECRET:.*]] = poulpy.secret_generate %[[M]] :
// CHECK-NEXT: %[[TSK:.*]] = poulpy.tensor_key_generate %[[M]], %[[SECRET]], %[[S]] <base2k = 52, k = 145, kAux = 66, dsize = 1>
// CHECK-NEXT: %[[AKM:.*]] = poulpy.automorphism_keys_generate %[[M]], %[[SECRET]], %[[S]] <base2k = 52, k = 145, kAux = 66, dsize = 1, rotations = [-1, 4]>
// CHECK-NEXT: %[[SK:.*]] = poulpy.secret_key_prepare %[[M]], %[[SECRET]]
// CHECK-NEXT: return %[[M]], %[[S]], %[[TSK]], %[[AKM]], %[[SK]]

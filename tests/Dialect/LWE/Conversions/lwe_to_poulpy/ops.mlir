// RUN: heir-opt --lwe-to-poulpy %s | FileCheck %s

// The moduli have 55, 45 and 45 bits: ciphertexts at levels 0, 1 and 2 have
// widths 55, 100 and 145.

!Z35184371138561_i64 = !mod_arith.int<35184371138561 : i64>
!Z35184372121601_i64 = !mod_arith.int<35184372121601 : i64>
!Z36028797017456641_i64 = !mod_arith.int<36028797017456641 : i64>
#inverse_canonical_encoding = #lwe.inverse_canonical_encoding<scaling_factor = 45>
#inverse_canonical_encoding1 = #lwe.inverse_canonical_encoding<scaling_factor = 90>
#key = #lwe.key<>
#modulus_chain_L2_C0 = #lwe.modulus_chain<elements = <36028797017456641 : i64, 35184371138561 : i64, 35184372121601 : i64>, current = 0>
#modulus_chain_L2_C1 = #lwe.modulus_chain<elements = <36028797017456641 : i64, 35184371138561 : i64, 35184372121601 : i64>, current = 1>
#modulus_chain_L2_C2 = #lwe.modulus_chain<elements = <36028797017456641 : i64, 35184371138561 : i64, 35184372121601 : i64>, current = 2>
#ring_f64_1_x16384 = #polynomial.ring<coefficientType = f64, polynomialModulus = <1 + x**16384>>
!rns_L0 = !rns.rns<!Z36028797017456641_i64>
!rns_L1 = !rns.rns<!Z36028797017456641_i64, !Z35184371138561_i64>
!rns_L2 = !rns.rns<!Z36028797017456641_i64, !Z35184371138561_i64, !Z35184372121601_i64>
!pt = !lwe.lwe_plaintext<plaintext_space = <ring = #ring_f64_1_x16384, encoding = #inverse_canonical_encoding>>
#ring_rns_L0_1_x16384 = #polynomial.ring<coefficientType = !rns_L0, polynomialModulus = <1 + x**16384>>
#ring_rns_L1_1_x16384 = #polynomial.ring<coefficientType = !rns_L1, polynomialModulus = <1 + x**16384>>
#ring_rns_L2_1_x16384 = #polynomial.ring<coefficientType = !rns_L2, polynomialModulus = <1 + x**16384>>
!skey_L0 = !lwe.lwe_secret_key<key = #key, ring = #ring_rns_L0_1_x16384>
!skey_L2 = !lwe.lwe_secret_key<key = #key, ring = #ring_rns_L2_1_x16384>
#ciphertext_space_L0 = #lwe.ciphertext_space<ring = #ring_rns_L0_1_x16384, encryption_type = mix>
#ciphertext_space_L1 = #lwe.ciphertext_space<ring = #ring_rns_L1_1_x16384, encryption_type = mix>
#ciphertext_space_L2 = #lwe.ciphertext_space<ring = #ring_rns_L2_1_x16384, encryption_type = mix>
#ciphertext_space_L2_D3 = #lwe.ciphertext_space<ring = #ring_rns_L2_1_x16384, encryption_type = mix, size = 3>
!ct_L0 = !lwe.lwe_ciphertext<plaintext_space = <ring = #ring_f64_1_x16384, encoding = #inverse_canonical_encoding>, ciphertext_space = #ciphertext_space_L0, key = #key, modulus_chain = #modulus_chain_L2_C0>
!ct_L1 = !lwe.lwe_ciphertext<plaintext_space = <ring = #ring_f64_1_x16384, encoding = #inverse_canonical_encoding>, ciphertext_space = #ciphertext_space_L1, key = #key, modulus_chain = #modulus_chain_L2_C1>
!ct_L1_1 = !lwe.lwe_ciphertext<plaintext_space = <ring = #ring_f64_1_x16384, encoding = #inverse_canonical_encoding1>, ciphertext_space = #ciphertext_space_L1, key = #key, modulus_chain = #modulus_chain_L2_C1>
!ct_L2 = !lwe.lwe_ciphertext<plaintext_space = <ring = #ring_f64_1_x16384, encoding = #inverse_canonical_encoding>, ciphertext_space = #ciphertext_space_L2, key = #key, modulus_chain = #modulus_chain_L2_C2>
!ct_L2_1 = !lwe.lwe_ciphertext<plaintext_space = <ring = #ring_f64_1_x16384, encoding = #inverse_canonical_encoding1>, ciphertext_space = #ciphertext_space_L2, key = #key, modulus_chain = #modulus_chain_L2_C2>
!ct_L2_D3 = !lwe.lwe_ciphertext<plaintext_space = <ring = #ring_f64_1_x16384, encoding = #inverse_canonical_encoding1>, ciphertext_space = #ciphertext_space_L2_D3, key = #key, modulus_chain = #modulus_chain_L2_C2>

module attributes {backend.poulpy, ckks.schemeParam = #ckks.scheme_param<logN = 14, Q = [36028797017456641, 35184371138561, 35184372121601], P = [1152921504607338497, 1152921504608747521], logDefaultScale = 45, encryptionType = sk>, scheme.ckks} {
  // CHECK: func.func @arith(%[[M:.*]]: !poulpy.module, %[[S:.*]]: !poulpy.scratch, %[[A:.*]]: tensor<!poulpy.ciphertext>, %[[B:.*]]: tensor<!poulpy.ciphertext>) -> tensor<!poulpy.ciphertext>
  // CHECK: %[[E0:.*]] = tensor.empty() : tensor<!poulpy.ciphertext>
  // CHECK-NEXT: %[[SUM:.*]] = poulpy.add %[[M]], %[[E0]], %[[A]], %[[B]], %[[S]]
  // CHECK: %[[DIFF:.*]] = poulpy.sub %[[M]], %{{.*}}, %[[SUM]], %[[B]], %[[S]]
  // CHECK: %[[NEG:.*]] = poulpy.neg %[[M]], %{{.*}}, %[[DIFF]], %[[S]]
  // The level reduction is dropped: Poulpy aligns operands itself.
  // CHECK-NOT: level_reduce
  // CHECK: return %[[NEG]]
  func.func @arith(%a: !ct_L1, %b: !ct_L1) -> !ct_L0 {
    %0 = lwe.radd %a, %b : (!ct_L1, !ct_L1) -> !ct_L1
    %1 = lwe.rsub %0, %b : (!ct_L1, !ct_L1) -> !ct_L1
    %2 = lwe.rnegate %1 : !ct_L1
    %3 = ckks.level_reduce %2 : !ct_L1 -> !ct_L0
    return %3 : !ct_L0
  }

  // A multiplication, its relinearization and its rescale are one poulpy.mul.
  // CHECK: func.func @mul(%[[M:.*]]: !poulpy.module, %[[S:.*]]: !poulpy.scratch, %[[TSK:.*]]: !poulpy.tensor_key, %[[A:.*]]: tensor<!poulpy.ciphertext>, %[[B:.*]]: tensor<!poulpy.ciphertext>)
  // CHECK: %[[P:.*]] = poulpy.mul %[[M]], %{{.*}}, %[[A]], %[[B]], %[[TSK]], %[[S]]
  // CHECK-NEXT: return %[[P]]
  func.func @mul(%a: !ct_L2, %b: !ct_L2) -> !ct_L1 {
    %0 = lwe.rmul %a, %b : (!ct_L2, !ct_L2) -> !ct_L2_D3
    %1 = ckks.relinearize %0 <from_basis = [0, 1, 2], to_basis = [0, 1]> : (!ct_L2_D3) -> !ct_L2_1
    %2 = ckks.rescale %1 <to_ring = #ring_rns_L1_1_x16384> : !ct_L2_1 -> !ct_L1
    return %2 : !ct_L1
  }

  // Ciphertext-plaintext ops take either operand order; `pt - ct` is
  // `-(ct - pt)`.
  // CHECK: func.func @plain(%[[M:.*]]: !poulpy.module, %[[S:.*]]: !poulpy.scratch, %[[A:.*]]: tensor<!poulpy.ciphertext>, %[[PT:.*]]: tensor<!poulpy.plaintext>)
  // CHECK: %[[R0:.*]] = poulpy.add_pt_vec %[[M]], %{{.*}}, %[[A]], %[[PT]], %[[S]]
  // CHECK: %[[R1:.*]] = poulpy.add_pt_vec %[[M]], %{{.*}}, %[[R0]], %[[PT]], %[[S]]
  // CHECK: %[[R2:.*]] = poulpy.sub_pt_vec %[[M]], %{{.*}}, %[[R1]], %[[PT]], %[[S]]
  // CHECK: %[[R3:.*]] = poulpy.sub_pt_vec %[[M]], %{{.*}}, %[[R2]], %[[PT]], %[[S]]
  // CHECK-NEXT: %[[R4:.*]] = poulpy.neg_assign %[[M]], %[[R3]], %[[S]]
  // CHECK: %[[R5:.*]] = poulpy.mul_pt_vec %[[M]], %{{.*}}, %[[R4]], %[[PT]], %[[S]]
  // CHECK-NEXT: return %[[R5]]
  func.func @plain(%a: !ct_L1, %pt: !pt) -> !ct_L0 {
    %0 = lwe.radd_plain %a, %pt : (!ct_L1, !pt) -> !ct_L1
    %1 = lwe.radd_plain %pt, %0 : (!pt, !ct_L1) -> !ct_L1
    %2 = lwe.rsub_plain %1, %pt : (!ct_L1, !pt) -> !ct_L1
    %3 = lwe.rsub_plain %pt, %2 : (!pt, !ct_L1) -> !ct_L1
    %4 = lwe.rmul_plain %pt, %3 : (!pt, !ct_L1) -> !ct_L1_1
    %5 = ckks.rescale %4 <to_ring = #ring_rns_L0_1_x16384> : !ct_L1_1 -> !ct_L0
    return %5 : !ct_L0
  }

  // CHECK: func.func @rotate(%[[M:.*]]: !poulpy.module, %[[S:.*]]: !poulpy.scratch, %[[AKM:.*]]: !poulpy.automorphism_key_map, %[[A:.*]]: tensor<!poulpy.ciphertext>)
  // CHECK: %[[R0:.*]] = poulpy.rotate %[[M]], %{{.*}}, %[[A]], %[[AKM]], %[[S]] <k = 3>
  // CHECK: poulpy.rotate %[[M]], %{{.*}}, %[[R0]], %[[AKM]], %[[S]] <k = -1>
  func.func @rotate(%a: !ct_L1) -> !ct_L1 {
    %c3 = arith.constant 3 : index
    %0 = ckks.rotate %a, %c3 : index : !ct_L1
    %1 = ckks.rotate %0 <static_shift = -1 : i32> : !ct_L1
    return %1 : !ct_L1
  }

  // A caller passes its context to a callee, and needs what the callee needs.
  // CHECK: func.func @caller(%[[M:.*]]: !poulpy.module, %[[S:.*]]: !poulpy.scratch, %[[TSK:.*]]: !poulpy.tensor_key, %[[A:.*]]: tensor<!poulpy.ciphertext>, %[[B:.*]]: tensor<!poulpy.ciphertext>)
  // CHECK: call @mul(%[[M]], %[[S]], %[[TSK]], %[[A]], %[[B]])
  func.func @caller(%a: !ct_L2, %b: !ct_L2) -> !ct_L1 {
    %0 = call @mul(%a, %b) : (!ct_L2, !ct_L2) -> !ct_L1
    return %0 : !ct_L1
  }

  // Encoding widens the cleartext to the real parts of as many slots, at the
  // plaintext's level; the unit tensor of ciphertexts is the ciphertext.
  // CHECK: func.func @encrypt(%[[M:.*]]: !poulpy.module, %[[S:.*]]: !poulpy.scratch, %[[V:.*]]: tensor<8xf32>, %[[SK:.*]]: !poulpy.secret_key) -> tensor<!poulpy.ciphertext>
  // CHECK: %[[RE:.*]] = arith.extf %[[V]] : tensor<8xf32> to tensor<8xf64>
  // CHECK: %[[IM:.*]] = arith.constant dense<0.000000e+00> : tensor<8xf64>
  // CHECK: %[[PT:.*]] = poulpy.encode %[[M]], %{{.*}}, %[[RE]], %[[IM]], %[[S]] <logDelta = 45, logBudget = 100, base2k = 52>
  // CHECK: %[[CT:.*]] = poulpy.encrypt %[[M]], %{{.*}}, %[[PT]], %[[SK]], %[[S]] <base2k = 52, ctk = 145>
  // CHECK-NEXT: return %[[CT]]
  func.func @encrypt(%v: tensor<8xf32>, %sk: !skey_L2) -> tensor<1x!ct_L2> {
    %pt = lwe.rlwe_encode %v <encoding = #inverse_canonical_encoding, ring = #ring_f64_1_x16384, level = 2, scale = 45> : tensor<8xf32> -> !pt
    %ct = lwe.rlwe_encrypt %pt, %sk : (!pt, !skey_L2) -> !ct_L2
    %t = tensor.from_elements %ct : tensor<1x!ct_L2>
    return %t : tensor<1x!ct_L2>
  }

  // Decoding keeps the real parts, narrowed and reshaped to the cleartext.
  // CHECK: func.func @decrypt(%[[M:.*]]: !poulpy.module, %[[S:.*]]: !poulpy.scratch, %[[CT:.*]]: tensor<!poulpy.ciphertext>, %[[SK:.*]]: !poulpy.secret_key) -> tensor<1x8xf32>
  // CHECK: %[[PT:.*]] = poulpy.decrypt %[[M]], %{{.*}}, %[[CT]], %[[SK]], %[[S]]
  // CHECK: %[[SLOTS:.*]]:2 = poulpy.decode %[[M]], %{{.*}}, %{{.*}}, %[[PT]], %[[S]]
  // CHECK-NEXT: %[[RE:.*]] = arith.truncf %[[SLOTS]]#0 : tensor<8xf64> to tensor<8xf32>
  // CHECK-NEXT: %[[OUT:.*]] = tensor.expand_shape %[[RE]] {{\[\[}}0, 1]] output_shape [1, 8]
  // CHECK-NEXT: return %[[OUT]]
  func.func @decrypt(%t: tensor<1x!ct_L0>, %sk: !skey_L0) -> tensor<1x8xf32> {
    %c0 = arith.constant 0 : index
    %ct = tensor.extract %t[%c0] : tensor<1x!ct_L0>
    %pt = lwe.rlwe_decrypt %ct, %sk : (!ct_L0, !skey_L0) -> !pt
    %v = lwe.rlwe_decode %pt <encoding = #inverse_canonical_encoding, ring = #ring_f64_1_x16384> : !pt -> tensor<1x8xf32>
    return %v : tensor<1x8xf32>
  }
}

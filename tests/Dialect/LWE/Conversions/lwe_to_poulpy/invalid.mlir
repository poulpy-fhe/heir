// RUN: heir-opt --split-input-file --lwe-to-poulpy --verify-diagnostics %s

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
!pkey_L2 = !lwe.lwe_public_key<key = #key, ring = #ring_rns_L2_1_x16384>

module attributes {backend.poulpy, ckks.schemeParam = #ckks.scheme_param<logN = 14, Q = [36028797017456641, 35184371138561, 35184372121601], P = [1152921504607338497, 1152921504608747521], logDefaultScale = 45, encryptionType = sk>, scheme.ckks} {
  // expected-error@below {{public-key encryption is not supported}}
  func.func @public_key(%pt: !pt, %pk: !pkey_L2) -> !ct_L2 {
    %ct = lwe.rlwe_encrypt %pt, %pk : (!pt, !pkey_L2) -> !ct_L2
    return %ct : !ct_L2
  }
}

// -----

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
  // expected-error@below {{tensors of more than one ciphertext or plaintext are not supported yet}}
  func.func @two_ciphertexts(%t: tensor<2x!ct_L1>) -> tensor<2x!ct_L1> {
    return %t : tensor<2x!ct_L1>
  }
}

// -----

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
  func.func @lazy_relinearization(%a: !ct_L2, %b: !ct_L2) -> !ct_L2_1 {
    // expected-error@below {{a ciphertext multiplication must be relinearized right away}}
    %0 = lwe.rmul %a, %b : (!ct_L2, !ct_L2) -> !ct_L2_D3
    %1 = lwe.radd %0, %0 : (!ct_L2_D3, !ct_L2_D3) -> !ct_L2_D3
    %2 = ckks.relinearize %1 <from_basis = [0, 1, 2], to_basis = [0, 1]> : (!ct_L2_D3) -> !ct_L2_1
    return %2 : !ct_L2_1
  }
}

// -----

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
  func.func @lone_rescale(%a: !ct_L2_1) -> !ct_L1 {
    // expected-error@below {{rescale is a no-op in Poulpy and must directly follow a multiplication}}
    %0 = ckks.rescale %a <to_ring = #ring_rns_L1_1_x16384> : !ct_L2_1 -> !ct_L1
    return %0 : !ct_L1
  }
}

// -----

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
  func.func @dynamic_rotation(%a: !ct_L1, %shift: index) -> !ct_L1 {
    // expected-error@below {{rotations by a non-constant shift are not supported}}
    %0 = ckks.rotate %a, %shift : index : !ct_L1
    return %0 : !ct_L1
  }
}

// -----

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

// expected-error@below {{lwe-to-poulpy needs CKKS scheme parameters}}
module attributes {backend.poulpy, scheme.ckks} {
  func.func @no_scheme_param(%a: !ct_L1) -> !ct_L1 {
    return %a : !ct_L1
  }
}

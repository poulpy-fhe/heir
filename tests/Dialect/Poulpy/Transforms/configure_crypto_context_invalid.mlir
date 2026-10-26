// RUN: heir-opt --poulpy-configure-crypto-context --split-input-file --verify-diagnostics %s

// expected-error@+1 {{needs CKKS scheme parameters}}
module {
  func.func @f(%m: !poulpy.module, %s: !poulpy.scratch) {
    return
  }
}

// -----

// The 145-bit ciphertexts need a ring degree of at least 2^13.
// expected-error@+1 {{a 145-bit Poulpy modulus is not 128-bit secure at ring degree 2^12}}
module attributes {ckks.schemeParam = #ckks.scheme_param<logN = 12, Q = [36028797017456641, 35184371138561, 35184372121601], P = [1152921504607338497], logDefaultScale = 45>} {
  func.func @f(%m: !poulpy.module, %s: !poulpy.scratch) {
    return
  }
}

// -----

// 370-bit ciphertexts fit 2^14, but their keys take 8 limbs of 52 bits plus a
// 66-bit guard: 482 bits.
// expected-error@+1 {{a 482-bit Poulpy modulus is not 128-bit secure at ring degree 2^14}}
module attributes {ckks.schemeParam = #ckks.scheme_param<logN = 14, Q = [36028797017456641, 35184371138561, 35184372121601, 35184371138561, 35184372121601, 35184371138561, 35184372121601, 35184371138561], P = [1152921504607338497], logDefaultScale = 45>} {
  func.func @f(%m: !poulpy.module, %s: !poulpy.scratch, %tsk: !poulpy.tensor_key) {
    return
  }
}

// RUN: heir-opt --optimize-relinearization %s | FileCheck %s

// Poulpy's ciphertext multiplication always relinearizes, so the target does not
// support lazy relinearization: each relinearization stays right after its
// multiplication instead of moving past the addition.

// CHECK: func.func @two_muls_followed_by_add
// CHECK: %[[M0:.*]] = arith.muli
// CHECK-NEXT: %[[R0:.*]] = mgmt.relinearize %[[M0]]
// CHECK-NEXT: %[[M1:.*]] = arith.muli
// CHECK-NEXT: %[[R1:.*]] = mgmt.relinearize %[[M1]]
// CHECK-NEXT: %[[SUM:.*]] = arith.addi %[[R0]], %[[R1]]
// CHECK-NEXT: secret.yield %[[SUM]]
module attributes {backend.poulpy, scheme.ckks} {
  func.func @two_muls_followed_by_add(%arg0: !secret.secret<tensor<8xi16>>, %arg1: !secret.secret<tensor<8xi16>>, %arg2: !secret.secret<tensor<8xi16>>, %arg3: !secret.secret<tensor<8xi16>>) -> !secret.secret<tensor<8xi16>> {
    %0 = secret.generic(%arg0: !secret.secret<tensor<8xi16>>, %arg1: !secret.secret<tensor<8xi16>>, %arg2: !secret.secret<tensor<8xi16>>, %arg3: !secret.secret<tensor<8xi16>>) {
    ^body(%input0: tensor<8xi16>, %input1: tensor<8xi16>, %input2: tensor<8xi16>, %input3: tensor<8xi16>):
      %1 = arith.muli %input0, %input1 : tensor<8xi16>
      %2 = mgmt.relinearize %1 : tensor<8xi16>
      %3 = arith.muli %input2, %input3 : tensor<8xi16>
      %4 = mgmt.relinearize %3 : tensor<8xi16>
      %5 = arith.addi %2, %4 : tensor<8xi16>
      secret.yield %5 : tensor<8xi16>
    } -> !secret.secret<tensor<8xi16>>
    return %0 : !secret.secret<tensor<8xi16>>
  }
}

// RUN: heir-opt --secret-insert-mgmt-ckks="min-slot-count=8" %s | FileCheck %s

// Poulpy has one canonical scale per level: a product already comes out at its
// operands' scale. Every multiplication is therefore relinearized and rescaled
// right away, which lowers to a single fused poulpy.mul, and a cross-level
// addition lowers the higher operand with level_reduce, without an adjust_scale
// that the target cannot emit.

// CHECK: func @cross_level
// CHECK-NOT: mgmt.adjust_scale
// CHECK: %[[M1:.*]] = arith.mulf
// CHECK-NEXT: %[[R1:.*]] = mgmt.relinearize %[[M1]]
// CHECK-NEXT: %[[S1:.*]] = mgmt.modreduce %[[R1]]
// CHECK-NEXT: %[[M2:.*]] = arith.mulf %[[S1]], %[[S1]]
// CHECK-NEXT: %[[R2:.*]] = mgmt.relinearize %[[M2]]
// CHECK-NEXT: %[[S2:.*]] = mgmt.modreduce %[[R2]]
// CHECK-NEXT: %[[L1:.*]] = mgmt.level_reduce %[[S1]]
// CHECK-NEXT: arith.addf %[[S2]], %[[L1]]
module attributes {backend.poulpy, scheme.ckks} {
  func.func @cross_level(
      %arg0: !secret.secret<tensor<8xf32>>,
      %arg1: !secret.secret<tensor<8xf32>>) -> !secret.secret<tensor<8xf32>> {
    %0 = secret.generic(%arg0 : !secret.secret<tensor<8xf32>>, %arg1 : !secret.secret<tensor<8xf32>>) {
    ^body(%input0: tensor<8xf32>, %input1: tensor<8xf32>):
      %m1 = arith.mulf %input0, %input1 : tensor<8xf32>
      %m2 = arith.mulf %m1, %m1 : tensor<8xf32>
      %result = arith.addf %m2, %m1 : tensor<8xf32>
      secret.yield %result : tensor<8xf32>
    } -> !secret.secret<tensor<8xf32>>
    return %0 : !secret.secret<tensor<8xf32>>
  }
}

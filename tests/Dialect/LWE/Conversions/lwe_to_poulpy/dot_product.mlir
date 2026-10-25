// RUN: heir-opt --annotate-module="backend=poulpy scheme=ckks" --mlir-to-ckks="min-slot-count=8 use-public-key=false enable-split-preprocessing=false" --ckks-to-lwe --lwe-to-poulpy %s | FileCheck %s

// HEIR's CKKS pipeline under the poulpy target relinearizes and rescales each
// multiplication right away, so it lowers to a single poulpy.mul.

// CHECK: func.func @dot_product(%[[M:.*]]: !poulpy.module, %[[S:.*]]: !poulpy.scratch, %[[TSK:.*]]: !poulpy.tensor_key, %[[AKM:.*]]: !poulpy.automorphism_key_map, %[[A:[a-z0-9]+]]: tensor<!poulpy.ciphertext>
// CHECK-NOT: lwe.
// CHECK-NOT: ckks.
// CHECK: poulpy.mul %[[M]], %{{.*}}, %[[A]], %{{.*}}, %[[TSK]], %[[S]]
// CHECK: poulpy.rotate {{.*}} <k = 1>
// CHECK: poulpy.rotate {{.*}} <k = 2>
// CHECK: poulpy.rotate {{.*}} <k = 4>
// CHECK: poulpy.add_pt_vec
// CHECK: poulpy.mul_pt_vec
// CHECK: return {{.*}} : tensor<!poulpy.ciphertext>

// CHECK: func.func @dot_product__encrypt__arg0(%{{.*}}: !poulpy.module, %{{.*}}: !poulpy.scratch, %{{.*}}: tensor<8xf32>, %{{.*}}: !poulpy.secret_key) -> tensor<!poulpy.ciphertext>
// CHECK: poulpy.encrypt {{.*}} <base2k = 52, ctk = 145>

// CHECK: func.func @dot_product__decrypt__result0(%{{.*}}: !poulpy.module, %{{.*}}: !poulpy.scratch, %{{.*}}: tensor<!poulpy.ciphertext>, %{{.*}}: !poulpy.secret_key) -> f32
// CHECK: poulpy.decrypt
// CHECK: poulpy.decode

func.func @dot_product(%arg0: tensor<8xf32> {secret.secret}, %arg1: tensor<8xf32> {secret.secret}) -> f32 {
  %cst = arith.constant dense<1.000000e-01> : tensor<f32>
  %0 = linalg.dot ins(%arg0, %arg1 : tensor<8xf32>, tensor<8xf32>) outs(%cst : tensor<f32>) -> tensor<f32>
  %1 = tensor.extract %0[] : tensor<f32>
  return %1 : f32
}

// RUN: heir-opt --annotate-module="backend=poulpy scheme=ckks" --mlir-to-ckks="min-slot-count=8 use-public-key=false enable-split-preprocessing=false" %s \
// RUN:   | heir-opt --scheme-to-poulpy | FileCheck %s

// CHECK: func.func @dot_product(%{{.*}}: !poulpy.module, %{{.*}}: !poulpy.scratch, %{{.*}}: !poulpy.tensor_key, %{{.*}}: !poulpy.automorphism_key_map, %{{.*}}: memref<!poulpy.ciphertext>
// CHECK: poulpy.mul
// CHECK: poulpy.rotate
// CHECK: return {{.*}} : memref<!poulpy.ciphertext>

// CHECK: func.func @dot_product__encrypt__arg0(%{{.*}}: !poulpy.module, %{{.*}}: !poulpy.scratch, %{{.*}}: memref<8xf32>, %{{.*}}: !poulpy.secret_key) -> memref<!poulpy.ciphertext>
// CHECK: poulpy.encode
// CHECK: poulpy.encrypt

// CHECK: func.func @dot_product__decrypt__result0(%{{.*}}: !poulpy.module, %{{.*}}: !poulpy.scratch, %{{.*}}: memref<!poulpy.ciphertext>, %{{.*}}: !poulpy.secret_key) -> f32
// CHECK: poulpy.decrypt
// CHECK: poulpy.decode

// CHECK: func.func @dot_product__configure() -> (!poulpy.module, !poulpy.scratch, !poulpy.tensor_key, !poulpy.automorphism_key_map, !poulpy.secret_key)
// CHECK: poulpy.tensor_key_generate
// CHECK: poulpy.automorphism_keys_generate

func.func @dot_product(%arg0: tensor<8xf32> {secret.secret}, %arg1: tensor<8xf32> {secret.secret}) -> f32 {
  %cst = arith.constant dense<1.000000e-01> : tensor<f32>
  %0 = linalg.dot ins(%arg0, %arg1 : tensor<8xf32>, tensor<8xf32>) outs(%cst : tensor<f32>) -> tensor<f32>
  %1 = tensor.extract %0[] : tensor<f32>
  return %1 : f32
}

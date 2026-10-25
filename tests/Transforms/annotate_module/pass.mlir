// RUN: heir-opt --annotate-module="backend=openfhe scheme=ckks" %s | FileCheck %s
// RUN: heir-opt --annotate-module="backend=cheddar scheme=ckks" %s | FileCheck %s --check-prefix=CHECK-CHEDDAR
// RUN: heir-opt --annotate-module="backend=poulpy scheme=ckks" %s | FileCheck %s --check-prefix=CHECK-POULPY

// CHECK: module attributes {backend.openfhe, scheme.ckks}
// CHECK-CHEDDAR: module attributes {backend.cheddar, scheme.ckks}
// CHECK-POULPY: module attributes {backend.poulpy, scheme.ckks}
module {

}

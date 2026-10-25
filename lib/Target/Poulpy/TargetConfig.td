include "lib/Target/CompilationTarget/HEIRTarget.td"

def Poulpy : CompilationTarget<"poulpy"> {
  // A product already comes out at its operands' scale (rescale is a no-op in
  // Poulpy), so there is one canonical scale per level and no operation for a
  // same-level scale correction.
  let supportsAdjustScale = 0;

  // A ciphertext-ciphertext multiplication always relinearizes: it is a fused
  // multiply, relinearize and rescale.
  let supportsLazyRelinearization = 0;
}

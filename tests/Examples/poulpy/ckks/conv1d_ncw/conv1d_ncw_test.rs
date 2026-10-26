use conv1d_ncw::*;
use poulpy_cpu_portable::NTT4x30Portable;

#[test]
fn conv1d_ncw_matches_cleartext() {
    let (module, mut scratch, akm, sk) = conv1d_ncw__configure::<NTT4x30Portable>().unwrap();

    let input = [0f32, 1.0, 2.0, 3.0];
    // Each of the 8 output channels sums windows of 2 with stride 2.
    let expected: Vec<f32> = (0..8).flat_map(|_| [1f32, 5.0]).collect();

    let ct = conv1d_ncw__encrypt__arg0(&module, &mut scratch, &input, &sk).unwrap();
    let result_ct = conv1d_ncw(&module, &mut scratch, &akm, &ct).unwrap();
    let result = conv1d_ncw__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    for (i, (got, want)) in result.iter().zip(&expected).enumerate() {
        assert!((got - want).abs() < 0.5, "index {i}: {got} != {want}");
    }
}

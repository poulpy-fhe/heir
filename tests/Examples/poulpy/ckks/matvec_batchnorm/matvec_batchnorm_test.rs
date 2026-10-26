use matvec_batchnorm::*;
use poulpy_cpu_portable::NTT4x30Portable;

#[test]
fn matvec_batchnorm_matches_cleartext() {
    let (module, mut scratch, akm, sk) = matvec_batchnorm__configure::<NTT4x30Portable>().unwrap();

    let input = [1f32, 2.0, 3.0, 4.0];
    let scale = [2f32; 4];
    let shift = [1f32; 4];
    let expected = [61f32, 141.0, 221.0, 301.0];

    let ct = matvec_batchnorm__encrypt__arg0(&module, &mut scratch, &input, &sk).unwrap();
    let result_ct = matvec_batchnorm(&module, &mut scratch, &akm, &ct, &scale, &shift).unwrap();
    let result =
        matvec_batchnorm__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    for (i, (got, want)) in result.iter().zip(&expected).enumerate() {
        assert!((got - want).abs() < 1e-3, "index {i}: {got} != {want}");
    }
}

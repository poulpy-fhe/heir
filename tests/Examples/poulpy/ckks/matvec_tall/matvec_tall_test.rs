use matvec_tall::*;
use poulpy_cpu_portable::NTT4x30Portable;

#[test]
fn matvec_tall_matches_cleartext() {
    let (module, mut scratch, akm, sk) = matvec__configure::<NTT4x30Portable>().unwrap();

    let input = [1f32, 2.0, 3.0, 4.0];
    let expected = [20f32, 60.0, 100.0, 140.0, 180.0, 220.0, 260.0, 300.0];

    let ct = matvec__encrypt__arg0(&module, &mut scratch, &input, &sk).unwrap();
    let result_ct = matvec(&module, &mut scratch, &akm, &ct).unwrap();
    let result = matvec__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    for (i, (got, want)) in result.iter().zip(&expected).enumerate() {
        assert!((got - want).abs() < 0.5, "index {i}: {got} != {want}");
    }
}

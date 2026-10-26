use linalg_dot::*;
use poulpy_cpu_portable::NTT4x30Portable;

#[test]
fn linalg_dot_matches_cleartext() {
    let (module, mut scratch, tsk, akm, sk) = dot_product__configure::<NTT4x30Portable>().unwrap();

    let arg: Vec<f32> = (0..1024).map(|i| i as f32 / 1024.0).collect();
    let expected: f32 = arg.iter().map(|x| x * x).sum();

    let ct0 = dot_product__encrypt__arg0(&module, &mut scratch, &arg, &sk).unwrap();
    let ct1 = dot_product__encrypt__arg1(&module, &mut scratch, &arg, &sk).unwrap();
    let result_ct = dot_product(&module, &mut scratch, &tsk, &akm, &ct0, &ct1).unwrap();
    let result = dot_product__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    assert!(
        (result[0] - expected).abs() < 1e-3,
        "{} != {expected}",
        result[0]
    );
}

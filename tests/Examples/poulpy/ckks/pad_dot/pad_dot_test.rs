use pad_dot::*;
use poulpy_cpu_portable::NTT4x30Portable;

#[test]
fn pad_dot_matches_cleartext() {
    let (module, mut scratch, tsk, akm, sk) = pad_dot__configure::<NTT4x30Portable>().unwrap();

    let arg0 = [0.1f32, 0.2, 0.3, 0.4, 0.5];
    let arg1 = [0.2f32, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9];
    let expected = 1.0f32;

    let ct0 = pad_dot__encrypt__arg0(&module, &mut scratch, &arg0, &sk).unwrap();
    let ct1 = pad_dot__encrypt__arg1(&module, &mut scratch, &arg1, &sk).unwrap();
    let result_ct = pad_dot(&module, &mut scratch, &tsk, &akm, &ct0, &ct1).unwrap();
    let result = pad_dot__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    assert!((result - expected).abs() < 1e-4, "{result} != {expected}");
}

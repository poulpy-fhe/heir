use poulpy_cpu_portable::NTT4x30Portable;
use softmax::*;

#[test]
fn softmax_matches_cleartext() {
    let (module, mut scratch, tsk, akm, sk) = softmax__configure::<NTT4x30Portable>().unwrap();

    let input = [-0.8f32, -0.5, -0.2, 0.0, 0.2, 0.5, 0.8, 1.0];
    let sum: f64 = input.iter().map(|x| (*x as f64).exp()).sum();
    let expected: Vec<f32> = input
        .iter()
        .map(|x| ((*x as f64).exp() / sum) as f32)
        .collect();

    let ct = softmax__encrypt__arg0(&module, &mut scratch, &input, &sk).unwrap();
    let result_ct = softmax(&module, &mut scratch, &tsk, &akm, &ct).unwrap();
    let result = softmax__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    // The CGF softmax is an approximation.
    for (i, (got, want)) in result.iter().zip(&expected).enumerate() {
        assert!((got - want).abs() < 0.08, "index {i}: {got} != {want}");
    }
}

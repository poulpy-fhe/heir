use poulpy_cpu_portable::NTT4x30Portable;
use sigmoid::*;

#[test]
fn sigmoid_matches_cleartext() {
    let (module, mut scratch, tsk, sk) = sigmoid__configure::<NTT4x30Portable>().unwrap();

    // 1x1x32x32 values in [-3, 3).
    let input: Vec<f32> = (0..1024).map(|i| -3.0 + 6.0 * i as f32 / 1024.0).collect();
    let expected: Vec<f32> = input
        .iter()
        .map(|x| (1.0 / (1.0 + (-*x as f64).exp())) as f32)
        .collect();

    let ct = sigmoid__encrypt__arg0(&module, &mut scratch, &input, &sk).unwrap();
    let result_ct = sigmoid(&module, &mut scratch, &tsk, &ct).unwrap();
    let result = sigmoid__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    for (i, (got, want)) in result.iter().zip(&expected).enumerate() {
        assert!((got - want).abs() < 0.01, "index {i}: {got} != {want}");
    }
}

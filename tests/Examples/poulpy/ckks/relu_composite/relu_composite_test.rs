use poulpy_cpu_portable::NTT4x30Portable;
use relu_composite::*;

#[test]
fn relu_composite_matches_cleartext() {
    let (module, mut scratch, tsk, sk) = relu_composite__configure::<NTT4x30Portable>().unwrap();

    let input: Vec<f32> = (0..16).map(|i| i as f32 * 0.25 - 2.0).collect();
    let expected: Vec<f32> = input.iter().map(|x| x.max(0.0)).collect();

    let ct = relu_composite__encrypt__arg0(&module, &mut scratch, &input, &sk).unwrap();
    let result_ct = relu_composite(&module, &mut scratch, &tsk, &ct).unwrap();
    let result = relu_composite__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    // The composite sign rounds the kink at 0, but the result is x * step(x),
    // so the error stays small there.
    for (i, (got, want)) in result.iter().zip(&expected).enumerate() {
        assert!(
            (got - want).abs() < 0.01,
            "index {i} (x = {}): {got} != {want}",
            input[i]
        );
    }
}

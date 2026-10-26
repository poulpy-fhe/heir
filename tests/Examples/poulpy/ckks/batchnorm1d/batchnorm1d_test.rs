use batchnorm1d::*;
use poulpy_cpu_portable::NTT4x30Portable;

#[test]
fn batchnorm1d_matches_cleartext() {
    let (module, mut scratch, sk) = batchnorm1d__configure::<NTT4x30Portable>().unwrap();

    let mean = [0.1f32, 0.2, 0.3];
    let var = [0.4f32, 0.5, 0.6];
    let data: Vec<f32> = (0..48).map(|i| i as f32 * 0.1).collect();
    // The model's weight is 1 and its bias 0.
    let expected: Vec<f32> = (0..48)
        .map(|i| (data[i] - mean[i / 16]) / (var[i / 16] + 1e-5).sqrt())
        .collect();

    let ct = batchnorm1d__encrypt__arg3(&module, &mut scratch, &data, &sk).unwrap();
    // The function updates the variance buffer in place.
    let mut var_buffer = var;
    let result_ct =
        batchnorm1d(&module, &mut scratch, &mean, &mut var_buffer, &[0i64], &ct).unwrap();
    let result = batchnorm1d__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    for (i, (got, want)) in result.iter().zip(&expected).enumerate() {
        assert!((got - want).abs() < 1e-2, "index {i}: {got} != {want}");
    }
}

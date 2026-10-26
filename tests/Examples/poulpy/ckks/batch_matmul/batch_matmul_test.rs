use batch_matmul::*;
use poulpy_cpu_portable::NTT4x30Portable;

#[test]
fn batch_matmul_matches_cleartext() {
    let (module, mut scratch, tsk, akm, sk) = batch_matmul__configure::<NTT4x30Portable>().unwrap();

    let mut q = vec![0f32; 2 * 17 * 19];
    for b in 0..2 {
        for i in 0..17 {
            for j in 0..19 {
                q[b * 17 * 19 + i * 19 + j] = (b + i + j) as f32 / 100.0;
            }
        }
    }
    let mut k = vec![0f32; 2 * 19 * 21];
    for b in 0..2 {
        for j in 0..19 {
            for l in 0..21 {
                k[b * 19 * 21 + j * 21 + l] = (b as f32 + j as f32 - l as f32) / 100.0;
            }
        }
    }
    let mut expected = vec![0f32; 2 * 17 * 21];
    for b in 0..2 {
        for i in 0..17 {
            for l in 0..21 {
                for j in 0..19 {
                    expected[b * 17 * 21 + i * 21 + l] +=
                        q[b * 17 * 19 + i * 19 + j] * k[b * 19 * 21 + j * 21 + l];
                }
            }
        }
    }

    let q_ct = batch_matmul__encrypt__arg0(&module, &mut scratch, &q, &sk).unwrap();
    let k_ct = batch_matmul__encrypt__arg1(&module, &mut scratch, &k, &sk).unwrap();
    let result_ct = batch_matmul(&module, &mut scratch, &tsk, &akm, &q_ct, &k_ct).unwrap();
    let result = batch_matmul__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    for (i, (got, want)) in result.iter().zip(&expected).enumerate() {
        assert!((got - want).abs() < 1e-2, "index {i}: {got} != {want}");
    }
}

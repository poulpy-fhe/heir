use bicyclic_matmul::*;
use poulpy_cpu_portable::NTT4x30Portable;

#[test]
fn bicyclic_matmul_matches_cleartext() {
    let (module, mut scratch, tsk, akm, sk) =
        bicyclic_matmul__configure::<NTT4x30Portable>().unwrap();

    let mut q = vec![0f32; 16 * 17];
    for i in 0..16 {
        for j in 0..17 {
            q[i * 17 + j] = (i + j) as f32 / 100.0;
        }
    }
    let mut k = vec![0f32; 17 * 19];
    for j in 0..17 {
        for l in 0..19 {
            k[j * 19 + l] = (j as f32 - l as f32) / 100.0;
        }
    }
    let mut expected = vec![0f32; 16 * 19];
    for i in 0..16 {
        for l in 0..19 {
            for j in 0..17 {
                expected[i * 19 + l] += q[i * 17 + j] * k[j * 19 + l];
            }
        }
    }

    let q_ct = bicyclic_matmul__encrypt__arg0(&module, &mut scratch, &q, &sk).unwrap();
    let k_ct = bicyclic_matmul__encrypt__arg1(&module, &mut scratch, &k, &sk).unwrap();
    let result_ct = bicyclic_matmul(&module, &mut scratch, &tsk, &akm, &q_ct, &k_ct).unwrap();
    let result = bicyclic_matmul__decrypt__result0(&module, &mut scratch, &result_ct, &sk).unwrap();

    for (i, (got, want)) in result.iter().zip(&expected).enumerate() {
        assert!((got - want).abs() < 1e-2, "index {i}: {got} != {want}");
    }
}

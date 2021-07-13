static TCGv gen_mulu_i64_i32(TCGv a, TCGv b)

{

    TCGv tmp1 = tcg_temp_new(TCG_TYPE_I64);

    TCGv tmp2 = tcg_temp_new(TCG_TYPE_I64);



    tcg_gen_extu_i32_i64(tmp1, a);

    dead_tmp(a);

    tcg_gen_extu_i32_i64(tmp2, b);

    dead_tmp(b);

    tcg_gen_mul_i64(tmp1, tmp1, tmp2);

    return tmp1;

}

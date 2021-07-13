int qcrypto_cipher_decrypt(QCryptoCipher *cipher,

                           const void *in,

                           void *out,

                           size_t len,

                           Error **errp)

{

    QCryptoCipherNettle *ctx = cipher->opaque;



    switch (cipher->mode) {

    case QCRYPTO_CIPHER_MODE_ECB:

        ctx->alg_decrypt(ctx->ctx_decrypt ? ctx->ctx_decrypt : ctx->ctx_encrypt,

                         len, out, in);

        break;



    case QCRYPTO_CIPHER_MODE_CBC:

        cbc_decrypt(ctx->ctx_decrypt ? ctx->ctx_decrypt : ctx->ctx_encrypt,

                    ctx->alg_decrypt, ctx->niv, ctx->iv,

                    len, out, in);

        break;

    default:

        error_setg(errp, "Unsupported cipher algorithm %d",

                   cipher->alg);

        return -1;

    }

    return 0;

}

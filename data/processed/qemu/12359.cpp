int bdrv_check(BlockDriverState *bs, BdrvCheckResult *res)

{

    if (bs->drv->bdrv_check == NULL) {

        return -ENOTSUP;

    }



    memset(res, 0, sizeof(*res));

    return bs->drv->bdrv_check(bs, res);

}

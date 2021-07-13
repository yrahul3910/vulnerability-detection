static int jpeg2000_decode_packets_po_iteration(Jpeg2000DecoderContext *s, Jpeg2000Tile *tile,

                                             int RSpoc, int CSpoc,

                                             int LYEpoc, int REpoc, int CEpoc,

                                             int Ppoc)

{

    int ret = 0;

    int layno, reslevelno, compno, precno, ok_reslevel;

    int x, y;

    int tp_index = 0;

    int step_x, step_y;



    switch (Ppoc) {

    case JPEG2000_PGOD_RLCP:

        av_log(s->avctx, AV_LOG_DEBUG, "Progression order RLCP\n");

        ok_reslevel = 1;

        for (reslevelno = RSpoc; ok_reslevel && reslevelno < REpoc; reslevelno++) {

            ok_reslevel = 0;

            for (layno = 0; layno < LYEpoc; layno++) {

                for (compno = CSpoc; compno < CEpoc; compno++) {

                    Jpeg2000CodingStyle *codsty = tile->codsty + compno;

                    Jpeg2000QuantStyle *qntsty  = tile->qntsty + compno;

                    if (reslevelno < codsty->nreslevels) {

                        Jpeg2000ResLevel *rlevel = tile->comp[compno].reslevel +

                                                reslevelno;

                        ok_reslevel = 1;

                        for (precno = 0; precno < rlevel->num_precincts_x * rlevel->num_precincts_y; precno++)

                            if ((ret = jpeg2000_decode_packet(s, tile, &tp_index,

                                                              codsty, rlevel,

                                                              precno, layno,

                                                              qntsty->expn + (reslevelno ? 3 * (reslevelno - 1) + 1 : 0),

                                                              qntsty->nguardbits)) < 0)

                                return ret;

                    }

                }

            }

        }

        break;



    case JPEG2000_PGOD_LRCP:

        av_log(s->avctx, AV_LOG_DEBUG, "Progression order LRCP\n");

        for (layno = 0; layno < LYEpoc; layno++) {

            ok_reslevel = 1;

            for (reslevelno = RSpoc; ok_reslevel && reslevelno < REpoc; reslevelno++) {

                ok_reslevel = 0;

                for (compno = CSpoc; compno < CEpoc; compno++) {

                    Jpeg2000CodingStyle *codsty = tile->codsty + compno;

                    Jpeg2000QuantStyle *qntsty  = tile->qntsty + compno;

                    if (reslevelno < codsty->nreslevels) {

                        Jpeg2000ResLevel *rlevel = tile->comp[compno].reslevel +

                                                reslevelno;

                        ok_reslevel = 1;

                        for (precno = 0; precno < rlevel->num_precincts_x * rlevel->num_precincts_y; precno++)

                            if ((ret = jpeg2000_decode_packet(s, tile, &tp_index,

                                                              codsty, rlevel,

                                                              precno, layno,

                                                              qntsty->expn + (reslevelno ? 3 * (reslevelno - 1) + 1 : 0),

                                                              qntsty->nguardbits)) < 0)

                                return ret;

                    }

                }

            }

        }

        break;



    case JPEG2000_PGOD_CPRL:

        av_log(s->avctx, AV_LOG_DEBUG, "Progression order CPRL\n");

        for (compno = CSpoc; compno < CEpoc; compno++) {

            Jpeg2000Component *comp     = tile->comp + compno;

            Jpeg2000CodingStyle *codsty = tile->codsty + compno;

            Jpeg2000QuantStyle *qntsty  = tile->qntsty + compno;

            step_x = 32;

            step_y = 32;



            for (reslevelno = RSpoc; reslevelno < FFMIN(codsty->nreslevels, REpoc); reslevelno++) {

                uint8_t reducedresno = codsty->nreslevels - 1 -reslevelno; //  ==> N_L - r

                Jpeg2000ResLevel *rlevel = comp->reslevel + reslevelno;

                step_x = FFMIN(step_x, rlevel->log2_prec_width  + reducedresno);

                step_y = FFMIN(step_y, rlevel->log2_prec_height + reducedresno);

            }

            step_x = 1<<step_x;

            step_y = 1<<step_y;



            for (y = tile->coord[1][0]; y < tile->coord[1][1]; y = (y/step_y + 1)*step_y) {

                for (x = tile->coord[0][0]; x < tile->coord[0][1]; x = (x/step_x + 1)*step_x) {

                    for (reslevelno = RSpoc; reslevelno < FFMIN(codsty->nreslevels, REpoc); reslevelno++) {

                        unsigned prcx, prcy;

                        uint8_t reducedresno = codsty->nreslevels - 1 -reslevelno; //  ==> N_L - r

                        Jpeg2000ResLevel *rlevel = comp->reslevel + reslevelno;

                        int xc = x / s->cdx[compno];

                        int yc = y / s->cdy[compno];



                        if (yc % (1 << (rlevel->log2_prec_height + reducedresno)) && y != tile->coord[1][0]) //FIXME this is a subset of the check

                            continue;



                        if (xc % (1 << (rlevel->log2_prec_width + reducedresno)) && x != tile->coord[0][0]) //FIXME this is a subset of the check

                            continue;



                        // check if a precinct exists

                        prcx   = ff_jpeg2000_ceildivpow2(xc, reducedresno) >> rlevel->log2_prec_width;

                        prcy   = ff_jpeg2000_ceildivpow2(yc, reducedresno) >> rlevel->log2_prec_height;

                        prcx  -= ff_jpeg2000_ceildivpow2(comp->coord_o[0][0], reducedresno) >> rlevel->log2_prec_width;

                        prcy  -= ff_jpeg2000_ceildivpow2(comp->coord_o[1][0], reducedresno) >> rlevel->log2_prec_height;



                        precno = prcx + rlevel->num_precincts_x * prcy;



                        if (prcx >= rlevel->num_precincts_x || prcy >= rlevel->num_precincts_y) {

                            av_log(s->avctx, AV_LOG_WARNING, "prc %d %d outside limits %d %d\n",

                                   prcx, prcy, rlevel->num_precincts_x, rlevel->num_precincts_y);

                            continue;

                        }



                        for (layno = 0; layno < LYEpoc; layno++) {

                            if ((ret = jpeg2000_decode_packet(s, tile, &tp_index, codsty, rlevel,

                                                              precno, layno,

                                                              qntsty->expn + (reslevelno ? 3 * (reslevelno - 1) + 1 : 0),

                                                              qntsty->nguardbits)) < 0)

                                return ret;

                        }

                    }

                }

            }

        }

        break;



    case JPEG2000_PGOD_RPCL:

        av_log(s->avctx, AV_LOG_WARNING, "Progression order RPCL\n");

        ok_reslevel = 1;

        for (reslevelno = RSpoc; ok_reslevel && reslevelno < REpoc; reslevelno++) {

            ok_reslevel = 0;

            step_x = 30;

            step_y = 30;

            for (compno = CSpoc; compno < CEpoc; compno++) {

                Jpeg2000Component *comp     = tile->comp + compno;

                Jpeg2000CodingStyle *codsty = tile->codsty + compno;



                if (reslevelno < codsty->nreslevels) {

                    uint8_t reducedresno = codsty->nreslevels - 1 -reslevelno; //  ==> N_L - r

                    Jpeg2000ResLevel *rlevel = comp->reslevel + reslevelno;

                    step_x = FFMIN(step_x, rlevel->log2_prec_width  + reducedresno);

                    step_y = FFMIN(step_y, rlevel->log2_prec_height + reducedresno);

                }

            }

            step_x = 1<<step_x;

            step_y = 1<<step_y;



            for (y = tile->coord[1][0]; y < tile->coord[1][1]; y = (y/step_y + 1)*step_y) {

                for (x = tile->coord[0][0]; x < tile->coord[0][1]; x = (x/step_x + 1)*step_x) {

                    for (compno = CSpoc; compno < CEpoc; compno++) {

                        Jpeg2000Component *comp     = tile->comp + compno;

                        Jpeg2000CodingStyle *codsty = tile->codsty + compno;

                        Jpeg2000QuantStyle *qntsty  = tile->qntsty + compno;

                        uint8_t reducedresno = codsty->nreslevels - 1 -reslevelno; //  ==> N_L - r

                        Jpeg2000ResLevel *rlevel = comp->reslevel + reslevelno;

                        unsigned prcx, prcy;



                        int xc = x / s->cdx[compno];

                        int yc = y / s->cdy[compno];



                        if (reslevelno >= codsty->nreslevels)

                            continue;



                        if (yc % (1 << (rlevel->log2_prec_height + reducedresno)) && y != tile->coord[1][0]) //FIXME this is a subset of the check

                            continue;



                        if (xc % (1 << (rlevel->log2_prec_width + reducedresno)) && x != tile->coord[0][0]) //FIXME this is a subset of the check

                            continue;



                        // check if a precinct exists

                        prcx   = ff_jpeg2000_ceildivpow2(xc, reducedresno) >> rlevel->log2_prec_width;

                        prcy   = ff_jpeg2000_ceildivpow2(yc, reducedresno) >> rlevel->log2_prec_height;

                        prcx  -= ff_jpeg2000_ceildivpow2(comp->coord_o[0][0], reducedresno) >> rlevel->log2_prec_width;

                        prcy  -= ff_jpeg2000_ceildivpow2(comp->coord_o[1][0], reducedresno) >> rlevel->log2_prec_height;



                        precno = prcx + rlevel->num_precincts_x * prcy;



                        ok_reslevel = 1;

                        if (prcx >= rlevel->num_precincts_x || prcy >= rlevel->num_precincts_y) {

                            av_log(s->avctx, AV_LOG_WARNING, "prc %d %d outside limits %d %d\n",

                                   prcx, prcy, rlevel->num_precincts_x, rlevel->num_precincts_y);

                            continue;

                        }



                            for (layno = 0; layno < LYEpoc; layno++) {

                                if ((ret = jpeg2000_decode_packet(s, tile, &tp_index,

                                                                codsty, rlevel,

                                                                precno, layno,

                                                                qntsty->expn + (reslevelno ? 3 * (reslevelno - 1) + 1 : 0),

                                                                qntsty->nguardbits)) < 0)

                                    return ret;

                            }

                    }

                }

            }

        }

        break;



    case JPEG2000_PGOD_PCRL:

        av_log(s->avctx, AV_LOG_WARNING, "Progression order PCRL\n");

        step_x = 32;

        step_y = 32;

        for (compno = CSpoc; compno < CEpoc; compno++) {

            Jpeg2000Component *comp     = tile->comp + compno;

            Jpeg2000CodingStyle *codsty = tile->codsty + compno;

            Jpeg2000QuantStyle *qntsty  = tile->qntsty + compno;



            for (reslevelno = RSpoc; reslevelno < FFMIN(codsty->nreslevels, REpoc); reslevelno++) {

                uint8_t reducedresno = codsty->nreslevels - 1 -reslevelno; //  ==> N_L - r

                Jpeg2000ResLevel *rlevel = comp->reslevel + reslevelno;

                step_x = FFMIN(step_x, rlevel->log2_prec_width  + reducedresno);

                step_y = FFMIN(step_y, rlevel->log2_prec_height + reducedresno);

            }

        }

        step_x = 1<<step_x;

        step_y = 1<<step_y;



        for (y = tile->coord[1][0]; y < tile->coord[1][1]; y = (y/step_y + 1)*step_y) {

            for (x = tile->coord[0][0]; x < tile->coord[0][1]; x = (x/step_x + 1)*step_x) {

                for (compno = CSpoc; compno < CEpoc; compno++) {

                    Jpeg2000Component *comp     = tile->comp + compno;

                    Jpeg2000CodingStyle *codsty = tile->codsty + compno;

                    Jpeg2000QuantStyle *qntsty  = tile->qntsty + compno;

                    int xc = x / s->cdx[compno];

                    int yc = y / s->cdy[compno];



                    for (reslevelno = RSpoc; reslevelno < FFMIN(codsty->nreslevels, REpoc); reslevelno++) {

                        unsigned prcx, prcy;

                        uint8_t reducedresno = codsty->nreslevels - 1 -reslevelno; //  ==> N_L - r

                        Jpeg2000ResLevel *rlevel = comp->reslevel + reslevelno;



                        if (yc % (1 << (rlevel->log2_prec_height + reducedresno)) && y != tile->coord[1][0]) //FIXME this is a subset of the check

                            continue;



                        if (xc % (1 << (rlevel->log2_prec_width + reducedresno)) && x != tile->coord[0][0]) //FIXME this is a subset of the check

                            continue;



                        // check if a precinct exists

                        prcx   = ff_jpeg2000_ceildivpow2(xc, reducedresno) >> rlevel->log2_prec_width;

                        prcy   = ff_jpeg2000_ceildivpow2(yc, reducedresno) >> rlevel->log2_prec_height;

                        prcx  -= ff_jpeg2000_ceildivpow2(comp->coord_o[0][0], reducedresno) >> rlevel->log2_prec_width;

                        prcy  -= ff_jpeg2000_ceildivpow2(comp->coord_o[1][0], reducedresno) >> rlevel->log2_prec_height;



                        precno = prcx + rlevel->num_precincts_x * prcy;



                        if (prcx >= rlevel->num_precincts_x || prcy >= rlevel->num_precincts_y) {

                            av_log(s->avctx, AV_LOG_WARNING, "prc %d %d outside limits %d %d\n",

                                   prcx, prcy, rlevel->num_precincts_x, rlevel->num_precincts_y);

                            continue;

                        }



                        for (layno = 0; layno < LYEpoc; layno++) {

                            if ((ret = jpeg2000_decode_packet(s, tile, &tp_index, codsty, rlevel,

                                                              precno, layno,

                                                              qntsty->expn + (reslevelno ? 3 * (reslevelno - 1) + 1 : 0),

                                                              qntsty->nguardbits)) < 0)

                                return ret;

                        }

                    }

                }

            }

        }

        break;



    default:

        break;

    }



    return ret;

}

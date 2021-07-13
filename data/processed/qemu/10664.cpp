int css_do_tsch(SubchDev *sch, IRB *target_irb)

{

    SCSW *s = &sch->curr_status.scsw;

    PMCW *p = &sch->curr_status.pmcw;

    uint16_t stctl;

    uint16_t fctl;

    uint16_t actl;

    IRB irb;

    int ret;



    if (!(p->flags & (PMCW_FLAGS_MASK_DNV | PMCW_FLAGS_MASK_ENA))) {

        ret = 3;

        goto out;

    }



    stctl = s->ctrl & SCSW_CTRL_MASK_STCTL;

    fctl = s->ctrl & SCSW_CTRL_MASK_FCTL;

    actl = s->ctrl & SCSW_CTRL_MASK_ACTL;



    /* Prepare the irb for the guest. */

    memset(&irb, 0, sizeof(IRB));



    /* Copy scsw from current status. */

    memcpy(&irb.scsw, s, sizeof(SCSW));

    if (stctl & SCSW_STCTL_STATUS_PEND) {

        if (s->cstat & (SCSW_CSTAT_DATA_CHECK |

                        SCSW_CSTAT_CHN_CTRL_CHK |

                        SCSW_CSTAT_INTF_CTRL_CHK)) {

            irb.scsw.flags |= SCSW_FLAGS_MASK_ESWF;

            irb.esw[0] = 0x04804000;

        } else {

            irb.esw[0] = 0x00800000;

        }

        /* If a unit check is pending, copy sense data. */

        if ((s->dstat & SCSW_DSTAT_UNIT_CHECK) &&

            (p->chars & PMCW_CHARS_MASK_CSENSE)) {

            irb.scsw.flags |= SCSW_FLAGS_MASK_ESWF | SCSW_FLAGS_MASK_ECTL;

            memcpy(irb.ecw, sch->sense_data, sizeof(sch->sense_data));

            irb.esw[1] = 0x01000000 | (sizeof(sch->sense_data) << 8);

        }

    }

    /* Store the irb to the guest. */

    copy_irb_to_guest(target_irb, &irb, p);



    /* Clear conditions on subchannel, if applicable. */

    if (stctl & SCSW_STCTL_STATUS_PEND) {

        s->ctrl &= ~SCSW_CTRL_MASK_STCTL;

        if ((stctl != (SCSW_STCTL_INTERMEDIATE | SCSW_STCTL_STATUS_PEND)) ||

            ((fctl & SCSW_FCTL_HALT_FUNC) &&

             (actl & SCSW_ACTL_SUSP))) {

            s->ctrl &= ~SCSW_CTRL_MASK_FCTL;

        }

        if (stctl != (SCSW_STCTL_INTERMEDIATE | SCSW_STCTL_STATUS_PEND)) {

            s->flags &= ~SCSW_FLAGS_MASK_PNO;

            s->ctrl &= ~(SCSW_ACTL_RESUME_PEND |

                         SCSW_ACTL_START_PEND |

                         SCSW_ACTL_HALT_PEND |

                         SCSW_ACTL_CLEAR_PEND |

                         SCSW_ACTL_SUSP);

        } else {

            if ((actl & SCSW_ACTL_SUSP) &&

                (fctl & SCSW_FCTL_START_FUNC)) {

                s->flags &= ~SCSW_FLAGS_MASK_PNO;

                if (fctl & SCSW_FCTL_HALT_FUNC) {

                    s->ctrl &= ~(SCSW_ACTL_RESUME_PEND |

                                 SCSW_ACTL_START_PEND |

                                 SCSW_ACTL_HALT_PEND |

                                 SCSW_ACTL_CLEAR_PEND |

                                 SCSW_ACTL_SUSP);

                } else {

                    s->ctrl &= ~SCSW_ACTL_RESUME_PEND;

                }

            }

        }

        /* Clear pending sense data. */

        if (p->chars & PMCW_CHARS_MASK_CSENSE) {

            memset(sch->sense_data, 0 , sizeof(sch->sense_data));

        }

    }



    ret = ((stctl & SCSW_STCTL_STATUS_PEND) == 0);



out:

    return ret;

}

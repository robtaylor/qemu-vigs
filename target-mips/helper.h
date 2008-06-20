#ifndef DEF_HELPER
#define DEF_HELPER(ret, name, params) ret name params;
#endif

DEF_HELPER(void, do_raise_exception_err, (int excp, int err))
DEF_HELPER(void, do_raise_exception, (int excp))
DEF_HELPER(void, do_interrupt_restart, (void))

DEF_HELPER(void, do_clo, (void))
DEF_HELPER(void, do_clz, (void))
#ifdef TARGET_MIPS64
DEF_HELPER(void, do_dclo, (void))
DEF_HELPER(void, do_dclz, (void))
DEF_HELPER(void, do_dmult, (void))
DEF_HELPER(void, do_dmultu, (void))
#endif

DEF_HELPER(void, do_muls, (void))
DEF_HELPER(void, do_mulsu, (void))
DEF_HELPER(void, do_macc, (void))
DEF_HELPER(void, do_maccu, (void))
DEF_HELPER(void, do_msac, (void))
DEF_HELPER(void, do_msacu, (void))
DEF_HELPER(void, do_mulhi, (void))
DEF_HELPER(void, do_mulhiu, (void))
DEF_HELPER(void, do_mulshi, (void))
DEF_HELPER(void, do_mulshiu, (void))
DEF_HELPER(void, do_macchi, (void))
DEF_HELPER(void, do_macchiu, (void))
DEF_HELPER(void, do_msachi, (void))
DEF_HELPER(void, do_msachiu, (void))

/* CP0 helpers */
#ifndef CONFIG_USER_ONLY
DEF_HELPER(void, do_mfc0_mvpcontrol, (void))
DEF_HELPER(void, do_mfc0_mvpconf0, (void))
DEF_HELPER(void, do_mfc0_mvpconf1, (void))
DEF_HELPER(void, do_mfc0_random, (void))
DEF_HELPER(void, do_mfc0_tcstatus, (void))
DEF_HELPER(void, do_mftc0_tcstatus, (void))
DEF_HELPER(void, do_mfc0_tcbind, (void))
DEF_HELPER(void, do_mftc0_tcbind, (void))
DEF_HELPER(void, do_mfc0_tcrestart, (void))
DEF_HELPER(void, do_mftc0_tcrestart, (void))
DEF_HELPER(void, do_mfc0_tchalt, (void))
DEF_HELPER(void, do_mftc0_tchalt, (void))
DEF_HELPER(void, do_mfc0_tccontext, (void))
DEF_HELPER(void, do_mftc0_tccontext, (void))
DEF_HELPER(void, do_mfc0_tcschedule, (void))
DEF_HELPER(void, do_mftc0_tcschedule, (void))
DEF_HELPER(void, do_mfc0_tcschefback, (void))
DEF_HELPER(void, do_mftc0_tcschefback, (void))
DEF_HELPER(void, do_mfc0_count, (void))
DEF_HELPER(void, do_mftc0_entryhi, (void))
DEF_HELPER(void, do_mftc0_status, (void))
DEF_HELPER(void, do_mfc0_lladdr, (void))
DEF_HELPER(void, do_mfc0_watchlo, (uint32_t sel))
DEF_HELPER(void, do_mfc0_watchhi, (uint32_t sel))
DEF_HELPER(void, do_mfc0_debug, (void))
DEF_HELPER(void, do_mftc0_debug, (void))
#ifdef TARGET_MIPS64
DEF_HELPER(void, do_dmfc0_tcrestart, (void))
DEF_HELPER(void, do_dmfc0_tchalt, (void))
DEF_HELPER(void, do_dmfc0_tccontext, (void))
DEF_HELPER(void, do_dmfc0_tcschedule, (void))
DEF_HELPER(void, do_dmfc0_tcschefback, (void))
DEF_HELPER(void, do_dmfc0_lladdr, (void))
DEF_HELPER(void, do_dmfc0_watchlo, (uint32_t sel))
#endif /* TARGET_MIPS64 */

DEF_HELPER(void, do_mtc0_index, (void))
DEF_HELPER(void, do_mtc0_mvpcontrol, (void))
DEF_HELPER(void, do_mtc0_vpecontrol, (void))
DEF_HELPER(void, do_mtc0_vpeconf0, (void))
DEF_HELPER(void, do_mtc0_vpeconf1, (void))
DEF_HELPER(void, do_mtc0_yqmask, (void))
DEF_HELPER(void, do_mtc0_vpeopt, (void))
DEF_HELPER(void, do_mtc0_entrylo0, (void))
DEF_HELPER(void, do_mtc0_tcstatus, (void))
DEF_HELPER(void, do_mttc0_tcstatus, (void))
DEF_HELPER(void, do_mtc0_tcbind, (void))
DEF_HELPER(void, do_mttc0_tcbind, (void))
DEF_HELPER(void, do_mtc0_tcrestart, (void))
DEF_HELPER(void, do_mttc0_tcrestart, (void))
DEF_HELPER(void, do_mtc0_tchalt, (void))
DEF_HELPER(void, do_mttc0_tchalt, (void))
DEF_HELPER(void, do_mtc0_tccontext, (void))
DEF_HELPER(void, do_mttc0_tccontext, (void))
DEF_HELPER(void, do_mtc0_tcschedule, (void))
DEF_HELPER(void, do_mttc0_tcschedule, (void))
DEF_HELPER(void, do_mtc0_tcschefback, (void))
DEF_HELPER(void, do_mttc0_tcschefback, (void))
DEF_HELPER(void, do_mtc0_entrylo1, (void))
DEF_HELPER(void, do_mtc0_context, (void))
DEF_HELPER(void, do_mtc0_pagemask, (void))
DEF_HELPER(void, do_mtc0_pagegrain, (void))
DEF_HELPER(void, do_mtc0_wired, (void))
DEF_HELPER(void, do_mtc0_srsconf0, (void))
DEF_HELPER(void, do_mtc0_srsconf1, (void))
DEF_HELPER(void, do_mtc0_srsconf2, (void))
DEF_HELPER(void, do_mtc0_srsconf3, (void))
DEF_HELPER(void, do_mtc0_srsconf4, (void))
DEF_HELPER(void, do_mtc0_hwrena, (void))
DEF_HELPER(void, do_mtc0_count, (void))
DEF_HELPER(void, do_mtc0_entryhi, (void))
DEF_HELPER(void, do_mttc0_entryhi, (void))
DEF_HELPER(void, do_mtc0_compare, (void))
DEF_HELPER(void, do_mtc0_status, (void))
DEF_HELPER(void, do_mttc0_status, (void))
DEF_HELPER(void, do_mtc0_intctl, (void))
DEF_HELPER(void, do_mtc0_srsctl, (void))
DEF_HELPER(void, do_mtc0_cause, (void))
DEF_HELPER(void, do_mtc0_ebase, (void))
DEF_HELPER(void, do_mtc0_config0, (void))
DEF_HELPER(void, do_mtc0_config2, (void))
DEF_HELPER(void, do_mtc0_watchlo, (uint32_t sel))
DEF_HELPER(void, do_mtc0_watchhi, (uint32_t sel))
DEF_HELPER(void, do_mtc0_xcontext, (void))
DEF_HELPER(void, do_mtc0_framemask, (void))
DEF_HELPER(void, do_mtc0_debug, (void))
DEF_HELPER(void, do_mttc0_debug, (void))
DEF_HELPER(void, do_mtc0_performance0, (void))
DEF_HELPER(void, do_mtc0_taglo, (void))
DEF_HELPER(void, do_mtc0_datalo, (void))
DEF_HELPER(void, do_mtc0_taghi, (void))
DEF_HELPER(void, do_mtc0_datahi, (void))
#endif /* !CONFIG_USER_ONLY */

/* MIPS MT functions */
DEF_HELPER(void, do_mftgpr, (uint32_t sel))
DEF_HELPER(void, do_mftlo, (uint32_t sel))
DEF_HELPER(void, do_mfthi, (uint32_t sel))
DEF_HELPER(void, do_mftacx, (uint32_t sel))
DEF_HELPER(void, do_mftdsp, (void))
DEF_HELPER(void, do_mttgpr, (uint32_t sel))
DEF_HELPER(void, do_mttlo, (uint32_t sel))
DEF_HELPER(void, do_mtthi, (uint32_t sel))
DEF_HELPER(void, do_mttacx, (uint32_t sel))
DEF_HELPER(void, do_mttdsp, (void))
DEF_HELPER(void, do_dmt, (void))
DEF_HELPER(void, do_emt, (void))
DEF_HELPER(void, do_dvpe, (void))
DEF_HELPER(void, do_evpe, (void))
DEF_HELPER(void, do_fork, (void))
DEF_HELPER(void, do_yield, (void))

/* CP1 functions */
DEF_HELPER(void, do_cfc1, (uint32_t reg))
DEF_HELPER(void, do_ctc1, (uint32_t reg))

DEF_HELPER(void, do_float_cvtd_s, (void))
DEF_HELPER(void, do_float_cvtd_w, (void))
DEF_HELPER(void, do_float_cvtd_l, (void))
DEF_HELPER(void, do_float_cvtl_d, (void))
DEF_HELPER(void, do_float_cvtl_s, (void))
DEF_HELPER(void, do_float_cvtps_pw, (void))
DEF_HELPER(void, do_float_cvtpw_ps, (void))
DEF_HELPER(void, do_float_cvts_d, (void))
DEF_HELPER(void, do_float_cvts_w, (void))
DEF_HELPER(void, do_float_cvts_l, (void))
DEF_HELPER(void, do_float_cvts_pl, (void))
DEF_HELPER(void, do_float_cvts_pu, (void))
DEF_HELPER(void, do_float_cvtw_s, (void))
DEF_HELPER(void, do_float_cvtw_d, (void))

DEF_HELPER(void, do_float_addr_ps, (void))
DEF_HELPER(void, do_float_mulr_ps, (void))

#define FOP_PROTO(op)                            \
DEF_HELPER(void, do_float_ ## op ## _s, (void))  \
DEF_HELPER(void, do_float_ ## op ## _d, (void))
FOP_PROTO(sqrt)
FOP_PROTO(roundl)
FOP_PROTO(roundw)
FOP_PROTO(truncl)
FOP_PROTO(truncw)
FOP_PROTO(ceill)
FOP_PROTO(ceilw)
FOP_PROTO(floorl)
FOP_PROTO(floorw)
FOP_PROTO(rsqrt)
FOP_PROTO(recip)
#undef FOP_PROTO

#define FOP_PROTO(op)                            \
DEF_HELPER(void, do_float_ ## op ## _s, (void))  \
DEF_HELPER(void, do_float_ ## op ## _d, (void))  \
DEF_HELPER(void, do_float_ ## op ## _ps, (void))
FOP_PROTO(add)
FOP_PROTO(sub)
FOP_PROTO(mul)
FOP_PROTO(div)
FOP_PROTO(abs)
FOP_PROTO(chs)
FOP_PROTO(muladd)
FOP_PROTO(mulsub)
FOP_PROTO(nmuladd)
FOP_PROTO(nmulsub)
FOP_PROTO(recip1)
FOP_PROTO(recip2)
FOP_PROTO(rsqrt1)
FOP_PROTO(rsqrt2)
#undef FOP_PROTO

#define FOP_PROTO(op)                            \
DEF_HELPER(void, do_cmp_d_ ## op, (long cc))     \
DEF_HELPER(void, do_cmpabs_d_ ## op, (long cc))  \
DEF_HELPER(void, do_cmp_s_ ## op, (long cc))     \
DEF_HELPER(void, do_cmpabs_s_ ## op, (long cc))  \
DEF_HELPER(void, do_cmp_ps_ ## op, (long cc))    \
DEF_HELPER(void, do_cmpabs_ps_ ## op, (long cc))
FOP_PROTO(f)
FOP_PROTO(un)
FOP_PROTO(eq)
FOP_PROTO(ueq)
FOP_PROTO(olt)
FOP_PROTO(ult)
FOP_PROTO(ole)
FOP_PROTO(ule)
FOP_PROTO(sf)
FOP_PROTO(ngle)
FOP_PROTO(seq)
FOP_PROTO(ngl)
FOP_PROTO(lt)
FOP_PROTO(nge)
FOP_PROTO(le)
FOP_PROTO(ngt)
#undef FOP_PROTO

/* Special functions */
DEF_HELPER(void, do_di, (void))
DEF_HELPER(void, do_ei, (void))
DEF_HELPER(void, do_eret, (void))
DEF_HELPER(void, do_deret, (void))
DEF_HELPER(void, do_rdhwr_cpunum, (void))
DEF_HELPER(void, do_rdhwr_synci_step, (void))
DEF_HELPER(void, do_rdhwr_cc, (void))
DEF_HELPER(void, do_rdhwr_ccres, (void))
DEF_HELPER(void, do_pmon, (int function))
DEF_HELPER(void, do_wait, (void))

/* Bitfield operations. */
DEF_HELPER(void, do_ext, (uint32_t pos, uint32_t size))
DEF_HELPER(void, do_ins, (uint32_t pos, uint32_t size))
DEF_HELPER(void, do_wsbh, (void))
#ifdef TARGET_MIPS64
DEF_HELPER(void, do_dext, (uint32_t pos, uint32_t size))
DEF_HELPER(void, do_dins, (uint32_t pos, uint32_t size))
DEF_HELPER(void, do_dsbh, (void))
DEF_HELPER(void, do_dshd, (void))
#endif

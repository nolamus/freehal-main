/*
 * This file was generated automatically by ExtUtils::ParseXS version 2.18_02 from the
 * contents of JP.xs. Do not edit this file, edit JP.xs instead.
 *
 *	ANY CHANGES MADE HERE WILL BE LOST! 
 *
 */

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#define U8 U8
#include "encode.h"
#include "ji_02_t.h"
#include "ji_03_t.h"
#include "eu_01_t.h"
#include "cp_00_t.h"
#include "ji_04_t.h"
#include "sh_06_t.h"
#include "ma_05_t.h"

static void
Encode_XSEncoding(pTHX_ encode_t *enc)
{
 dSP;
 HV *stash = gv_stashpv("Encode::XS", TRUE);
 SV *sv    = sv_bless(newRV_noinc(newSViv(PTR2IV(enc))),stash);
 int i = 0;
 PUSHMARK(sp);
 XPUSHs(sv);
 while (enc->name[i])
  {
   const char *name = enc->name[i++];
   XPUSHs(sv_2mortal(newSVpvn(name,strlen(name))));
  }
 PUTBACK;
 call_pv("Encode::define_encoding",G_DISCARD);
 SvREFCNT_dec(sv);
}

#ifndef PERL_UNUSED_VAR
#  define PERL_UNUSED_VAR(var) if (0) var = var
#endif

#ifdef __cplusplus
extern "C"
#endif
XS(boot_Encode__JP); /* prototype to pass -Wmissing-prototypes */
XS(boot_Encode__JP)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif

    PERL_UNUSED_VAR(cv); /* -W */
    PERL_UNUSED_VAR(items); /* -W */
    XS_VERSION_BOOTCHECK ;


    /* Initialisation Section */

{
#include "ji_02_t.exh"
#include "ji_03_t.exh"
#include "eu_01_t.exh"
#include "cp_00_t.exh"
#include "ji_04_t.exh"
#include "sh_06_t.exh"
#include "ma_05_t.exh"
}


    /* End of Initialisation Section */

    if (PL_unitcheckav)
         call_list(PL_scopestack_ix, PL_unitcheckav);
    XSRETURN_YES;
}


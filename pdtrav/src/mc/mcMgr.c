/**CFile***********************************************************************

  FileName    [mcMgr.c]

  PackageName [mc]

  Synopsis    [Functions to handle an MC Manager]

  Description []

  SeeAlso     []

  Author      [Gianpiero Cabodi]

  Copyright [  Copyright (c) 2001 by Politecnico di Torino.
    All Rights Reserved. This software is for educational purposes only.
    Permission is given to academic institutions to use, copy, and modify
    this software and its documentation provided that this introductory
    message is not removed, that this software and its documentation is
    used for the institutions' internal research and educational purposes,
    and that no monies are exchanged. No guarantee is expressed or implied
    by the distribution of this code.
    Send bug-reports and/or questions to: {cabodi,quer}@polito.it. ]

  Revision    []

******************************************************************************/

#include "mcInt.h"

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Stucture declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of internal functions                                          */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [Creates a MC Manager.]

  Description []

  SideEffects [none]

  SeeAlso     [Ddi_MgrInit, Fsm_MgrInit, Trav_MgrInit]

******************************************************************************/

Mc_Mgr_t *
Mc_MgrInit(
  char *mcName /* Name of the MC structure */ ,
  Fsm_Mgr_t * fsmMgr            /* FSM manager structure */
)
{
  Mc_Mgr_t *mcMgr;

  mcMgr = Pdtutil_Alloc(Mc_Mgr_t, 1);
  Pdtutil_Assert(mcMgr != NULL, "Out of memory.");

  Mc_MgrSetMcName(mcMgr, (mcName != NULL) ? mcName : (char *)"mc");
  mcMgr->fsmMgr = fsmMgr;
  if (fsmMgr != NULL) {
    mcMgr->ddiMgr = Fsm_MgrReadDdManager(fsmMgr);
  } else {
    mcMgr->ddiMgr = Ddi_MgrInit("ddiMgr", NULL, 0, DDI_UNIQUE_SLOTS,
      DDI_CACHE_SLOTS * 10, 0, -1, -1);
    mcMgr->fsmMgr = Fsm_MgrInit("fsmMgr", NULL);
    Fsm_MgrSetDdManager(mcMgr->fsmMgr, mcMgr->ddiMgr);
  }

  mcMgr->trMgr = NULL;
  mcMgr->travMgr = NULL;
  mcMgr->optList = NULL;

  /* mc specific settings */
  mcMgr->settings.verbosity = Pdtutil_VerbLevelUsrMax_c;

  mcMgr->settings.task = Mc_TaskNone_c;
  mcMgr->settings.method = Mc_VerifApproxFwdExactBck_c;
  mcMgr->settings.pdr = 0;
  mcMgr->settings.aig = 0;
  mcMgr->settings.inductive = -1;
  mcMgr->settings.interpolant = -1;
  mcMgr->settings.custom = 0;
  mcMgr->settings.lemmas = -1;
  mcMgr->settings.lazy = -1;
  mcMgr->settings.lazyRate = 1.0;
  mcMgr->settings.qbf = 0;
  mcMgr->settings.diameter = -1;
  mcMgr->settings.checkMult = 0;
  mcMgr->settings.exactTrav = 0;

  mcMgr->settings.bmc = -1;
  mcMgr->settings.bmcStrategy = 1;
  mcMgr->settings.interpolantBmcSteps = 0;
  mcMgr->settings.bmcTr = -1;
  mcMgr->settings.bmcStep = 1;
  mcMgr->settings.bmcTe = 0;
  mcMgr->settings.bmcFirst = 0;

  mcMgr->settings.itpSeq = 0;
  mcMgr->settings.itpSeqGroup = 0;
  mcMgr->settings.itpSeqSerial = 1.0;
  mcMgr->settings.initBound = -1;
  mcMgr->settings.enFastRun = -1;

  mcMgr->settings.bddNodeLimit = -1;
  mcMgr->settings.bddMemoryLimit = -1;
  mcMgr->settings.bddTimeLimit = -1;
  mcMgr->settings.itpBoundLimit = -1;
  mcMgr->settings.bddPeakNodes = -1;
  mcMgr->settings.bddTimeLimit = -1;
  mcMgr->settings.pdrTimeLimit = -1;
  mcMgr->settings.itpTimeLimit = -1;
  mcMgr->settings.itpMemoryLimit = -1;
  mcMgr->settings.bmcTimeLimit = -1;
  mcMgr->settings.bmcMemoryLimit = -1;
  mcMgr->settings.indTimeLimit = -1;

  mcMgr->auxPtr = NULL;

  Pdtutil_Verbosity(printf("-- MC Manager Init.\n");
    )
    return mcMgr;
}

/**Function********************************************************************

  Synopsis     [Closes a Model Checking Manager.]

  Description  [Closes a Model Checking Manager freeing all the
    correlated fields.]

  SideEffects  [none]

  SeeAlso      [Ddi_BddiMgrInit]

******************************************************************************/

void
Mc_MgrQuit(
  Mc_Mgr_t * mcMgr              /* Mc manager */
)
{
  if (mcMgr == NULL) {
    return;
  }

  Pdtutil_VerbosityMgr(mcMgr,
    Pdtutil_VerbLevelUsrMax_c, fprintf(stdout, "-- Mc Manager Quit.\n")
    );

  Pdtutil_Free(mcMgr->mcName);

  Pdtutil_Free(mcMgr);
}

/**Function********************************************************************

  Synopsis    [Prints Statistics on a Transition Relation Manager.]

  Description [Prints Statistics on a Transition Relation Manager on
    standard output.]

  SideEffects []

  SeeAlso     []

******************************************************************************/

int
Mc_MgrPrintStats(
  Mc_Mgr_t * mcMgr              /* Mc manager */
)
{
  Pdtutil_VerbosityMgr(mcMgr, Pdtutil_VerbLevelUsrMin_c,
    fprintf(stdout, "Model Checking Manager Name %s.\n",
      mcMgr->mcName);
    fprintf(stdout, "Total CPU Time: %s.\n",
      util_print_time(mcMgr->stats.mcTime))
    );

  return (1);
}

/**Function********************************************************************

  Synopsis    [Read verbosity]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

Pdtutil_VerbLevel_e
Mc_MgrReadVerbosity(
  Mc_Mgr_t * mcMgr              /* MC Manager */
)
{
  return (mcMgr->settings.verbosity);
}

/**Function********************************************************************

  Synopsis    [Set verbosity.]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Mc_MgrSetVerbosity(
  Mc_Mgr_t * mcMgr /* MC Manager */ ,
  Pdtutil_VerbLevel_e verbosity /* Verbosity */
)
{
  mcMgr->settings.verbosity = verbosity;
}


/**Function********************************************************************

  Synopsis    []

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/

void
Mc_MgrSetMcName(
  Mc_Mgr_t * mcMgr /* MC Manager */ ,
  char *mcName                  /* MC Manager Name */
)
{
  mcMgr->mcName = Pdtutil_StrDup(mcName);
}


/**Function********************************************************************

  Synopsis    [Read name]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
char *
Mc_MgrReadMcName(
  Mc_Mgr_t * mcMgr              /* MC Manager */
)
{
  return (mcMgr->mcName);
}


/**Function********************************************************************

  Synopsis    [Transfer the full option list to the specific managers]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
void
Mc_MgrSetMgrOptList(
  Mc_Mgr_t * mcMgr /* MC Manager */ ,
  Pdtutil_OptList_t * optList
)
{
  Pdtutil_OptList_t *pkgOpt;

  /* process list and transfer settings to managers */

  Pdtutil_Assert(Pdtutil_OptListReadModule(optList) == Pdt_OptPdt_c,
    "wrong module for opt list");

  while (!Pdtutil_OptListEmpty(optList)) {
    Pdtutil_OptItem_t item = Pdtutil_OptListExtractHead(optList);

    Pdtutil_Assert(item.optTag.eListOpt == Pdt_ListNone_c,
      "wrong tag for opt list item");
    pkgOpt = (Pdtutil_OptList_t *) (item.optData.pvoid);
    switch (Pdtutil_OptListReadModule(pkgOpt)) {
      case Pdt_OptDdi_c:
        Ddi_MgrSetOptionList(mcMgr->ddiMgr, pkgOpt);
        break;
      case Pdt_OptFsm_c:
        Fsm_MgrSetOptionList(mcMgr->fsmMgr, pkgOpt);
        break;
      case Pdt_OptTr_c:
        Tr_MgrSetOptionList(mcMgr->trMgr, pkgOpt);
        break;
      case Pdt_OptTrav_c:
        Trav_MgrSetOptionList(mcMgr->travMgr, pkgOpt);
        break;
      case Pdt_OptMc_c:
        Mc_MgrSetOptionList(mcMgr, pkgOpt);
        break;
      default:
        Pdtutil_Assert(0, "wrong opt list item tag");
    }
  }
}

/**Function********************************************************************

  Synopsis    [Transfer options from an option list to a Mc Manager]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
void
Mc_MgrSetOptionList(
  Mc_Mgr_t * mcMgr /* MC Manager */ ,
  Pdtutil_OptList_t * pkgOpt    /* list of options */
)
{
  Pdtutil_OptItem_t optItem;

  while (!Pdtutil_OptListEmpty(pkgOpt)) {
    optItem = Pdtutil_OptListExtractHead(pkgOpt);
    Mc_MgrSetOptionItem(mcMgr, optItem);
  }
}

/**Function********************************************************************

  Synopsis    [Transfer options from an option list to a Mc Manager]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
void
Mc_MgrSetOptionItem(
  Mc_Mgr_t * mcMgr /* MC Manager */ ,
  Pdtutil_OptItem_t optItem     /* option to set */
)
{
  switch (optItem.optTag.eMcOpt) {
    case Pdt_McVerbosity_c:
      mcMgr->settings.verbosity = Pdtutil_VerbLevel_e(optItem.optData.inum);
      break;
    case Pdt_McMethod_c:
      mcMgr->settings.method = (Mc_VerifMethod_e) (optItem.optData.inum);
      break;
    case Pdt_McPdr_c:
      mcMgr->settings.pdr = optItem.optData.inum;
      break;
    case Pdt_McAig_c:
      mcMgr->settings.aig = optItem.optData.inum;
      break;
    case Pdt_McInductive_c:
      mcMgr->settings.inductive = optItem.optData.inum;
      break;
    case Pdt_McInterpolant_c:
      mcMgr->settings.interpolant = optItem.optData.inum;
      break;
    case Pdt_McCustom_c:
      mcMgr->settings.custom = optItem.optData.inum;
      break;
    case Pdt_McLemmas_c:
      mcMgr->settings.lemmas = optItem.optData.inum;
      break;
    case Pdt_McLazy_c:
      mcMgr->settings.lazy = optItem.optData.inum;
      break;
    case Pdt_McLazyRate_c:
      mcMgr->settings.lazyRate = optItem.optData.fnum;
      break;
    case Pdt_McQbf_c:
      mcMgr->settings.qbf = optItem.optData.inum;
      break;
    case Pdt_McDiameter_c:
      mcMgr->settings.diameter = optItem.optData.inum;
      break;
    case Pdt_McCheckMult_c:
      mcMgr->settings.checkMult = optItem.optData.inum;
      break;
    case Pdt_McExactTrav_c:
      mcMgr->settings.exactTrav = optItem.optData.inum;
      break;
    case Pdt_McBmc_c:
      mcMgr->settings.bmc = optItem.optData.inum;
      break;
    case Pdt_McBmcStrategy_c:
      mcMgr->settings.bmcStrategy = optItem.optData.inum;
      break;
    case Pdt_McInterpolantBmcSteps_c:
      mcMgr->settings.interpolantBmcSteps = optItem.optData.inum;
      break;
    case Pdt_McBmcTr_c:
      mcMgr->settings.bmcTr = optItem.optData.inum;
      break;
    case Pdt_McBmcStep_c:
      mcMgr->settings.bmcStep = optItem.optData.inum;
      break;
    case Pdt_McBmcTe_c:
      mcMgr->settings.bmcTe = optItem.optData.inum;
      break;
    case Pdt_McBmcLearnStep_c:
      mcMgr->settings.bmcLearnStep = optItem.optData.inum;
      break;
    case Pdt_McBmcFirst_c:
      mcMgr->settings.bmcFirst = optItem.optData.inum;
      break;
    case Pdt_McItpSeq_c:
      mcMgr->settings.itpSeq = optItem.optData.inum;
      break;
    case Pdt_McItpSeqGroup_c:
      mcMgr->settings.itpSeqGroup = optItem.optData.inum;
      break;
    case Pdt_McItpSeqSerial_c:
      mcMgr->settings.itpSeqSerial = optItem.optData.fnum;
      break;
    case Pdt_McInitBound_c:
      mcMgr->settings.initBound = optItem.optData.inum;
      break;
    case Pdt_McEnFastRun_c:
      mcMgr->settings.enFastRun = optItem.optData.inum;
      break;
    case Pdt_McFwdBwdFP_c:
      mcMgr->settings.fwdBwdFP = optItem.optData.inum;
      break;
    default:
      Pdtutil_Assert(0, "wrong mc opt list item tag");
  }
}

/**Function********************************************************************

  Synopsis    [Return an option value from a Mc Manager ]

  Description []

  SideEffects [none]

  SeeAlso     []

******************************************************************************/
void
Mc_MgrReadOption(
  Mc_Mgr_t * mcMgr /* MC Manager */ ,
  Pdt_OptTagMc_e mcOpt /* option code */ ,
  void *optRet                  /* returned option value */
)
{
  switch (mcOpt) {
    case Pdt_McVerbosity_c:
      *(int *)optRet = (int)mcMgr->settings.verbosity;
      break;
    case Pdt_McMethod_c:
      *(int *)optRet = (int)mcMgr->settings.method;
      break;
    case Pdt_McPdr_c:
      *(int *)optRet = mcMgr->settings.pdr;
      break;
    case Pdt_McAig_c:
      *(int *)optRet = mcMgr->settings.aig;
      break;
    case Pdt_McInductive_c:
      *(int *)optRet = mcMgr->settings.inductive;
      break;
    case Pdt_McInterpolant_c:
      *(int *)optRet = mcMgr->settings.interpolant;
      break;
    case Pdt_McCustom_c:
      *(int *)optRet = mcMgr->settings.custom;
      break;
    case Pdt_McLemmas_c:
      *(int *)optRet = mcMgr->settings.lemmas;
      break;
    case Pdt_McLazy_c:
      *(int *)optRet = mcMgr->settings.lazy;
      break;
    case Pdt_McLazyRate_c:
      *(float *)optRet = mcMgr->settings.lazyRate;
      break;
    case Pdt_McQbf_c:
      *(int *)optRet = mcMgr->settings.qbf;
      break;
    case Pdt_McDiameter_c:
      *(int *)optRet = mcMgr->settings.diameter;
      break;
    case Pdt_McCheckMult_c:
      *(int *)optRet = mcMgr->settings.checkMult;
      break;
    case Pdt_McExactTrav_c:
      *(int *)optRet = mcMgr->settings.exactTrav;
      break;
    case Pdt_McBmc_c:
      *(int *)optRet = mcMgr->settings.bmc;
      break;
    case Pdt_McBmcStrategy_c:
      *(int *)optRet = mcMgr->settings.bmcStrategy;
      break;
    case Pdt_McInterpolantBmcSteps_c:
      *(int *)optRet = mcMgr->settings.interpolantBmcSteps;
      break;
    case Pdt_McBmcTr_c:
      *(int *)optRet = mcMgr->settings.bmcTr;
      break;
    case Pdt_McBmcStep_c:
      *(int *)optRet = mcMgr->settings.bmcStep;
      break;
    case Pdt_McBmcTe_c:
      *(int *)optRet = mcMgr->settings.bmcTe;
      break;
    case Pdt_McBmcLearnStep_c:
      *(int *)optRet = mcMgr->settings.bmcLearnStep;
      break;
    case Pdt_McBmcFirst_c:
      *(int *)optRet = mcMgr->settings.bmcFirst;
      break;
    case Pdt_McItpSeq_c:
      *(int *)optRet = mcMgr->settings.itpSeq;
      break;
    case Pdt_McItpSeqGroup_c:
      *(int *)optRet = mcMgr->settings.itpSeqGroup;
      break;
    case Pdt_McItpSeqSerial_c:
      *(float *)optRet = mcMgr->settings.itpSeqSerial;
      break;
    case Pdt_McInitBound_c:
      *(int *)optRet = mcMgr->settings.initBound;
      break;
    case Pdt_McEnFastRun_c:
      *(int *)optRet = mcMgr->settings.enFastRun;
      break;
    case Pdt_McFwdBwdFP_c:
      *(int *)optRet = mcMgr->settings.fwdBwdFP;
      break;

    default:
      Pdtutil_Assert(0, "wrong mc opt item tag");
  }
}

#if 0
Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiVerbosity_c, inum, opt->verbosity);
Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiSatSolver_c, pchar, opt->satSolver);
Pdtutil_OptListIns(pkgOpt, eDdiOpt, Pdt_DdiItpAbc_cs, inum, opt->itpAbc);


Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigAbcOpt_c, inum, aol);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigAbcOpt_c, inum, opt->aigAbcOpt);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigCnfLevel_c, inum, opt->aigCnfLev);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiDynAbstrStrategy_c, inum,
  opt->dynAbstrStrategy);


Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpAbc_c, inum, opt->itpAbc);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCore_c, inum, opt->itpCore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCore_c, inum, opt->itpCore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCore_c, inum, opt->itpCore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCore_c, inum, opt->itpCore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCore_c, inum, opt->itpCore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCore_c, inum, opt->itpCore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCore_c, inum, opt->itpCore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpCore_c, inum, opt->itpCore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMem_c, inum, opt->itpMem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMem_c, inum, opt->itpMem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMem_c, inum, opt->itpMem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMem_c, inum, opt->itpMem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMem_c, inum, opt->itpMem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMem_c, inum, opt->itpMem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMem_c, inum, opt->itpMem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpMem_c, inum, opt->itpMem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpOpt_c, inum, opt->itpOpt);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpOpt_c, inum, opt->itpOpt);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpOpt_c, inum, opt->itpOpt);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpOpt_c, inum, opt->itpOpt);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpOpt_c, inum, opt->itpOpt);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpOpt_c, inum, opt->itpOpt);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpOpt_c, inum, opt->itpOpt);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpOpt_c, inum, opt->itpOpt);
Ddi_MgrSetOption(ddiMgr, Ddi_MgrSetAigItpPartTh, inum, opt->itpPartTh);
Ddi_MgrSetOption(ddiMgr, Ddi_MgrSetAigItpPartTh, inum, opt->itpPartTh);
Ddi_MgrSetOption(ddiMgr, Ddi_MgrSetAigItpPartTh, inum, opt->itpPartTh);
Ddi_MgrSetOption(ddiMgr, Ddi_MgrSetAigItpPartTh, inum, opt->itpPartTh);
Ddi_MgrSetOption(ddiMgr, Ddi_MgrSetAigItpPartTh, inum, opt->itpPartTh);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStore_c, inum, opt->itpStore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStore_c, inum, opt->itpStore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStore_c, inum, opt->itpStore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStore_c, inum, opt->itpStore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStore_c, inum, opt->itpStore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStore_c, inum, opt->itpStore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStore_c, inum, opt->itpStore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiItpStore_c, inum, opt->itpStore);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiLazyRate_c, fnum, opt->lazyRate);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiLazyRate_c, fnum, opt->lazyRate);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiLazyRate_c, fnum, opt->lazyRate);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiLazyRate_c, fnum, opt->lazyRate);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRem_c, inum, opt->aigRedRem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRem_c, inum, opt->aigRedRem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRem_c, inum, opt->aigRedRem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRem_c, inum, opt->aigRedRem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRem_c, inum, opt->aigRedRem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRem_c, inum, opt->aigRedRem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRem_c, inum, opt->aigRedRem);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRemPeriod_c, inum, opt->aigRedRemPeriod);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRemPeriod_c, inum, opt->aigRedRemPeriod);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRemPeriod_c, inum, opt->aigRedRemPeriod);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRemPeriod_c, inum, opt->aigRedRemPeriod);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRemPeriod_c, inum, opt->aigRedRemPeriod);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRemPeriod_c, inum, opt->aigRedRemPeriod);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiAigRedRemPeriod_c, inum, opt->aigRedRemPeriod);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatIncremental_c, inum, opt->satIncremental);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatIncremental_c, inum, opt->satIncremental);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatIncremental_c, inum, opt->satIncremental);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatIncremental_c, inum, opt->satIncremental);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatTimeLimit_c, inum, opt->satTimeLimit);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatTimeLimit_c, inum, opt->satTimeLimit);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatTimeLimit_c, inum, opt->satTimeLimit);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatTimeout_c, inum, opt->satTimeout);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatTimeout_c, inum, opt->satTimeout);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatTimeout_c, inum, opt->satTimeout);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatTimeout_c, inum, opt->satTimeout);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatVarActivity_c, inum, opt->satVarActivity);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatVarActivity_c, inum, opt->satVarActivity);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatVarActivity_c, inum, opt->satVarActivity);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatVarActivity_c, inum, opt->satVarActivity);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiTernaryAbstrStrategy_c, inum,
  opt->ternaryAbstrStrategy);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiTernaryAbstrStrategy_c, inum,
  opt->ternaryAbstrStrategy);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiTernaryAbstrStrategy_c, inum,
  opt->ternaryAbstrStrategy);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiTernaryAbstrStrategy_c, inum,
  opt->ternaryAbstrStrategy);
Ddi_MgrSetExistDisjPartTh(ddiMgr, opt->imgDpartTh);
Ddi_MgrSetExistDisjPartTh(ddiMgr, opt->imgDpartTh * 10 /**5*/ );
Ddi_MgrSetOption(ddiMgr, Pdt_DdiExistOptThresh_c, inum, opt->exist_opt_thresh);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiExistOptThresh_c, inum, opt->exist_opt_thresh);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiExistOptThresh_c, inum, opt->exist_opt_thresh);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiExistOptThresh_c, inum, opt->exist_opt_thresh);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiExistOptLevel_c, inum, 2);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiExistOptLevel_c, inum, opt->exist_opt_level);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiExistOptLevel_c, inum, opt->exist_opt_level);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiExistOptLevel_c, inum, opt->exist_opt_level);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiExistOptLevel_c, inum, opt->exist_opt_level);
Ddi_MgrSetMemoryLimit(ddiMgr, -1);
Ddi_MgrSetMemoryLimit(ddiMgr, bddMemoryLimit);
Ddi_MgrSetMemoryLimit(ddiMgr, bddMemoryLimit);
Ddi_MgrSetMemoryLimit(ddiMgr, bddMemoryLimit);
Ddi_MgrSetMemoryLimit(ddiMgr, opt->expt.bddMemoryLimit);
Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->metaMaxSmooth);
Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->metaMaxSmooth);
Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->metaMaxSmooth);
Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->metaMaxSmooth);
Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->metaMaxSmooth);
Ddi_MgrSetMetaMaxSmooth(ddiMgr, opt->metaMaxSmooth);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatSolver_c, pchar, opt->satSolver);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatSolver_c, pchar, opt->satSolver);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatSolver_c, pchar, opt->satSolver);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiSatSolver_c, pchar, opt->satSolver);
Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->siftMaxCalls);
Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->siftMaxCalls);
Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->siftMaxCalls);
Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->siftMaxCalls);
Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->siftMaxCalls);
Ddi_MgrSetSiftMaxCalls(ddiMgr, opt->siftMaxCalls);
Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->siftMaxTh);
Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->siftMaxTh);
Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->siftMaxTh);
Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->siftMaxTh);
Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->siftMaxTh);
Ddi_MgrSetSiftMaxThresh(ddiMgr, opt->siftMaxTh);
Ddi_MgrSetSiftThresh(ddiMgr, 1000000);
Ddi_MgrSetSiftThresh(ddiMgr2, Ddi_MgrReadLiveNodes(ddiMgr));
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetSiftThresh(ddiMgr, opt->siftTh);
Ddi_MgrSetTimeInitial(ddiMgr);
Ddi_MgrSetTimeInitial(ddiMgr);
Ddi_MgrSetTimeInitial(ddiMgr);
Ddi_MgrSetTimeInitial(ddiMgr);
Ddi_MgrSetTimeLimit(ddiMgr, -1);
Ddi_MgrSetTimeLimit(ddiMgr, bddTimeLimit);
Ddi_MgrSetTimeLimit(ddiMgr, bddTimeLimit);
Ddi_MgrSetTimeLimit(ddiMgr, opt->expt.bddTimeLimit);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum,
  (Pdtutil_VerbLevel_e) ((int)verbosityDd - 1));
Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum,
  Pdtutil_VerbLevel_e(opt->verbosity));
Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum,
  Pdtutil_VerbLevel_e(opt->verbosity));
Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum,
  Pdtutil_VerbLevel_e(opt->verbosity));
Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum,
  Pdtutil_VerbLevel_e(opt->verbosity));
Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum,
  Pdtutil_VerbLevel_e(opt->verbosity));
Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum, verbosityDd);
Ddi_MgrSetOption(ddiMgr, Pdt_DdiVerbosity_c, inum, verbosityDd);

#endif

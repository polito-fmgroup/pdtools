/**CFile***********************************************************************

  FileName    [mcMc.c]

  PackageName [tr]

  Synopsis    [Functions to handle a Model Checking run]

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
#include "fbv.h"
#include "pdtutil.h"

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

/**Function*******************************************************************
  Synopsis    [Run a Model Checking task.]
  Description []
  SideEffects []
  SeeAlso     []
******************************************************************************/
int
Mc_McRun(
  Mc_Mgr_t * mcMgr,
  Trav_Xchg_t ** xchgP
)
{
  Ddi_Mgr_t *ddiMgr = Fsm_MgrReadDdManager(mcMgr->fsmMgr);
  Ddi_Bdd_t *init, *invarspec, *invar, *care;
  int verificationOK, verificationResult = -1;
  int nl, ni, ng, logEnable = 0;
  char *taskName;


  invarspec = Fsm_MgrReadInvarspecBDD(mcMgr->fsmMgr);
  init = Fsm_MgrReadInitBDD(mcMgr->fsmMgr);
  invar = Fsm_MgrReadConstraintBDD(mcMgr->fsmMgr);
  care = Fsm_MgrReadCareBDD(mcMgr->fsmMgr);

  nl = Ddi_VararrayNum(Fsm_MgrReadVarPS(mcMgr->fsmMgr));
  ni = Ddi_VararrayNum(Fsm_MgrReadVarI(mcMgr->fsmMgr));
  ng = Ddi_BddarraySize(Fsm_MgrReadDeltaBDD(mcMgr->fsmMgr));

  mcMgr->travMgr = Trav_MgrInit(NULL, ddiMgr);
  mcMgr->trMgr = Tr_MgrInit("trMgr", ddiMgr);
  Mc_MgrSetMgrOptList(mcMgr, mcMgr->optList);

  /* create new option list for variations */
  mcMgr->optList = Pdtutil_OptListCreate(Pdt_OptPdt_c);
  // lemmasSteps

  if (xchgP != NULL) {
    // start xchg data
    *xchgP = Trav_MgrXchg(mcMgr->travMgr);
    logEnable = 1;
  }
#if 0
  if (worker->io.enOutputPipe) {
    int flags;

    pipe(worker->io.outputPipe);
    flags = fcntl(worker->io.outputPipe[0], F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(worker->io.outputPipe[0], F_SETFL, flags);
    worker->io.fpi = fdopen(worker->io.outputPipe[0], "r");
    worker->io.fpo = fdopen(worker->io.outputPipe[1], "w");
    Trav_MgrSetStdout(worker->travMgr, worker->io.fpo);
    Ddi_MgrSetStdout(worker->ddiMgr, worker->io.fpo);
  }
#endif

  CATCH {

    switch (mcMgr->settings.task) {

      case Mc_TaskBmc_c:
      case Mc_TaskBmc2_c:
      case Mc_TaskBmcb_c:

	{
	  int bmcStep = 1, bmcFirst = 0, bmcLearnStep = 4, bmcTe = 0;
          taskName = "BMC";
	  bmcTe = mcMgr->settings.bmcTe;
	  if (mcMgr->settings.task==Mc_TaskBmc2_c) {
	    taskName = "BMC2";
	    bmcFirst = mcMgr->settings.bmcFirst;
	    bmcStep = mcMgr->settings.bmcStep;
	    bmcLearnStep = mcMgr->settings.bmcLearnStep;
	    Ddi_MgrSetAigSatTimeLimit(mcMgr->ddiMgr, 500.0);
	  }
	  else if (mcMgr->settings.task==Mc_TaskBmcb_c) {
	    taskName = "BMCB";
	    mcMgr->settings.bmcStrategy = 4;
	  }
	  if (mcMgr->settings.bmcMemoryLimit>0) {
	    Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravBmcMemLimit_c, 
			      inum,mcMgr->settings.bmcMemoryLimit);
	  }
          if (mcMgr->settings.bmcStrategy >= 1) {
            Trav_TravSatBmcIncrVerif(mcMgr->travMgr, mcMgr->fsmMgr,
              init, invarspec, invar, NULL,
              20000 /*mcMgr->opt->expt.bmcSteps */ ,
	      bmcStep /*bmcStep */ , 
	      bmcLearnStep /*mcMgr->opt->trav.bmcLearnStep */ ,
              bmcFirst, 1 /*doCex */ , 0, 0 /*interpolantBmcSteps */ ,
	      mcMgr->settings.bmcStrategy, bmcTe,
              -1 /*mcMgr->opt->expt.bmcTimeLimit */ );
          } else {
            Trav_TravSatBmcVerif(mcMgr->travMgr, mcMgr->fsmMgr,
              init, invarspec, invar, 10000 /*mcMgr->opt->expt.bmcSteps */ ,
               bmcStep /*bmcStep */ , 0, 0 /*apprAig */ , bmcFirst,
	      0 /*interpolantBmcSteps */ , 
              -1 /*mcMgr->opt->expt.bmcTimeLimit */ );
          }
        }
        verificationResult = Trav_MgrReadAssertFlag(mcMgr->travMgr);

        break;

      case Mc_TaskLms_c:

        taskName = "LMS";

        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravLemmasTimeLimit_c, fnum,
          600);

        Trav_TravSatItpVerif(mcMgr->travMgr, mcMgr->fsmMgr,
	  init, invarspec, NULL, invar, NULL, 12,
          -1, mcMgr->settings.lazyRate, -1, -1 /*itpTimeLimit */ );

        verificationResult = Trav_MgrReadAssertFlag(mcMgr->travMgr);

        break;

      case Mc_TaskPdrs_c:
      case Mc_TaskPdr2_c:
      case Mc_TaskPdr_c:

        taskName = "PDR";

        if (logEnable) {
          Trav_LogInit(mcMgr->travMgr, 4, "CPUTIME", "RESTART", "TOTCONFL",
            "TOTCL");
        }

        if (mcMgr->settings.task == Mc_TaskPdr2_c) {
          taskName = "PDR2";
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravPdrUnfold_c, inum, 0);
        } else if (mcMgr->settings.task == Mc_TaskPdrs_c) {
          taskName = "PDRS";
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravPdrUnfold_c, inum, 0);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravPdrGenEffort_c, inum, 1);
        }
        Trav_TravPdrVerif(mcMgr->travMgr, mcMgr->fsmMgr, init,
          invarspec, invar, NULL, -1, -1);

        verificationResult = Trav_MgrReadAssertFlag(mcMgr->travMgr);

        break;

      case Mc_TaskBdd_c:

        taskName = "BDD";
        verificationOK = Fbv_RunBddMc(mcMgr);

        if (verificationOK >= 0) {
          /* property proved or failed */
          verificationResult = !verificationOK;
        }

        break;

      case Mc_TaskBdd2_c:

        taskName = "BDD2";
        mcMgr->settings.method = Mc_VerifExactBck_c;
        verificationOK = Fbv_RunBddMc(mcMgr);

        if (verificationOK >= 0) {
          /* property proved or failed */
          verificationResult = !verificationOK;
        }

        break;

      case Mc_TaskItpl_c:
      case Mc_TaskItp_c:
      case Mc_TaskItp2_c:
        taskName = "ITP";
        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpTimeLimit_c, inum,
          ((Fbv_Globals_t *) (mcMgr->auxPtr))->expt.itpTimeLimit);
        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpPeakAig_c, inum,
          ((Fbv_Globals_t *) (mcMgr->auxPtr))->expt.itpPeakAig);
	Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravDynAbstrInitIter_c, inum, 2);
        if (mcMgr->settings.task == Mc_TaskItpl_c) {
          //mcMgr->settings.lemmas = mcMgr->opt->expt.lemmasSteps;
          taskName = "ITPL";
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravDynAbstr_c, inum, 0);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpAppr_c, inum, 1);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravImplAbstr_c, inum, 0);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpInductiveTo_c, inum, 1);
        } else if (mcMgr->settings.task == Mc_TaskItp2_c) {
          taskName = "ITP2";
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpAppr_c, inum, 0);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum, 0);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpInductiveTo_c, inum, 2);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravImplAbstr_c, inum, 2);
          Ddi_MgrSetOption(mcMgr->ddiMgr, Pdt_DdiItpAigCore_c, inum, 50000);
        } else if (1) {
          // Trav_MgrSetOption(mcMgr->travMgr,Pdt_TravDynAbstr_c,inum,2);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpAppr_c, inum, 0);
          if (nl < 2000) {
            // Trav_MgrSetOption(mcMgr->travMgr,Pdt_TravImplAbstr_c,inum,0);
            if (1 || nl > 1000) {
              Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpInductiveTo_c, inum, 2);
            }
            else {
              Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravDynAbstrInitIter_c, inum, 1);
            }
          }
          if (nl > 200) {
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum, 2);
            if (nl < 80) {
              Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpInductiveTo_c, inum,
                2);
            }
          }
        }
        mcMgr->settings.bmcTimeLimit = -1 /*60 */ ;
        mcMgr->settings.lazy = 1;
        mcMgr->settings.lazyRate = 1.0;

        // Ddi_MgrSetOption(mcMgr->ddiMgr, Pdt_DdiItpDrup_c, inum, 1);
        // Ddi_MgrSetOption(mcMgr->ddiMgr, Pdt_DdiItpReverse_c, inum, 1);
        Trav_TravSatItpVerif(mcMgr->travMgr, mcMgr->fsmMgr,
	  init, invarspec, NULL, invar, NULL /*care */ , mcMgr->settings.lemmas,
          -1 /*itpBoundLimit */ ,
          mcMgr->settings.lazyRate, -1, -1 /*itpTimeLimit */ );

        Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
          fprintf(stdout, "THRD: %s - peak AIG: %d\n", taskName,
            Ddi_MgrReadPeakAigNodesNum(mcMgr->ddiMgr));
        }

        verificationResult = Trav_MgrReadAssertFlag(mcMgr->travMgr);

        break;

      case Mc_TaskIgr_c:
      case Mc_TaskIgr2_c:

        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpTimeLimit_c, inum,
          ((Fbv_Globals_t *) (mcMgr->auxPtr))->expt.itpTimeLimit);
        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpPeakAig_c, inum,
          ((Fbv_Globals_t *) (mcMgr->auxPtr))->expt.itpPeakAig);
        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpReuseRings_c, inum, 2);

        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravDynAbstr_c, inum, 0);
        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpAppr_c, inum, 0);

        //Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 2);
        if (mcMgr->settings.task == Mc_TaskIgr2_c) {
          taskName = "IGR2";
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrRewind_c, inum, 0);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum, 0);
          Ddi_MgrSetOption(mcMgr->ddiMgr, Pdt_DdiItpAigCore_c, inum, 50000);
          if ((nl < 1000) || ni < 100) {
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravImplAbstr_c, inum, 3);
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrSide_c, inum, 0);
            //            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 0);
            if (nl < 200) {
              Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum,
                0);
            }
          } else {
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravImplAbstr_c, inum, 0);
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravAbstrRef_c, inum, 0);
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpConstrLevel_c, inum,
              1);
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 2);
          }
          if (nl < 2000) {
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpInductiveTo_c, inum,
              2);
          } 
        } else if (1) {
          taskName = "IGR";
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum, 0);
          Ddi_MgrSetOption(mcMgr->ddiMgr, Pdt_DdiItpCore_c, inum, 30000);
          Ddi_MgrSetOption(mcMgr->ddiMgr, Pdt_DdiItpAigCore_c, inum, 30000);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrSide_c, inum, 1);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrRewind_c, inum, 0);
	  Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
	    fprintf(stdout, "IGR setup: ");
	  }
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum, 0);
          if ((nl < 2500) && ni < 300) {
	    //Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrRewind_c, inum, 0);
	    Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
	      fprintf(stdout, "1");
	    }
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravImplAbstr_c, inum, 3);
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpInductiveTo_c, inum,2);
            if (nl < 1200 && nl > 300 && ng > 5000) {
              //	      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravAbstrRef_c, inum, 2);
	      Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
		fprintf(stdout, "a");
	      }
	    }
            if (nl < 300) {
	      Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
		fprintf(stdout, "b");
	      }
	      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravImplAbstr_c, inum, 0);
	      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpInductiveTo_c, inum,0);
	      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrUseRings_c, inum, 30);
              //	      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrRewind_c, inum, 12);

	      if (nl < 200) {
		Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum,
                0);
	      }
	    }
          } else {
	    Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
	      fprintf(stdout, "2");
	    }
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravImplAbstr_c, inum, 3);
	    //            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravAbstrRef_c, inum, 2);
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpConstrLevel_c, inum,
              1);
	    Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum, 0);
	    //Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 2);
	    Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 0);
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrSide_c, inum, 0);
          }
          if (nl > 10000 || ng>25000) {
	    Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
	      fprintf(stdout, "c");
	    }
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravImplAbstr_c, inum, 0);
	    Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum, 0);
	    //Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 2);
	    Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 3);
	    if (nl > 10000) {
	      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrSide_c, inum, 0);
              Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 2);
              Ddi_MgrSetOption(mcMgr->ddiMgr, Pdt_DdiItpDrup_c, inum, 1);
              Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpInductiveTo_c, inum,2);
	      Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
		fprintf(stdout, "d");
	      }
	    }
	    else if (nl < 3000 && ng < 20000 || nl < 1000 && ng < 50000) {
	      Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
		fprintf(stdout, "e");
	      }
	      Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrSide_c, inum, 0);
	    }
          }
          if ((nl < 2000) && (ng < 50000)) {
	    int itpInd = 2;
	    Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
	      fprintf(stdout, "f");
	    }
	    if (ni < 100) {
	      itpInd = 1;
	      Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
		fprintf(stdout, "g");
	     }
	    }
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpEnToPlusImg_c, inum,0);
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpInductiveTo_c, inum,
              itpInd);
            if ((nl < 1000) && (ng < 20000)) {
              Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 2);
              Ddi_MgrSetOption(mcMgr->ddiMgr, Pdt_DdiItpDrup_c, inum, 1);
            }
          }
	  else if (0) {
	    // Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 0);
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrSide_c, inum, 0);
	    Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrRewind_c, inum, 0);
	  }
	  Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
	    fprintf(stdout, "\n");
	  }

        }
        mcMgr->settings.bmcTimeLimit = -1 /*60 */ ;
        mcMgr->settings.lazy = 1;
        mcMgr->settings.lazyRate = 1.0;

        Trav_TravSatItpVerif(mcMgr->travMgr, mcMgr->fsmMgr,
	  init, invarspec, NULL, invar, NULL /*care */ , mcMgr->settings.lemmas,
          -1 /*itpBoundLimit */ ,
          mcMgr->settings.lazyRate, -1, -1 /*itpTimeLimit */ );

        Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
          fprintf(stdout, "THRD: %s - peak AIG: %d\n", taskName,
            Ddi_MgrReadPeakAigNodesNum(mcMgr->ddiMgr));
        }
        verificationResult = Trav_MgrReadAssertFlag(mcMgr->travMgr);

        break;

      case Mc_TaskIgrPdr_c:

        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpTimeLimit_c, inum,
          ((Fbv_Globals_t *) (mcMgr->auxPtr))->expt.itpTimeLimit);
        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpPeakAig_c, inum,
          ((Fbv_Globals_t *) (mcMgr->auxPtr))->expt.itpPeakAig);
        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpReuseRings_c, inum, 2);

        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravDynAbstr_c, inum, 0);
        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpAppr_c, inum, 0);

        taskName = "IGRPDR";
        Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum, 2);
        if ((nl < 1000) || ni < 100) {
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravImplAbstr_c, inum, 3);
          if (nl < 200) {
            Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpBoundkOpt_c, inum,
              0);
          }
        } else {
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravImplAbstr_c, inum, 0);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravAbstrRef_c, inum, 2);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpConstrLevel_c, inum,
            1);
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravIgrGrowCone_c, inum, 2);
        }
        if (nl < 2000) {
          Trav_MgrSetOption(mcMgr->travMgr, Pdt_TravItpInductiveTo_c, inum,
            2);
        }
        mcMgr->settings.bmcTimeLimit = -1 /*60 */ ;
        mcMgr->settings.lazy = 1;
        mcMgr->settings.lazyRate = 1.0;
        Trav_TravIgrPdrVerif(mcMgr->travMgr, mcMgr->fsmMgr, -1);
        Pdtutil_VerbosityMgrIf(mcMgr, Pdtutil_VerbLevelUsrMed_c) {
          fprintf(stdout, "THRD: %s - peak AIG: %d\n", taskName,
            Ddi_MgrReadPeakAigNodesNum(mcMgr->ddiMgr));
        }
        verificationResult = Trav_MgrReadAssertFlag(mcMgr->travMgr);

        break;

      case Mc_TaskSim_c:
        {
          taskName = "SIM";

          //    fprintf(stdout, "RARITy ...\n");
          int resSim = Trav_SimulateRarity(mcMgr->travMgr, mcMgr->fsmMgr);

          //    fprintf(stdout, "RARITy %d ...\n",resSim);
          verificationResult = Trav_MgrReadAssertFlag(mcMgr->travMgr);
          if (verificationResult == 0) {
            verificationResult = -1;    // undecided
          }
        }
        break;

      case Mc_TaskNone_c:
      default:

        break;
    }

    if (xchgP != NULL) {
      // start xchg data
      *xchgP = NULL;
    }

    //    Trav_MgrQuit(mcMgr->travMgr);
    //    mcMgr->travMgr = NULL;

    if (verificationResult > 0) {
      verificationResult = 1;
    } else if (verificationResult < 0) {
      verificationResult = -2;
    }

  }
  FAIL {

#if 0
    sleep(1);
    Pdtutil_Verbosity(fprintf(stdout,
        "Invariant verification UNDECIDED after %s ...\n", mcMgr->taskName));
#endif
    Pdtutil_Verbosity(fprintf(stdout,
        "Invariant verification ABORTED by %d ...\n", mcMgr->settings.task));

    verificationResult = -2;
  }

  Pdtutil_Verbosity(fprintf(stdout,
      "Invariant verification TERMINATED with %d [%s]\n", verificationResult,
      taskName));

  return (verificationResult);
}

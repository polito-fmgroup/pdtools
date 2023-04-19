/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <math.h>

#include "mtl/Sort.h"
#include "coreDir/Solver.h"
#include "coreDir/ProofVisitor.h"

using namespace Minisat;

//=================================================================================================
// Options:


static const char* _cat = "CORE";

static DoubleOption  opt_var_decay         (_cat, "var-decay",   "The variable activity decay factor",            0.95,     DoubleRange(0, false, 1, false));
static DoubleOption  opt_clause_decay      (_cat, "cla-decay",   "The clause activity decay factor",              0.999,    DoubleRange(0, false, 1, false));
static DoubleOption  opt_random_var_freq   (_cat, "rnd-freq",    "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption  opt_random_seed       (_cat, "rnd-seed",    "Used by the random variable selection",         91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption     opt_ccmin_mode        (_cat, "ccmin-mode",  "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 0, IntRange(0, 2));
static IntOption     opt_phase_saving      (_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption    opt_rnd_init_act      (_cat, "rnd-init",    "Randomize the initial activity", false);
static BoolOption    opt_luby_restart      (_cat, "luby",        "Use the Luby restart sequence", true);
static IntOption     opt_restart_first     (_cat, "rfirst",      "The base restart interval", 100, IntRange(1, INT32_MAX));
static DoubleOption  opt_restart_inc       (_cat, "rinc",        "Restart interval increase factor", 2, DoubleRange(1, false, HUGE_VAL, false));
static DoubleOption  opt_garbage_frac      (_cat, "gc-frac",     "The fraction of wasted memory allowed before a garbage collection is triggered",  0.2 , DoubleRange(0, false, HUGE_VAL, false));

static BoolOption    opt_valid             (_cat, "valid",    "Validate UNSAT answers", false);


//=================================================================================================
// Constructor/Destructor:

#define GPC_DBG 0
#if GPC_DBG
static int trailAssL0[1000000];
static int tl0Phase, proof_i;
#endif

Solver::Solver() :

    // Parameters (user settable):
    //

    verbosity        (0)
  , log_proof (opt_valid)
  , ordered_propagate (false)
  , var_decay        (opt_var_decay)
  , clause_decay     (opt_clause_decay)
  , random_var_freq  (opt_random_var_freq)
  , random_seed      (opt_random_seed)
  , luby_restart     (opt_luby_restart)
  , ccmin_mode       (opt_ccmin_mode)
  , phase_saving     (opt_phase_saving)
  , rnd_pol          (false)
  , rnd_init_act     (opt_rnd_init_act)
  , garbage_frac     (opt_garbage_frac)
  , restart_first    (opt_restart_first)
  , restart_inc      (opt_restart_inc)

    // Parameters (the rest):
    //
  , learntsize_factor((double)1/(double)3), learntsize_inc(1.1)

    // Parameters (experimental):
    //
  , learntsize_adjust_start_confl (100)
  , learntsize_adjust_inc         (1.5)

    // Statistics: (formerly in 'SolverStats')
    //
  , solves(0), starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0)
  , dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)

  , ok                 (true)
  , cla_inc            (1)
  , var_inc            (1)
  , watches            (WatcherDeleted(ca))
  , qhead              (0)
  , simpDB_assigns     (-1)
  , simpDB_props       (0)
  , order_heap         (VarOrderLt(activity))
  , progress_estimate  (0)
  , remove_satisfied   (true)

    // Resource constraints:
    //
  , conflict_budget    (-1)
  , propagation_budget (-1)
  , asynch_interrupt   (false)
  , currentPart (1)
  , start(0)
{}


Solver::~Solver()
{
}


// === Validation
namespace
{

  //Structure that enables/disable scopped ordered propagation
  struct scopped_ordered_propagate
  {
    Solver &m;  //Link to solver
    bool m_mode; //Saved mode enabled
    scopped_ordered_propagate (Solver &s, bool v) : m(s)
    {
      m_mode = m.orderedPropagate ();
      m.orderedPropagate (v);
    }
    ~scopped_ordered_propagate () { m.orderedPropagate (m_mode); }
  };
}

// - Process clausal proof backwards
// - For each clause:
//    - If is unit undoes BCP
//    - Delete clause
//    - If is core validate it using RUP


inline void Solver::reInitClause(CRef r) {
  Clause &rc = ca [r];

  // -- move false literals to the end
  int sz = rc.size ();
  for (int i = 0; i < sz; ++i) {
    if (value (rc[i]) == l_False) {
      // -- push current literal to the end
      Lit l = rc[i];
      rc[i] = rc[sz-1];
      rc[sz-1] = l;
      sz--;
      i--;
    }
  }
  assert(value (rc[0]) != l_False);
  if (rc.size()==1 || (value (rc[1]) == l_False)) {
    Lit l = rc[0];
    if (value(l) == l_Undef)
      uncheckedEnqueue(l,r);
  }
  else {
    if (!isAttached(r)) 
      attachClause(r);
  }
}

//Validate the proof. Also produce the UNSAT core. Traverse the trail backwards, undoing BCP and detaching learnt clauses.
bool Solver::validate (ProofVisitor& v, bool coreFirst)
{
  assert (log_proof);
  assert (!ok || v.doUndef);
  assert (proof.size () > 0);

  vec<CRef> reInitClauses;
  bool handleReInit = false;
  
  int lastProofIndex = proof.size () - 2;

  int nUndeleted=0, nCore=0;
  //Activate scopped ordered propagation
  scopped_ordered_propagate scp_propagate (*this, true);

  if (v.doUndef) {
    lastProofIndex++;

#if 0
    for (int i = 0; i < v.undefTopClauses.size (); ++i)
      {
	CRef cr = v.undefTopClauses [i];
	Clause &cl = ca [cr];
	cl.core (1);
      }
#else
    for (int i = proof.size()-1; i >= 0; i--) {
      Clause &cl = ca [proof[i]];
      cl.core (1);
    }
#endif
    
  }
  else {
    //Add final conflct to the core
    // -- final conflict clause is in the core
    Clause &last = ca [proof.last ()];
    last.core (1);
    nCore++;
    
#if 0
    fprintf (stdout, "LAST ");
    for (int j = 0; j < last.size (); ++j)
      {
	fprintf (stdout, "%s%d", sign (last[j]) ? "-" : "",
		 var (last[j]) + 1);
	fprintf (stdout, " ");
      }
    fprintf (stdout, " 0\n");
#endif

    // -- mark all reasons for the final conflict as core
    for (int i = 0; i < last.size (); i++)
      {
	// -- validate that the clause is really a conflict clause
	if (value (last [i]) != l_False) return false;
	Var x = var (last [i]);
	ca [reason (x)].core (1);
      }
  }

  int trail_sz = trail.size ();
  ok = true;
  // -- move back through the proof, shrinking the trail and
  // -- validating the clauses

  //Traverse the proof backwards starting from the clause prior to the conflict

  int prevTrail = 100000000;
  for (int i = lastProofIndex; i >= 0; i--)
    {
      if (verbosity >= 2) fflush (stdout);

      //Get clause from the proof
      CRef cr = proof [i];
      assert (cr != CRef_Undef);
      Clause &c = ca [cr];

#if 0 

      if (trail.size()<prevTrail) {
	printf("trailV[P:%d] -> %d\n", i+2, prevTrail);
	prevTrail = trail.size();
      }
      if (cr==5001546 || cr==7760717) {
	printf("v found cr: %d\n", cr);
      }
#endif
      //if (verbosity >= 2) printf ("Validating lemma #%d ... ", i);

      // -- resurect deleted clauses
      if (c.mark () == 1)
        {
          // -- undelete
          c.mark (0);
          Var x = var (c[0]);

          nUndeleted++;
          // -- if non-unit clause, attach it
          if (c.size () > 1) attachClause (cr);
          else // -- if unit clause, enqueue it
            {
              bool res = enqueue (c[0], cr);
              assert (res);
            }
          if (verbosity >= 2) printf ("^");
          continue;
        }

      assert (c.mark () == 0);
      // -- detach the clause

      //Consider only clauses that propagate an assignment
      //It tries to revert search to the point when clause c was learnt
      //to do so it has to revert BCPs
      if (locked (c))
        {
          // -- undo the bcp
          //Revert assignments made propagating the asserting literal of the learnt clause
          while (trail[trail_sz - 1] != c[0])
            {
              Var x = var (trail [trail_sz - 1]);
              assigns [x] = l_Undef;
              if (handleReInit && !isAttached(reason(x))) 
                reInitClauses.push(reason(x));
#if GPC_DBG
	      if ((x==1780 || x==2931 || x==2194|| x==3561|| x==5001) 
		  && level(x)<=0) {
		printf("P(%d) - v: %d UNDONE!!!\n", i, (int)x);
	      }
#endif
              insertVarOrder (x);
              trail_sz--;

              //Mark as core the reasons of each literal of the reason clause
              CRef r = reason (x);
              assert (r != CRef_Undef);
              // -- mark literals of core clause as core
              if (ca [r].core ())
                {
                  Clause &rc = ca [r];
                  for (int j = 1; j < rc.size (); ++j)
                    {
                      Var x = var (rc [j]);
                      ca [reason (x)].core (1);
                    }
                }
            }
          assert (c[0] == trail [trail_sz - 1]);

          //Undo asserting literal assignment
          // -- unassign the variable
          assigns [var (c[0])] = l_Undef;

#if GPC_DBG
	  if ((var(c[0])==1780 || var(c[0])==2931 || var(c[0])==2194|| 
	       var(c[0])==3561|| var(c[0])==5001) 
	      && level(var(c[0]))<=0) {
	    printf("P(%d) - v: %d UNDONE\n", i, (int)var(c[0]));
	  }
#endif
          // -- put it back in order heap in case we want to restart
          // -- solving in the future
          insertVarOrder (var (c[0]));
          trail_sz--;
        }

      //Delete learnt clause
      // -- unit clauses don't need to be detached from watched literals
      if (c.size () > 1) detachClause (cr);
      // -- mark clause deleted
      c.mark (1);

      //If the considered proof clause is core validate it with RUP
      if (c.core () == 1)
        {
          assert (value (c[0]) == l_Undef);
          nCore++;
#if 0
	  fprintf (stdout, "CORE[%d] ", i);
	  for (int j = 0; j < c.size (); ++j)
	    {
	      fprintf (stdout, "%s%d", sign (c[j]) ? "-" : "",
		       var (c[j]) + 1);
	      fprintf (stdout, " ");
	    }
	  fprintf (stdout, " 0\n");
#endif

          // -- put trail in a good state
          trail.shrink (trail.size () - trail_sz);
          qhead = trail.size ();
          if (trail_lim.size () > 0) trail_lim [0] = trail.size ();
          if (verbosity >= 2) printf ("V");
          if (!validateLemma (cr,coreFirst)) return false;
        }
      else if (verbosity >= 2) printf ("-");


    }
  if (verbosity >= 2) printf ("\n");


  // update trail and qhead
  trail.shrink (trail.size () - trail_sz);
  qhead = trail.size ();
  if (trail_lim.size () > 0) trail_lim [0] = trail.size ();

  //The above loop terminate with the first learned clause. Some original clauses could be core and not yet discovered
  // find core clauses in the rest of the trail
  for (int i = trail.size () - 1; i >= 0; --i)
    {
      assert (reason (var (trail [i])) != CRef_Undef);
      Clause &c = ca [reason (var (trail [i]))];
      // -- if c is core, mark all clauses it depends as core
      if (c.core () == 1)
        for (int j = 1; j < c.size (); ++j)
          {
            Var x = var (c[j]);
            ca[reason (x)].core (1);
          }

    }
  
  while (reInitClauses.size()>0) {
    CRef r = reInitClauses.last();
    reInitClauses.pop();
    reInitClause(r);
  }
  // resort clause literals
  if (0)
  for (int i=0; i<nClauses(); i++) {
    CRef r = clauses[i];
    Clause& c = ca[r];
    int sz = c.size ();
    for (int i = 0; i < sz; ++i) {
      if (value (c[i]) == l_False) {
        // -- push current literal to the end
        Lit l = c[i];
        c[i] = c[sz-1];
        c[sz-1] = l;
        sz--;
        i--;
      }
    }
    assert(value (c[0]) != l_False);
  }
  CRef confl = propagate ();
  //  if (confl == CRef_Undef) return false;

  if (verbosity >= 1)
    printf("VALIDATION: core: %d - undeleted: %d - total: %d\n",
         nCore, nUndeleted, proof.size());
  if (verbosity >= 1) printf ("VALIDATED\n");
  return true;
}


//Validate core clause. Perform RUP to validate the clause. It is called when the trail is put in the state it was when
//the lemma was learnt. Propagating the negation of the lemma should result in a conflict.
//Perform RUP and mark as core all the clauses that cotribute deriving the lemma.
bool Solver::validateLemma (CRef cr, bool coreFirst)
{
  assert (decisionLevel () == 0);
  assert (ok);

  Clause &lemma = ca [cr];
  assert (lemma.core ());
  assert (!locked (lemma));

  bool prioritizeGlobals = false;

  //Set up a new decision level
  // -- go to decision level 1
  newDecisionLevel ();

  //Enqueue negation of the clause
  if (globals.size()==0 || !prioritizeGlobals) {
    for (int i = 0; i < lemma.size (); ++i)
      enqueue (~lemma [i]);
  }
  else if(prioritizeGlobals){
    for (int i = 0; i < lemma.size (); ++i) {
      if (globals[var(lemma[i])])
	enqueue (~lemma [i]);
    }
    for (int i = 0; i < lemma.size (); ++i) {
      if (!globals[var(lemma[i])])
	enqueue (~lemma [i]);
    }
  }

  //Set up another decision level
  // -- go to decision level 2
  newDecisionLevel ();

  //Propagate lemma
  // true param is to prioritize core clauses
  CRef confl = propagate (false,0,coreFirst); 
  //CRef confl = propagate ();
  if (confl == CRef_Undef)
    {
      if (verbosity >= 2) printf ("FAILED: No Conflict from propagate()\n");
      return false;
    }

  //Get conflict clause and mark it as core
  Clause &conflC = ca [confl];
  conflC.core (1);

  //Clauses that produces such conflict are the clauses that are used to derive
  //the lemma though resolution. We have to set those clauses as core as long as
  //every unit used to derive such conflict.

  //The clauses that contribute in causing the conflcit can be:
  // - Propagated from the assumed lemma negation.
  // - Assigned prior to RUP (those are core in turn because the contribute in deriving the current lemma)
  // - Assigned by the assumed lemma negation.

  //Set as core clauses those that are not propagated by the RUP but concur in determining
  //the conflict.
  for (int i = 0; i < conflC.size (); ++i)
    {
      Var x = var (conflC [i]);
      // -- if the variable got value by propagation,
      // -- mark it to be unrolled
      if (level (x) > 1) seen [x] = 1;
      else if (level (x) <= 0) ca [reason(x)].core (1);
    }

  //Mark as core every clause that assign literals of the lemma prior to the RUP
  // mark all level0 literals in the lemma as core
  for (int i = 0; i < lemma.size (); ++i)
    if (value (lemma[i]) != l_Undef && level (var (lemma [i])) <= 0)
      ca[reason (var (lemma [i]))].core (1);

  //Scan literals propagated by the assumed lemma negation that appear in the conflict
  for (int i = trail.size () - 1; i >= trail_lim[1]; i--)
    {
      Var x = var (trail [i]);
      if (!seen [x]) continue;

      //Set reason as core
      seen [x] = 0;
      assert (reason (x) != CRef_Undef);
      Clause &c = ca [reason (x)];
      c.core (1);

      assert (value (c[0]) == l_True);
      // -- for all other literals in the reason
      for (int j = 1; j < c.size (); ++j)
        {
          Var y = var (c [j]);
          assert (value (c [j]) == l_False);

          // -- if the literal is assigned at level 2,
          // -- mark it for processing
          if (level (y) > 1) seen [y] = 1;
          // -- else if the literal is assigned at level 0,
          // -- mark its reason clause as core
          else if (level (y) <= 0)
            // -- mark the reason for y as core
            ca[reason (y)].core (1);
        }
    }
  // reset
  cancelUntil (0);
  ok = true;
  return true;
}

//Replay clausal proof after validation
bool Solver::replay (ProofVisitor& v)
{
  assert (log_proof);
  assert (proof.size () > 0);
  //  verbosity = 3;
  if (verbosity >= 2) printf ("REPLAYING: ");
  long int maxCoreClSize=0, avgCoreClSize=0;
  long int maxNoCoreClSize=0, avgNoCoreClSize=0;
  int nCore=0;
  bool doLogSize=verbosity>0;
  
  // -- enter ordered propagate mode
  scopped_ordered_propagate scp_propagate (*this, true);


  //Now we are at decision level zero
  CRef confl = propagate (true);
  // -- assume that initial clause database is consistent
  assert (confl == CRef_Undef);

  //Trace resolutions that leades to zero level assignments
  labelLevel0(v);

  //  int prevTrail = -1;
  //  tl0Phase=1;
  //Process the clausal proof forward
  //  int start = (proof.size()>10) ? proof.size()/2 : 0;
  for (int i = 0; i < proof.size (); ++i)
    {
      if (verbosity >= 2) fflush (stdout);

      CRef cr = proof [i];
      assert (cr != CRef_Undef);
      Clause &c = ca [cr];

      if (doLogSize) {
        if (c.core ()) {
          nCore++;
          avgCoreClSize += c.size();
          if (c.size()>maxCoreClSize)
            maxCoreClSize = c.size();
        }
        else {
          avgNoCoreClSize += c.size();
          if (c.size()>maxNoCoreClSize)
            maxNoCoreClSize = c.size();
        }
      }
      
#if GPC_DBG
      proof_i = i;

      if (trail.size()>prevTrail) {
	printf("trail[P:%d] -> %d\n", i, trail.size());
	prevTrail = trail.size();
      }
      if (cr==5001546 || cr==7760717) {
	printf("found cr: %d\n", cr);
      }
      if (i==175838) {
	printf("found0!\n");
      }
      if (i==175860) {
	printf("found1!\n");
      }
#endif
      //Process only clauses that are core, undeleted in the original solver run
      //and not currently satisfied


      // -- delete clause that was deleted before
      // -- except for locked and core clauses
      if (c.mark () == 0 && !locked (c) && !c.core ())
        {
          if (c.size () > 1) detachClause (cr);
          c.mark (1);
          if (verbosity >= 2) printf ("-");
          continue;
        }
      // -- if current clause is not core or is already present or is satisfied
      // -- skip it and continue
      if (i>0 && i==proof.size()-1 && c.core()) {
        confl = cr;
        for (int i=0; i<c.size(); i++) {
          if (value (c [i]) != l_False) {
            confl = CRef_Undef;
            break;
          }
        }
        if (confl != CRef_Undef)
        {
          labelFinal(v, confl);
          break;
        }
      }

      if (c.core () == 0 || c.mark () == 0 || satisfied (c))
        {
	  //	  assert(!locked(c));
          if (verbosity >= 2) printf ("-");
          continue;
        }

      if (verbosity >= 2) printf ("v");

      // -- at least one literal must be undefined
      //assert (value (c[0]) == l_Undef);
      if  (value (c[0]) != l_Undef) {
        // count undef lits
        bool undefFound = false;
        for (int j = 0; j < c.size (); ++j) {
          if (value(c[j]) == l_Undef) {
            undefFound = true;
            break;
          }
        }
        if (!undefFound) {
          // null refutation found
          // shrink proof
          int reduce = proof.size()-1-i;
          //          proof.shrink(reduce);
        }
      }

      //Enter decision level 1
      newDecisionLevel (); // decision level 1

      //Enqueue the clause negation to perform RUP
      for (int j = 0; j < c.size (); ++j) {
	if (value(c[j]) == l_True) {
	  printf("proof clause is satisfied\n");
	}
	enqueue (~c[j]);
      }

      //Set up a new decision level
      newDecisionLevel (); // decision level 2

      //Propagate (should result in a conflict)
      CRef p = propagate (true);
      //      assert (p != CRef_Undef);

      // -- proof, extract interpolants, etc.
      // -- trail at decision level 0 is implied by the database
      // -- trail at decision level 1 are the decision forced by the clause
      // -- trail at decision level 2 is derived from level 1

      // -- undelete the clause and attach it to the database
      // -- unless the learned clause is already in the database
      if ((p != CRef_Undef) && traverseProof (v, cr, p))
      {
        cancelUntil (0);
        c.mark (0);
        if (verbosity >= 2 && shared (cr)) printf ("S");
        // -- if unit clause, add to trail and propagate
        if (c.size () <= 1 || (value (c[1]) == l_False && !c.learnt()))
	// GC:        if (c.size () <= 1)
        {
	  int isUnit = 1;
          assert (value (c[0]) == l_Undef);
	  if (0 && c.size()>1) {
	    //	    attachClause(cr);
	    printf("found[%d]: %d\n", i, cr);
	  }
#ifndef NDEBUG
          for (int j = 1; j < c.size (); ++j) {
	    assert (value (c[j]) == l_False);
	    //	    if (value (c[j]) != l_False) isUnit = 0;
	  }
#endif
	  if (isUnit) {
	    uncheckedEnqueue (c[0], cr);
	    confl = propagate (true);
	    labelLevel0(v);
	    // -- if got a conflict at level 0, bail out
	    if (confl != CRef_Undef)
	      {
		labelFinal(v, confl);
		break;
	      }
	  }
        }
        else attachClause (cr);
      }
      else
      {
        assert (c.core ());
        assert (c.mark ());
        // -- mark this clause as non-core. It is not part of the
        // -- proof.
        c.core (0);
        cancelUntil (0);
      }
    }

  if (proof.size () == 1) {
    confl = proof[0];
    labelFinal (v, proof [0]);
  }
  if (verbosity >= 2)
    {
      printf ("\n");
      fflush (stdout);
    }


  //  tl0Phase=0;
    //Restructure proof
    if(0) restructureProof();
    if (0) countPivots();

  if (verbosity >= 1 && confl != CRef_Undef) printf ("Replay SUCCESS\n");

  if (doLogSize) {
    int nNoCore = proof.size()-nCore;
    if (nNoCore>0 && nCore>0) {
      avgNoCoreClSize /= nNoCore;
      avgCoreClSize /= nCore;
      printf("REPLAY ENDED - core/nocore: %d/%d - cl size (core/nocore) max: %d/%d - avg: %d/%d\n",
           nCore, nNoCore, maxCoreClSize, maxNoCoreClSize,
           avgCoreClSize, avgNoCoreClSize);
    }
  }
  
  CRef cr = proof [proof.size()-1];
  assert (cr != CRef_Undef);
  if (1 && confl == CRef_Undef) 
    return false;
  else if (ca [cr].core()) return true;
  else return false;

}

void Solver::dumpGlobalPivots(int i){
    vec<Lit>& pivots = proofPdt.resNodes[i].pivots;
    for(int j = 0; j < pivots.size(); j++){
      bool global = isGlobal(pivots[j]);
      printf("%s%s%d ", global ? "#" : "@", sign(pivots[j])?"-":"", var(pivots[j]));
    }
    printf("\n");
}

void Solver::checkChainSoundness(int i){
  vec<int>& antecedents = proofPdt.resNodes[i].antecedents;
  vec<Lit>& pivots = proofPdt.resNodes[i].pivots;
  for(int j = 0; j < antecedents.size(); j++){
    vec<Lit>& cl = proofPdt.resNodes[antecedents[j]].resolvents;
    updateChainLevels(cl, j);
  }

  assert(chainLevels[var(pivots[0])] == 0);
  for(int j = 1; j < pivots.size(); j++){
    assert(chainLevels[var(pivots[j])] < j);
  }

  for(int j = 0; j < pivots.size(); j++){
    bool found = false;
    Var v = var(pivots[j]);
    vec<Lit>& antecendent = proofPdt.resNodes[antecedents[j+1]].resolvents;
    for(int k = 0; k < antecendent.size(); k++){
      if(var(antecendent[k]) == v){
        found = true;
        break;
      }
    }
    assert(found);
  }
}

void Solver::restructureProof(){

  assert(chainLevels.size() == nVars());

  for(int i = 0; i < proofPdt.resNodes.size(); i++){
    if(!proofPdt.resNodes[i].isChain()) continue;

    vec<int>& antecedents = proofPdt.resNodes[i].antecedents;
    vec<Lit>& pivots = proofPdt.resNodes[i].pivots;

    int numGlobals = 0;
    int numLocals = 0;
    for(int j = 0; j < pivots.size(); j++){
      if(isGlobal(pivots[j])) numGlobals++;
      else numLocals++;
    }
    //if(numGlobals > 2 && numLocals > 1)
      //dumpGlobalPivots(i);

    //Update chain levels
    for(int j = 0; j < antecedents.size(); j++){
      vec<Lit>& cl = proofPdt.resNodes[antecedents[j]].resolvents;
      updateChainLevels(cl, j);
    }

    //Try to group globals together
    bool globalRegion = true;
    for(int j = 0, k = 0; j < pivots.size(); j++){
      if(!isGlobal(pivots[j])){
        globalRegion = false;
        continue;
      } else if(globalRegion){
        k++;
        continue;
      } else {
        if(chainLevels[var(pivots[j])] < k){

          assert(!isGlobal(pivots[k]));

          //Switch pivots
          Lit temp = pivots[j];
          pivots[j] = pivots[k];
          pivots[k] = temp;

          //Switch clauses
          int temp2 = antecedents[j+1];
          antecedents[j+1] = antecedents[k+1];
          antecedents[k+1] = temp2;

          //Update chain levels
          vec<Lit>& goneUp = proofPdt.resNodes[antecedents[j+1]].resolvents;
          for(int q = 0; q < goneUp.size(); q++){
            if(chainLevels[var(goneUp[q])] == k){
              chainLevels[var(goneUp[q])] = j;
            }
          }
          for(int q = k+1; q < j+1; q++){
            vec<Lit>& cl = proofPdt.resNodes[antecedents[q]].resolvents;
            updateChainLevels(cl, q);
          }

          k++;

        } else {
          k = j+1;
          globalRegion = true;
        }
      }
    }

    //if(numGlobals > 2 && numLocals > 1)
      //dumpGlobalPivots(i);

    checkChainSoundness(i);

    //Clear chain levels
    for(int j = 0; j < nVars(); j++){
      chainLevels[j] = -1;
    }
  }

}

void Solver::countPivots(){

  vec<int> counters;
  int tot=0, nv=0, imax=-1;
  assert(chainLevels.size() == nVars());
  counters.growTo(nVars());

  maxPivots.growTo(1, -1);

  for(int i = 0; i < proofPdt.resNodes.size(); i++){
    if(!proofPdt.resNodes[i].isChain()) continue;

    vec<Lit>& pivots = proofPdt.resNodes[i].pivots;

    for(int j = 0; j < pivots.size(); j++){
      tot++;
      if (counters[var(pivots[j])]==0) nv++;
      counters[var(pivots[j])]++;
    }
  }

  if (nv>0) {
    printf("Avg pivot cnt (%d/%d vars): %d\n", nv, nVars(), tot/nv);
    for(int i = 0; i < nVars(); i++){
      if (imax<0 || counters[i]>counters[imax]) {
	imax=i;
      }
    }
    printf("Max pivot cnt [%d]: %d\n", imax, counters[imax]);
    maxPivots[0] = imax;
  }
  else {
    assert(tot==0);
  }
}


void Solver::updateChainLevels(vec<Lit>& clause, int level){
  level = (level == 0 ? 0 : level -1);
  for(int i = 0; i<clause.size(); i++){
    if((chainLevels[var(clause[i])] == -1) || (chainLevels[var(clause[i])] > level))
      chainLevels[var(clause[i])] = level;
  }
}



void Solver::labelFinal(ProofVisitor& v, CRef confl)
{
    // The conflict clause is the clause with which we resolve.
    const Clause& source = ca[confl];

    v.chainClauses.clear();
    v.chainPivots.clear();

    v.chainClauses.push(confl);
    // The clause is false, and results in the empty clause,
    // all are therefore seen and resolved.
    for (int i = 0; i < source.size (); ++i)
    {
      v.chainPivots.push(source [i]);
      v.chainClauses.push (CRef_Undef);
    }

    v.visitChainResolvent(CRef_Undef);
}

//Trace resolutions that leads to a learned clause that is part of the UNSAT core
bool Solver::traverseProof(ProofVisitor& v, CRef proofClause, CRef confl)
{
    // The clause with which we resolve.
    const Clause& proof = ca[proofClause];

    // The conflict clause
    const Clause& conflC = ca[confl];

    //Starting from the clause that conflicted when applying RUP to the learned clause
    //Set its variables as seen
    int pathC = conflC.size ();
    for (int i = 0; i < conflC.size (); ++i)
    {
        Var x = var (conflC [i]);
        seen[x] = 1;
    }

    //Clear trace proof visitor data
    v.chainClauses.clear();
    v.chainPivots.clear();

    //Push conflict in the resolution chain
    v.chainClauses.push(confl);
    Range range;
    range.join(conflC.part());

    //Now we apply conflict analysis to add each other antecedent clause to the chain
    // Now walk up the trail.
    for (int i = trail.size () - 1; pathC > 0; i--)
    {
        assert (i >= 0);
        Var x = var (trail [i]);
        if (!seen [x]) continue;

        seen [x] = 0;
        pathC--;

        //Skip variables assigned at RUP level
        if (level(x) == 1) continue;

        // --The pivot variable is x.

        assert (reason (x) != CRef_Undef);

        //Add pivot variable to the chain
        v.chainPivots.push(~trail [i]);
        CRef r = reason(x);
        range.join(ca[r].part());

        //Add antecedent clause to the chain
        if (level(x) > 0)
          v.chainClauses.push(r);
        else
        {
            v.chainClauses.push (CRef_Undef);
            range.join(trail_part[x]);
            continue;
        }

        Clause &rC = ca [r];
	if (rC.mark()) {
	  printf("not marked\n");
	}

        assert (value (rC[0]) == l_True);
        // -- for all other literals in the reason
        for (int j = 1; j < rC.size (); ++j)
        {
            if (seen [var (rC [j])] == 0)
            {
                seen [var (rC [j])] = 1;
                pathC++;
            }
        }
    }

    if (v.chainClauses.size () <= 1) return false;

    //if (range != ca[proofClause].part())
    //    printf("(%d,%d) vs (%d,%d)\n", range.min(), range.max(), ca[proofClause].part().min(), ca[proofClause].part().max());
    ca[proofClause].part(range);
    v.visitChainResolvent(proofClause);
    return true;
}


void Solver::labelLevel0(ProofVisitor& v)
{
    // -- Walk the trail forward
  while (start < trail.size ())
  {
    //Get assignments from the trail
    Lit q = trail [start++];
    Var x = var(q);
    assert(reason(x) != CRef_Undef);
    Clause& c = ca [reason(x)];
    Range r = c.part ();

    //Skip literals assigned at toplevel by unit clauses
    if (c.size () == 1) trail_part [x] = r;

    //If not assigned by a unit we need to record n-1 resolution steps where n
    //is the size of the assigned literal's reason clause

    // -- The number of resolution steps at this point is size-1
    // -- where size is the number of literals in the reason clause
    // -- for the unit that is currently on the trail.
    else if (c.size () == 2)
    {
      r.join (trail_part [var(c[1])]);
      trail_part [x] = r;

      //If binary record just a single binary resolution
      v.visitResolvent(q, ~c[1], reason(x)); // -- Binary resolution
    }
    else
    {

      //We need to record a resolution chain, cleans proof visitor variables
      v.chainClauses.clear ();
      v.chainPivots.clear ();

      //Push the first clause
      v.chainClauses.push (reason(x));

      //Starting from the second literal add a new resolution step pivot to the chain
      // -- The first literal (0) is the result of resolution, start from 1.
      for (int j = 1; j < c.size (); j++)
      {
        r.join (trail_part[var(c[j])]);
        v.chainPivots.push (c[j]);
        v.chainClauses.push (CRef_Undef);
      }
      trail_part [x] = r;

      //Trace resolution chain
      v.visitChainResolvent (q);
    }
  }
}

//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar)
{
    int v = nVars();
    watches  .init(mkLit(v, false));
    watches  .init(mkLit(v, true ));
    assigns  .push(l_Undef);
    vardata  .push(mkVarData(CRef_Undef, 0));
    //activity .push(0);
    activity .push(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
    seen     .push(0);
    polarity .push(sign);
    decision .push();
    trail    .capacity(v+1);

    //MARCO
    chainLevels.push(-1);

    setDecisionVar(v, dvar);

    partInfo.push(Range ());
    trail_part.push (Range ());

    return v;
}


bool Solver::addClause_(vec<Lit>& ps, Range part)
{
    assert(decisionLevel() == 0);
    assert (!log_proof || !part.undef ());

    if (!ok) return false;

    // Check if clause is satisfied and remove false/duplicate literals:
    sort(ps);
    Lit p; int i, j;
    if (log_proof)
      {
        // -- remove duplicates
        for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
          if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
          else if (ps[i] != p)
            ps[j++] = p = ps[i];
        ps.shrink(i - j);

        // -- move false literals to the end
        int sz = ps.size ();
        for (i = 0; i < sz; ++i)
          {
            if (value (ps [i]) == l_False)
              {
                // -- push current literal to the end
                Lit l = ps[i];
                ps[i] = ps[sz-1];
                ps[sz-1] = l;
                sz--;
                i--;
              }
          }
      }
    else
      {
        // -- AG: original minisat code
        for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
          if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
          else if (value(ps[i]) != l_False && ps[i] != p)
            ps[j++] = p = ps[i];
        ps.shrink(i - j);
      }

    if (ps.size() == 0)
        return ok = false;
    else if (log_proof && value (ps[0]) == l_False)
      {
        // -- this is the conflict clause, log it as the last clause in the proof
        CRef cr = ca.alloc (ps, false);
        Clause &c = ca[cr];
        c.part ().join (part);
        // clauses.push (cr); // GC: added for incremental SAT
        proof.push (cr);
        for (int i = 0; part.singleton () && i < ps.size (); ++i)
          partInfo [var (ps[i])].join (part);
        return ok = false;
      }
    else if (ps.size() == 1 || (log_proof && value (ps[1]) == l_False)){
      if (log_proof)
        {
          CRef cr = ca.alloc (ps, false);
          Clause &c = ca[cr];
          c.part ().join (part);
          clauses.push (cr);
          //dumpClause(clauses.size()-1, cr); //MARCO: debug
          totalPart.join (part);
          uncheckedEnqueue (ps[0], cr);
          // if (c.size()>1) attachClause(cr); // GC: just a try for replay
        }
      else
        uncheckedEnqueue(ps[0]);

      // -- mark variables as shared if necessary
      for (int i = 0; part.singleton () && i < ps.size (); ++i)
        partInfo [var (ps[i])].join (part);

      CRef confl = propagate ();
      if (log_proof && confl != CRef_Undef) proof.push (confl);
      return ok = (confl == CRef_Undef);
    }else{
        CRef cr = ca.alloc(ps, false);
        Clause& c = ca[cr];
        c.part ().join (part);
        clauses.push(cr);
        //dumpClause(clauses.size()-1, cr); //MARCO: debug
        totalPart.join (part);
        attachClause(cr);

        for (i = 0; part.singleton () && i < ps.size(); i++)
          partInfo[var (ps[i])].join (part);

    }

    return true;
}

void Solver::dumpClause(int id, CRef cr){
  Clause& cl = ca[cr];
  Var v;

  fprintf(stdout,"%d: ", id);
  for (int i=0; i<cl.size(); i++) {
    v = var(cl[i]);
    fprintf(stdout, "%s%d ", sign(cl[i])?"-":"", v);
  }
  fprintf(stdout,"0\n");
}

void Solver::dumpCNF(){
  for(int i = 0; i<clauses.size(); i++){
    dumpClause(i, clauses[i]);
  }
}

void Solver::attachClause(CRef cr) {
    const Clause& c = ca[cr];
    assert(c.size() > 1);
    watches[~c[0]].push(Watcher(cr, c[1]));
    watches[~c[1]].push(Watcher(cr, c[0]));
    if (c.learnt()) learnts_literals += c.size();
    else            clauses_literals += c.size(); }


void Solver::detachClause(CRef cr, bool strict) {
    const Clause& c = ca[cr];
    assert(c.size() > 1);

    if (strict){
        remove(watches[~c[0]], Watcher(cr, c[1]));
        remove(watches[~c[1]], Watcher(cr, c[0]));
    }else{
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
        watches.smudge(~c[0]);
        watches.smudge(~c[1]);
    }

    if (c.learnt()) learnts_literals -= c.size();
    else            clauses_literals -= c.size(); }


void Solver::removeClause(CRef cr) {
    if (log_proof) proof.push (cr);
    Clause& c = ca[cr];
    if (c.size () > 1) detachClause(cr);
    // Don't leave pointers to free'd memory!
    if (locked(c) && !log_proof) vardata[var(c[0])].reason = CRef_Undef;
    c.mark(1);
    if (!log_proof) ca.free(cr);
}


bool Solver::satisfied(const Clause& c) const {
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True)
            return true;
    return false; }


// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level) {
    if (decisionLevel() > level){
        for (int c = trail.size()-1; c >= trail_lim[level]; c--){
            Var      x  = var(trail[c]);
            assigns [x] = l_Undef;
            if (phase_saving > 1 || (phase_saving == 1) && c > trail_lim.last())
                polarity[x] = sign(trail[c]);
            insertVarOrder(x); }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    } }


//=================================================================================================
// Major methods:


Lit Solver::pickBranchLit()
{
    Var next = var_Undef;

    // Random decision:
    if (drand(random_seed) < random_var_freq && !order_heap.empty()){
        next = order_heap[irand(random_seed,order_heap.size())];
        if (value(next) == l_Undef && decision[next])
            rnd_decisions++; }

    // Activity based decision:
    while (next == var_Undef || value(next) != l_Undef || !decision[next])
        if (order_heap.empty()){
            next = var_Undef;
            break;
        }else
            next = order_heap.removeMin();

    return next == var_Undef ? lit_Undef : mkLit(next, rnd_pol ? drand(random_seed) < 0.5 : polarity[next]);
}


/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|
|  Description:
|    Analyze conflict and produce a reason clause.
|
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
|        rest of literals. There may be others from the same level though.
|
|________________________________________________________________________________________________@*/
void Solver::analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel,
                     Range &part)
{
    int pathC = 0;
    Lit p     = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    int index   = trail.size() - 1;

    //Initialize partition to the conflict clause one
    if (log_proof) part = ca [confl].part ();

    do{
        assert(confl != CRef_Undef); // (otherwise should be UIP)

        //Get current reason clause
        Clause& c = ca[confl];

        //Join partition with the current reason clause
        if (log_proof) part.join (c.part ());

        //Bump activty
        if (c.learnt())
            claBumpActivity(c);

        //Scan clause (skipping unit literal for non empty clauses)
        for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++){
            Lit q = c[j];

            //Skip previously encountered literals
            if (!seen[var(q)]){

              //Analyze clause
              if (level(var(q)) > 0)
                {
                  varBumpActivity(var(q));
                  seen[var(q)] = 1;
                  if (level(var(q)) >= decisionLevel())
                    pathC++;
                  else
                    out_learnt.push(q);
                }

              //If a literal is falsified by a unit clause join to the current partition
              //the partition of the unit clause
              else if (log_proof)
                {
                  assert (!trail_part[var (q)].undef ());
                  // update part based on partition of var(q)
                  part.join (trail_part [var (q)]);
                }
            }
        }

        // Select next clause to look at:
        while (!seen[var(trail[index--])]);
        p     = trail[index+1];
        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;

    }while (pathC > 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    int i, j;
    out_learnt.copyTo(analyze_toclear);
    if (ccmin_mode == 2){
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        for (i = j = 1; i < out_learnt.size(); i++)
          if (reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level, part))
                out_learnt[j++] = out_learnt[i];

    }else if (ccmin_mode == 1){
      assert (!log_proof);
        for (i = j = 1; i < out_learnt.size(); i++){
            Var x = var(out_learnt[i]);

            if (reason(x) == CRef_Undef)
                out_learnt[j++] = out_learnt[i];
            else{
                Clause& c = ca[reason(var(out_learnt[i]))];
                for (int k = 1; k < c.size(); k++)
                    if (!seen[var(c[k])] && level(var(c[k])) > 0){
                        out_learnt[j++] = out_learnt[i];
                        break; }
            }
        }
    }else
        i = j = out_learnt.size();

    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();

    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else{
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (int i = 2; i < out_learnt.size(); i++)
            if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
                max_i = i;
        // Swap-in this literal at index 1:
        Lit p             = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1]     = p;
        out_btlevel       = level(var(p));
    }

    for (int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;    // ('seen[]' is now cleared)
}


// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels, Range &part)
{
    analyze_stack.clear(); analyze_stack.push(p);
    // -- partition of all clauses used in the derivation to replace p
    Range lPart;
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0){
        assert(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause& c = ca[reason(var(analyze_stack.last()))]; analyze_stack.pop();
        if (log_proof) lPart.join (c.part ());

        for (int i = 1; i < c.size(); i++){
            Lit p  = c[i];
            if (!seen[var(p)]){
              if (level (var (p)) > 0)
              {
                if (reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0){
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                }else{
                    for (int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
              }
              else if (log_proof)
              {
                assert (!trail_part[var (p)].undef ());
                // update part based on partition of var(q)
                lPart.join (trail_part [var (p)]);
              }
            }
        }
    }

    if (log_proof) part.join (lPart);

    return true;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    seen[var(p)] = 1;

    for (int i = trail.size()-1; i >= trail_lim[0]; i--){
        Var x = var(trail[i]);
        if (seen[x]){
            if (reason(x) == CRef_Undef){
                assert(level(x) > 0);
                out_conflict.push(~trail[i]);
            }else{
                Clause& c = ca[reason(x)];
                for (int j = 1; j < c.size(); j++)
                    if (level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
            }
            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}


void Solver::uncheckedEnqueue(Lit p, CRef from)
{
    assert(value(p) == l_Undef);
    assigns[var(p)] = lbool(!sign(p));
    vardata[var(p)] = mkVarData(from, decisionLevel());
    trail.push_(p);
#if GPC_DBG
    if (level(var(p))<=0) {
      if (tl0Phase==0) {
	trailAssL0[var(p)] = proof.size()-1;
      }
      else {
	if (trailAssL0[var(p)] != proof_i) {
	  printf("AAACK!!!\n");
	}
      }
    }
    if ((var(p)==1780 || var(p)==2931 || var(p)==2194|| 
	 var(p)==3561|| var(p)==5001 ||var(p)==3345) 
	&& level(var(p))<=0) {
      printf("P(%d) - v: %d!!!\n", proof.size()-1, (int)var(p));
    }
    if ((var(p)==5424 || var(p)==4791) 
	&& level(var(p))<=0) {
      printf("P1(%d) - v: %d!!!\n", proof.size()-1, (int)var(p));
    }
#endif
    // -- everything at level 0 has a reason
    assert (!log_proof || decisionLevel () != 0 || from != CRef_Undef);


    //When assigning a top level literal set as is partition the partition
    //of the unit clause it was propagated from joined with the partition
    //of every other toplevel assigned clause
    if (log_proof && decisionLevel () == 0)
      {
        Clause &c = ca[from];
        Var x = var (p);

        assert (!c.part ().undef ());

        trail_part [x] = c.part ();
        for (int i = 1; i < c.size (); ++i)
          trail_part [x].join (trail_part[var (c[i])]);
      }

}


/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise CRef_Undef.
|
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
CRef Solver::propagate(bool coreOnly, int maxPart, bool coreFirst)
{

    CRef    confl     = CRef_Undef;

    // -- in ordered propagate mode, keep applying propagate with increasing partitions
    if (coreFirst)
    {
      int init_qhead = qhead;

      confl = propagate (true, maxPart);

      if (confl == CRef_Undef) {
        // -- reset qhead to the beginning
        qhead = init_qhead;
        // -- propagate up to (and including) partition i
        confl = propagate (false, maxPart);
      }
      return confl;
    }

    // -- in ordered propagate mode, keep applying propagate with increasing partitions
    if (ordered_propagate && maxPart == 0)
    {
      int init_qhead = qhead;
      for (int i = 1; confl == CRef_Undef && i <= totalPart.max (); i++)
      {
        // -- reset qhead to the beginning
        qhead = init_qhead;
        // -- propagate up to (and including) partition i
        confl = propagate (coreOnly, i);
      }
      return confl;
    }

    int     num_props = 0;
    watches.cleanAll();

    while (qhead < trail.size()){
        Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Watcher>&  ws  = watches[p];
        Watcher        *i, *j, *end;
        num_props++;

        //sort (ws, WatcherLt(ca));
        for (i = j = (Watcher*)ws, end = i + ws.size();  i != end;){
            // Try to avoid inspecting the clause:
            Lit blocker = i->blocker;
            if (value(blocker) == l_True){
                *j++ = *i++; continue; }

            CRef     cr        = i->cref;
            Clause&  c         = ca[cr];

            if (coreOnly && !c.core ()) { *j++ = *i++; continue; }

            //Exclude clause based on its partition
            if (maxPart > 0 && maxPart < c.part ().max ()) { *j++ = *i++; continue; }

            // Make sure the false literal is data[1]:
            Lit      false_lit = ~p;
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;
            assert(c[1] == false_lit);
            i++;

            // If 0th watch is true, then clause is already satisfied.
            Lit     first = c[0];
            Watcher w     = Watcher(cr, first);
            if (first != blocker && value(first) == l_True){
                *j++ = w; continue; }

            // Look for new watch:
            for (int k = 2; k < c.size(); k++)
                if (value(c[k]) != l_False){
                    c[1] = c[k]; c[k] = false_lit;
                    watches[~c[1]].push(w);
                    goto NextClause; }

            // Did not find watch -- clause is unit under assignment:
            *j++ = w;
            if (value(first) == l_False){
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                while (i < end)
                    *j++ = *i++;
            }else
                uncheckedEnqueue(first, cr);

        NextClause:;
        }
        ws.shrink(i - j);
    }
    propagations += num_props;
    simpDB_props -= num_props;

    return confl;
}


/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt {
    ClauseAllocator& ca;
    reduceDB_lt(ClauseAllocator& ca_) : ca(ca_) {}
    bool operator () (CRef x, CRef y) {
        return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity()); }
};
void Solver::reduceDB()
{
    int     i, j;
    double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity

    sort(learnts, reduceDB_lt(ca));
    // Don't delete binary or locked clauses. From the rest, delete clauses from the first half
    // and clauses with activity smaller than 'extra_lim':
    for (i = j = 0; i < learnts.size(); i++){
        Clause& c = ca[learnts[i]];
        if (c.size() > 2 && !locked(c) && (i < learnts.size() / 2 || c.activity() < extra_lim))
            removeClause(learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    learnts.shrink(i - j);
    checkGarbage();
}


void Solver::removeSatisfied(vec<CRef>& cs)
{
    int i, j;
    for (i = j = 0; i < cs.size(); i++){
        Clause& c = ca[cs[i]];
        if (satisfied(c)){
            removeClause(cs[i]);
        }
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}


void Solver::rebuildOrderHeap()
{
    vec<Var> vs;
    for (Var v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef)
            vs.push(v);
    order_heap.build(vs);
}


/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplify()
{
    assert(decisionLevel() == 0);

    if (!ok) return false;

    CRef confl = propagate ();
    if (confl != CRef_Undef)
    {
      if (log_proof) proof.push (confl);
      return ok = false;
    }

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
        return true;

    // Remove satisfied clauses:
    removeSatisfied(learnts);
    if (remove_satisfied)        // Can be turned off.
        removeSatisfied(clauses);
    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    return true;
}


/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
|
|  Description:
|    Search for a model the specified number of conflicts.
|    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
|
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts)
{
    assert(ok);
    int         backtrack_level;
    int         conflictC = 0;
    vec<Lit>    learnt_clause;
    Range       part;
    starts++;

    for (;;){
        CRef confl = propagate();
        if (confl != CRef_Undef){
            // CONFLICT
            conflicts++; conflictC++;
            if (decisionLevel() == 0)
              {
                if (log_proof) proof.push (confl);
                return l_False;
              }

            learnt_clause.clear();
            analyze(confl, learnt_clause, backtrack_level, part);
            cancelUntil(backtrack_level);

            if (learnt_clause.size() == 1){
              if (log_proof)
                {
                  // Need to log learned unit clauses in the proof
                  CRef cr = ca.alloc (learnt_clause, true);
                  proof.push (cr);
                  ca[cr].part (part);
                  uncheckedEnqueue (learnt_clause [0], cr);
                }
              else
                uncheckedEnqueue(learnt_clause[0]);
            }else if (learnt_clause.size()) {
                CRef cr = ca.alloc(learnt_clause, true);
                if (log_proof) proof.push (cr);
                if (log_proof) ca[cr].part (part);
                learnts.push(cr);
                attachClause(cr);
                claBumpActivity(ca[cr]);
                uncheckedEnqueue(learnt_clause[0], cr);
            }

            varDecayActivity();
            claDecayActivity();

            if (--learntsize_adjust_cnt == 0){
                learntsize_adjust_confl *= learntsize_adjust_inc;
                learntsize_adjust_cnt    = (int)learntsize_adjust_confl;
                max_learnts             *= learntsize_inc;

                if (verbosity >= 1)
                    printf("| %9d | %7d %8d %8d | %8d %8d %6.0f | %6.3f %% |\n",
                           (int)conflicts,
                           (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), nClauses(), (int)clauses_literals,
                           (int)max_learnts, nLearnts(), (double)learnts_literals/nLearnts(), progressEstimate()*100);
            }

        }else{
            // NO CONFLICT
            if (nof_conflicts >= 0 && conflictC >= nof_conflicts || !withinBudget()){
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                cancelUntil(0);
                return l_Undef; }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify())
                return l_False;

            if (learnts.size()-nAssigns() >= max_learnts)
                // Reduce the set of learnt clauses:
                reduceDB();

            Lit next = lit_Undef;
            while (decisionLevel() < assumptions.size()){
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if (value(p) == l_True){
                    // Dummy decision level:
                    newDecisionLevel();
                }else if (value(p) == l_False){
                    analyzeFinal(~p, conflict);
                    return l_False;
                }else{
                    next = p;
                    break;
                }
            }

            if (next == lit_Undef){
                // New variable decision:
                decisions++;
                next = pickBranchLit();

                if (next == lit_Undef)
                    // Model found:
                    return l_True;
                if (varDecisions.size() > var(next)) {
                  varDecisions[var(next)]++;
                }
            }

            // Increase decision level and enqueue 'next'
            newDecisionLevel();
            uncheckedEnqueue(next);
        }
    }
}


double Solver::progressEstimate() const
{
    double  progress = 0;
    double  F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++){
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}

/*
  Finite subsequences of the Luby-sequence:

  0: 1
  1: 1 1 2
  2: 1 1 2 1 1 2 4
  3: 1 1 2 1 1 2 4 1 1 2 1 1 2 4 8
  ...


 */

static double luby(double y, int x){

    // Find the finite subsequence that contains index 'x', and the
    // size of that subsequence:
    int size, seq;
    for (size = 1, seq = 0; size < x+1; seq++, size = 2*size+1);

    while (size-1 != x){
        size = (size-1)>>1;
        seq--;
        x = x % size;
    }

    return pow(y, seq);
}

// NOTE: assumptions passed in member-variable 'assumptions'.
lbool Solver::solve_()
{
    model.clear();
    conflict.clear();
    if (!ok) return l_False;

    solves++;

    max_learnts               = nClauses() * learntsize_factor;
    learntsize_adjust_confl   = learntsize_adjust_start_confl;
    learntsize_adjust_cnt     = (int)learntsize_adjust_confl;
    lbool   status            = l_Undef;

    if (verbosity >= 1){
        printf("============================[ Search Statistics ]==============================\n");
        printf("| Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
        printf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
        printf("===============================================================================\n");
    }

    // Search:
    int curr_restarts = 0;
    while (status == l_Undef){
        double rest_base = luby_restart ? luby(restart_inc, curr_restarts) : pow(restart_inc, curr_restarts);
        status = search(rest_base * restart_first);
        if (!withinBudget()) break;
        curr_restarts++;
    }

    if (verbosity >= 1)
        printf("===============================================================================\n");


    if (status == l_True){
        // Extend & copy model:
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
    }else if (status == l_False && conflict.size() == 0)
        ok = false;

    cancelUntil(0);
    return status;
}

//=================================================================================================
// Writing CNF to DIMACS:
//
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var>& map, Var& max)
{
    if (map.size() <= x || map[x] == -1){
        map.growTo(x+1, -1);
        map[x] = max++;
    }
    return map[x];
}


void Solver::toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max)
{
    //if (satisfied(c)) return;

    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) != l_False)
            fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max)+1);
    fprintf(f, "0\n");
}


void Solver::toDimacs(const char *file, const vec<Lit>& assumps)
{
    FILE* f = fopen(file, "wr");
    if (f == NULL)
        fprintf(stderr, "could not open file %s\n", file), exit(1);
    toDimacs(f, assumps);
    fclose(f);
}


void Solver::toDimacs(FILE* f, const vec<Lit>& assumps)
{
    // Handle case when solver is in contradictory state:
    if (!ok){
        fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
        return; }

    vec<Var> map; Var max = 0;

    // Cannot use removeClauses here because it is not safe
    // to deallocate them at this point. Could be improved.
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        //if (!satisfied(ca[clauses[i]]))
            cnt++;

    for (int i = 0; i < clauses.size(); i++)
        if (true) {//!satisfied(ca[clauses[i]])){
            Clause& c = ca[clauses[i]];
            for (int j = 0; j < c.size(); j++)
                if (value(c[j]) != l_False)
                    mapVar(var(c[j]), map, max);
        }

    // Assumptions are added as unit clauses:
    cnt += assumptions.size();

    fprintf(f, "p cnf %d %d\n", max, cnt);

    for (int i = 0; i < assumptions.size(); i++){
        assert(value(assumptions[i]) != l_False);
        fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "", mapVar(var(assumptions[i]), map, max)+1);
    }

    assert(totalPart.min() == 1);
	for (int part = 1; part <= totalPart.max(); part++)
	{
		fprintf(f, "c partition\n");
		for (int i = 0; i < clauses.size(); i++)
			if (part == ca[clauses[i]].part().max())
				toDimacs(f, ca[clauses[i]], map, max);
	}

    if (verbosity > 0)
        printf("Wrote %d clauses with %d variables.\n", cnt, max);
}


//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator& to)
{
    // All watchers:
    //
    // for (int i = 0; i < watches.size(); i++)
    watches.cleanAll();
    for (int v = 0; v < nVars(); v++)
        for (int s = 0; s < 2; s++){
            Lit p = mkLit(v, s);
            // printf(" >>> RELOCING: %s%d\n", sign(p)?"-":"", var(p)+1);
            vec<Watcher>& ws = watches[p];
            for (int j = 0; j < ws.size(); j++)
                ca.reloc(ws[j].cref, to);
        }

    // All reasons:
    //
    for (int i = 0; i < trail.size(); i++){
        Var v = var(trail[i]);

        if (reason(v) != CRef_Undef && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
            ca.reloc(vardata[v].reason, to);
    }

    // All learnt:
    //
    for (int i = 0; i < learnts.size(); i++)
        ca.reloc(learnts[i], to);

    // All original:
    //
    for (int i = 0; i < clauses.size(); i++)
        ca.reloc(clauses[i], to);

    // Clausal proof:
    //
    for (int i = 0; i < proof.size (); i++)
      ca.reloc (proof[i], to);
}




void Solver::garbageCollect()
{
  //assert (!log_proof);
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(ca.size() - ca.wasted());

    relocAll(to);
    if (verbosity >= 2)
        printf("|  Garbage collection:   %12d bytes => %12d bytes             |\n",
               ca.size()*ClauseAllocator::Unit_Size, to.size()*ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}

void Solver::printTrail(void)
 {
    for (int i=0; i<trail.size(); i++) {
      printf("t[%d]: %d ", i, trail[i]); printf("\n");
    }
  }




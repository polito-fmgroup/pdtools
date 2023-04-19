/***********************************************************************************[SimpSolver.cc]
Copyright (c) 2006,      Niklas Een, Niklas Sorensson
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

#include "mtl/Sort.h"
#include "simp/SimpSolver.h"
#include "utils/System.h"

using namespace Minisat;

//=================================================================================================
// Options:


static const char* _cat = "SIMP";

static BoolOption   opt_use_asymm        (_cat, "asymm",        "Shrink clauses by asymmetric branching.", false);
static BoolOption   opt_use_rcheck       (_cat, "rcheck",       "Check if a clause is already implied. (costly)", false);
static BoolOption   opt_use_elim         (_cat, "elim",         "Perform variable elimination.", true);
//static BoolOption   opt_use_elim         (_cat, "elim",         "Perform variable elimination.", false);
static IntOption    opt_grow             (_cat, "grow",         "Allow a variable elimination step to grow by a number of clauses.", 0);
static IntOption    opt_clause_lim       (_cat, "cl-lim",       "Variables are not eliminated if it produces a resolvent with a length above this limit. -1 means no limit", 20,   IntRange(-1, INT32_MAX));
static IntOption    opt_subsumption_lim  (_cat, "sub-lim",      "Do not check if subsumption against a clause larger than this. -1 means no limit.", 1000, IntRange(-1, INT32_MAX));
static DoubleOption opt_simp_garbage_frac(_cat, "simp-gc-frac", "The fraction of wasted memory allowed before a garbage collection is triggered during simplification.",  0.5, DoubleRange(0, false, HUGE_VAL, false));


//=================================================================================================
// Constructor/Destructor:


SimpSolver::SimpSolver() :
    grow               (opt_grow)
  , clause_lim         (opt_clause_lim)
  , subsumption_lim    (opt_subsumption_lim)
  , simp_garbage_frac  (opt_simp_garbage_frac)
  , use_asymm          (opt_use_asymm)
  , use_rcheck         (opt_use_rcheck)
  , use_elim           (opt_use_elim)
  , merges             (0)
  , asymm_lits         (0)
  , eliminated_vars    (0)
  , eliminated_clauses (0)
  , elimorder          (1)
  , use_simplification (true)
  , occurs             (ClauseDeleted(ca))
  , elim_heap          (ElimLt(n_occ))
  , bwdsub_assigns     (0)
  , n_touched          (0)
{
    vec<Lit> dummy(1,lit_Undef);
    ca.extra_clause_field = true; // NOTE: must happen before allocating the dummy clause below.
    bwdsub_tmpunit        = ca.alloc(dummy);
    remove_satisfied      = false;
}


SimpSolver::~SimpSolver()
{
}


Var SimpSolver::newVar(bool sign, bool dvar) {
    Var v = Solver::newVar(sign, dvar);

    frozen    .push((char)false);
    eliminated.push((char)false);

    if (use_simplification){
        n_occ     .push(0);
        n_occ     .push(0);
        occurs    .init(v);
        touched   .push(0);
        elim_heap .insert(v);
    }
    return v; }



lbool SimpSolver::solve_(bool do_simp, bool turn_off_simp)
{
    vec<Var> extra_frozen;
    lbool    result = l_True;

    do_simp &= use_simplification;

    if (do_simp){
        // Assumptions must be temporarily frozen to run variable elimination:
        for (int i = 0; i < assumptions.size(); i++){
            Var v = var(assumptions[i]);

            // If an assumption has been eliminated, remember it.
            assert(!isEliminated(v));

            if (!frozen[v]){
                // Freeze and store.
                setFrozen(v, true);
                extra_frozen.push(v);
            } }

        result = lbool(eliminate(turn_off_simp));
    }

    if (result == l_True)
        result = Solver::solve_();
    else if (verbosity >= 1)
        printf("===============================================================================\n");

    if (result == l_True)
        extendModel();

    if (do_simp)
        // Unfreeze the assumptions that were frozen:
        for (int i = 0; i < extra_frozen.size(); i++)
            setFrozen(extra_frozen[i], false);

    return result;
}



bool SimpSolver::addClause_(vec<Lit>& ps, Range part)
{
#ifndef NDEBUG
    for (int i = 0; i < ps.size(); i++)
        assert(!use_simplification || !isEliminated(var(ps[i])));
#endif

    int nclauses = clauses.size();

    if (use_rcheck && implied(ps))
        return true;

    if (!Solver::addClause_(ps, part))
        return false;

    if (use_simplification && clauses.size() == nclauses + 1){
        CRef          cr = clauses.last();
        const Clause& c  = ca[cr];

        // -- AG: in proof logging mode unit clauses are put into clause db
        // -- AG: but, they should be ignored by the simplifier
        if (proofLogging () && c.size () <= 1) return true;
        if (proofLogging () && value (c[1]) == l_False) return true;

        // NOTE: the clause is added to the queue immediately and then
        // again during 'gatherTouchedClauses()'. If nothing happens
        // in between, it will only be checked once. Otherwise, it may
        // be checked twice unnecessarily. This is an unfortunate
        // consequence of how backward subsumption is used to mimic
        // forward subsumption.
        subsumption_queue.insert(cr);
        for (int i = 0; i < c.size(); i++){
            occurs[var(c[i])].push(cr);
            n_occ[toInt(c[i])]++;
            touched[var(c[i])] = 1;
            n_touched++;
            if (elim_heap.inHeap(var(c[i])))
                elim_heap.increase(var(c[i]));
        }
    }

    return true;
}


void SimpSolver::removeClause(CRef cr)
{
    const Clause& c = ca[cr];

    if (use_simplification)
        for (int i = 0; i < c.size(); i++){
            n_occ[toInt(c[i])]--;
            updateElimHeap(var(c[i]));
            occurs.smudge(var(c[i]));
        }
    
    unsigned loc;
    if (proofLogging () && proofLoc.has (cr, loc))
      proofLoc.remove (cr);
    
    Solver::removeClause(cr);
}


bool SimpSolver::strengthenClause (CRef cr, Lit l)
{
  assert(decisionLevel() == 0);
  assert(use_simplification);
  assert (!locked (ca[cr]));

  // FIX: this is too inefficient but would be nice to have (properly implemented)
  // if (!find(subsumption_queue, &c))
  subsumption_queue.insert(cr);

  // -- Allocate new clause
  CRef ncr = CRef_Undef;

  if (proofLogging ())
  {
    // -- Create a duplicate of c
    // -- Since c is changed in-place
    add_tmp.clear ();
    for (int i = 0; i < ca[cr].size (); ++i)
      add_tmp.push (ca[cr][i]);
    ncr = ca.alloc (add_tmp, ca[cr].learnt ());
    ca[ncr].mark (ca[cr].mark ());
    ca[ncr].core (ca[cr].core ());
    ca[ncr].part (ca[cr].part ());

    // -- if 'c' is already in the proof, replace it 
    // -- in there by the duplicate
    unsigned loc;
    if (proofLoc.has (cr, loc)) 
    {
      proof [loc] = ncr;
      // -- 'c' is no longer in the proof
      proofLoc.remove (cr);
    }
  }
  
  Clause &c = ca[cr];
  
  // -- modify c
  if (c.size() == 2){
    detachClause (cr, true);
    c.strengthen(l);
  }else{
    Lit q = c[2];
    detachClause(cr, true);
    c.strengthen(l);
    
    // -- if strengthen removed a watched literal, and the new
    // -- secondary watch is false, check for another literal to watch
    if (c[1] == q && value (c[1]) == l_False)
    {
      for (int i = 2; i < c.size (); ++i)
        if (value (c[i]) != l_False)
        {
          Lit tmp = c[1];
          c[1] = c[i], c[i] = tmp;
          break;
        }
    }
    
    // ensure that first literal is l_Undef
    if (value (c[1]) == l_Undef)
    {
      Lit tmp = c[1];
      c[1] = c[0], c[0] = tmp;
    }
    
    attachClause(cr);
    remove(occurs[var(l)], cr);
    n_occ[toInt(l)]--;
    updateElimHeap(var(l));
  }

  if (proofLogging ())
  {
    // -- mark new clause as deleted and place it in the proof to replace modified 'c'
    ca[ncr].mark (1);
    // -- log insertion of 'new' c
    proof.push (cr); 
    // mark location of 'c' in the proof
    proofLoc.insert (cr, proof.size () - 1);
    // -- log deletion of 'old' c
    proof.push (ncr);
  }

  
  // -- remove a clause if it is satisfied already
  // XXX something breaks when remove sat unit clauses
  if ((c.size () > 1 && value (c[0]) == l_True) ||
      (c.size () > 1 && value (c[1]) == l_True))
    removeClause (cr);
  else if (c.size () == 1 || (value (c[0]) == l_Undef && value (c[1]) == l_False))
  {
#ifndef NDEBUG
    for (int i = 2; i < c.size (); ++i) assert (value (c[i]) == l_False);
#endif
    // -- if new clause is a unit, propagate and bail out with a conflict
    enqueue (c[0], cr);
    CRef confl = propagate ();
    // -- conflict must be logged for a proper clausal proof
    if (proofLogging () && confl != CRef_Undef) proof.push (confl);
    return confl == CRef_Undef;
  }
  return true;
}


// Returns FALSE if clause is always satisfied ('out_clause' should not be used).
bool SimpSolver::merge(const Clause& _ps, const Clause& _qs, Var v, vec<Lit>& out_clause)
{
    merges++;
    out_clause.clear();

    bool  ps_smallest = _ps.size() < _qs.size();
    const Clause& ps  =  ps_smallest ? _qs : _ps;
    const Clause& qs  =  ps_smallest ? _ps : _qs;

    for (int i = 0; i < qs.size(); i++){
        if (var(qs[i]) != v){
            for (int j = 0; j < ps.size(); j++)
                if (var(ps[j]) == var(qs[i]))
                    if (ps[j] == ~qs[i])
                        return false;
                    else
                        goto next;
            out_clause.push(qs[i]);
        }
        next:;
    }

    for (int i = 0; i < ps.size(); i++)
        if (var(ps[i]) != v)
            out_clause.push(ps[i]);

    return true;
}


// Returns FALSE if clause is always satisfied.
bool SimpSolver::merge(const Clause& _ps, const Clause& _qs, Var v, int& size)
{
    merges++;

    bool  ps_smallest = _ps.size() < _qs.size();
    const Clause& ps  =  ps_smallest ? _qs : _ps;
    const Clause& qs  =  ps_smallest ? _ps : _qs;
    const Lit*  __ps  = (const Lit*)ps;
    const Lit*  __qs  = (const Lit*)qs;

    size = ps.size()-1;

    for (int i = 0; i < qs.size(); i++){
        if (var(__qs[i]) != v){
            for (int j = 0; j < ps.size(); j++)
                if (var(__ps[j]) == var(__qs[i]))
                    if (__ps[j] == ~__qs[i])
                        return false;
                    else
                        goto next;
            size++;
        }
        next:;
    }

    return true;
}


void SimpSolver::gatherTouchedClauses()
{
    if (n_touched == 0) return;

    int i,j;
    for (i = j = 0; i < subsumption_queue.size(); i++)
        if (ca[subsumption_queue[i]].mark() == 0)
            ca[subsumption_queue[i]].mark(2);

    for (i = 0; i < touched.size(); i++)
        if (touched[i]){
            const vec<CRef>& cs = occurs.lookup(i);
            for (j = 0; j < cs.size(); j++)
                if (ca[cs[j]].mark() == 0){
                    subsumption_queue.insert(cs[j]);
                    ca[cs[j]].mark(2);
                }
            touched[i] = 0;
        }

    for (i = 0; i < subsumption_queue.size(); i++)
        if (ca[subsumption_queue[i]].mark() == 2)
            ca[subsumption_queue[i]].mark(0);

    n_touched = 0;
}


bool SimpSolver::implied(const vec<Lit>& c)
{
    assert(decisionLevel() == 0);

    trail_lim.push(trail.size());
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True){
            cancelUntil(0);
            return true;
        }else if (value(c[i]) != l_False){
            assert(value(c[i]) == l_Undef);
            uncheckedEnqueue(~c[i]);
        }

    bool result = propagate() != CRef_Undef;
    cancelUntil(0);
    return result;
}


// Backward subsumption + backward subsumption resolution
bool SimpSolver::backwardSubsumptionCheck(bool verbose)
{
    int cnt = 0;
    int subsumed = 0;
    int deleted_literals = 0;
    assert(decisionLevel() == 0);

    while (subsumption_queue.size() > 0 || bwdsub_assigns < trail.size()){

        // Empty subsumption queue and return immediately on user-interrupt:
        if (asynch_interrupt){
            subsumption_queue.clear();
            bwdsub_assigns = trail.size();
            break; }

        // Check top-level assignments by creating a dummy clause and placing it in the queue:
        if (subsumption_queue.size() == 0 && bwdsub_assigns < trail.size()){
            Lit l = trail[bwdsub_assigns++];
            ca[bwdsub_tmpunit][0] = l;
            ca[bwdsub_tmpunit].calcAbstraction();
            subsumption_queue.insert(bwdsub_tmpunit); }

        CRef    cr = subsumption_queue.peek(); subsumption_queue.pop();
        Clause& c  = ca[cr];

        if (c.mark()) continue;

        if (verbose && verbosity >= 2 && cnt++ % 1000 == 0)
            printf("subsumption left: %10d (%10d subsumed, %10d deleted literals)\r", subsumption_queue.size(), subsumed, deleted_literals);

        assert(c.size() > 1 || value(c[0]) == l_True);    // Unit-clauses should have been propagated before this point.

        // Find best variable to scan:
        Var best = var(c[0]);
        for (int i = 1; i < c.size(); i++)
            if (occurs[var(c[i])].size() < occurs[best].size())
                best = var(c[i]);

        // Search all candidates:
        vec<CRef>& _cs = occurs.lookup(best);
        CRef*       cs = (CRef*)_cs;

        for (int j = 0; j < _cs.size(); j++)
        {
          Clause &c = ca[cr];
          if (c.mark())
            break;
          else if (!ca[cs[j]].mark() &&  cs[j] != cr && (subsumption_lim == -1 || ca[cs[j]].size() < subsumption_lim)){
            
            if (satisfied (ca[cs[j]]))
            {
              removeClause (cs[j]);
              continue;
            }
            
            Lit l = c.subsumes(ca[cs[j]]);

            if (l == lit_Undef)
              subsumed++, removeClause(cs[j]);
            else if (l != lit_Error){
              deleted_literals++;
              
              CRef csj = cs[j];
              // AG: the result of strengthenClause is a new clause that replaced cs[j]
              // AG: partition of new clause is cs[j].part ().join (c.part ())
              if (!strengthenClause(csj, ~l))
                return false;

              if (proofLogging ()) ca [csj].part ().join (ca[cr].part ());
                    

              // Did current candidate get deleted from cs? Then check candidate at index j again:
              if (var(l) == best)
                j--;
            }
          }
        }
        
    }

    return true;
}


bool SimpSolver::asymm(Var v, CRef cr)
{
    Clause& c = ca[cr];
    assert(decisionLevel() == 0);

    if (c.mark() || satisfied(c)) return true;

    trail_lim.push(trail.size());
    Lit l = lit_Undef;
    for (int i = 0; i < c.size(); i++)
        if (var(c[i]) != v && value(c[i]) != l_False)
            uncheckedEnqueue(~c[i]);
        else
            l = c[i];
    trail_lim.push (trail.size ());
    CRef confl = propagate ();
    if (confl != CRef_Undef){
        cancelUntil(1);
        Clause &conflC = ca[confl];
        bool allFalse = false;
        for (int i = 0; allFalse && conflC.size () <= c.size () && i < conflC.size (); ++i)
          allFalse = allFalse && (value (conflC[i]) == l_False);

        cancelUntil(0);
        if (allFalse)
        {
          removeClause (cr);
          return true;
        }
        
        asymm_lits++;
        /// AG: the result of strengthenClause is the new clause added to the proof
        /// AG: the new clause does not replace anything
        /// AG: partition of the new clause depends on analyzing propagate
        /// AG: simple solution is to set partition of new clause to the widest range
        /// AG: and let interpolation/validation figure out how the clause was derived
        if (!strengthenClause(cr, l))
            return false;
        if (proofLogging ())
        {
          ca [cr].part ().join (totalPart);
          /* 
          ca [cr].part ().join (1);
          ca [cr].part ().join (currentPart);
          */
        }
    }else
        cancelUntil(0);

    return true;
}


bool SimpSolver::asymmVar(Var v)
{
    assert(use_simplification);

    const vec<CRef>& cls = occurs.lookup(v);

    if (value(v) != l_Undef || cls.size() == 0)
        return true;

    for (int i = 0; i < cls.size(); i++)
        if (!asymm(v, cls[i]))
            return false;

    return backwardSubsumptionCheck();
}


static void mkElimClause(vec<uint32_t>& elimclauses, Lit x)
{
    elimclauses.push(toInt(x));
    elimclauses.push(1);

}


static void mkElimClause(vec<uint32_t>& elimclauses, Var v, Clause& c)
{
    int first = elimclauses.size();
    int v_pos = -1;

    // Copy clause to elimclauses-vector. Remember position where the
    // variable 'v' occurs:
    for (int i = 0; i < c.size(); i++){
      if (elimclauses.capacity() == elimclauses.size()) {
	elimclauses.growTo(elimclauses.capacity()*1.5);
      }
      elimclauses.push(toInt(c[i]));
      if (var(c[i]) == v)
            v_pos = i + first;
    }
    assert(v_pos != -1);

    // Swap the first literal with the 'v' literal, so that the literal
    // containing 'v' will occur first in the clause:
    uint32_t tmp = elimclauses[v_pos];
    elimclauses[v_pos] = elimclauses[first];
    elimclauses[first] = tmp;

    // Store the length of the clause last:
    elimclauses.push(c.size());
}



bool SimpSolver::eliminateVar(Var v)
{
    assert(!frozen[v]);
    assert(!isEliminated(v));
    assert(value(v) == l_Undef);

    // Split the occurrences into positive and negative:
    //
    const vec<CRef>& cls = occurs.lookup(v);
    vec<CRef>        pos, neg;
    for (int i = 0; i < cls.size(); i++)
        (find(ca[cls[i]], mkLit(v)) ? pos : neg).push(cls[i]);

    // Check wether the increase in number of clauses stays within the allowed ('grow'). Moreover, no
    // clause must exceed the limit on the maximal clause size (if it is set):
    //
    int cnt         = 0;
    int clause_size = 0;

    for (int i = 0; i < pos.size(); i++)
        for (int j = 0; j < neg.size(); j++)
            if (merge(ca[pos[i]], ca[neg[j]], v, clause_size) && 
                (++cnt > cls.size() + grow || (clause_lim != -1 && clause_size > clause_lim)))
                return true;

    // Delete and store old clauses:
    eliminated[v] = true;
    setDecisionVar(v, false);
    eliminated_vars++;

    if (pos.size() > neg.size()){
        for (int i = 0; i < neg.size(); i++)
            mkElimClause(elimclauses, v, ca[neg[i]]);
        mkElimClause(elimclauses, mkLit(v));
    }else{
        for (int i = 0; i < pos.size(); i++)
            mkElimClause(elimclauses, v, ca[pos[i]]);
        mkElimClause(elimclauses, ~mkLit(v));
    }

    // Produce clauses in cross product:
    vec<Lit>& resolvent = add_tmp;
    for (int i = 0; i < pos.size(); i++)
        for (int j = 0; j < neg.size(); j++)
        {
          Range part;
          part.join (ca [pos [i]].part ());
          part.join (ca [neg [j]].part ());
          int nclauses = clauses.size ();
          // merged clause is join of partitions of pos[i] and neg[j]
          // AG: partition of the resolvent is join of partitions of pos[i] and neg[j]
          if (merge(ca[pos[i]], ca[neg[j]], v, resolvent) && 
              !addClause_(resolvent, part))
                return false;
          if (proofLogging () && clauses.size () == nclauses + 1)
          {
            proof.push (clauses.last ());
            proofLoc.insert (clauses.last (), proof.size () - 1);
          }
        }
    
    for (int i = 0; i < cls.size(); i++)
        removeClause(cls[i]); 

    // Free occurs list for this variable:
    occurs[v].clear(true);
    
    // Free watchers lists for this variable, if possible:
    if (watches[ mkLit(v)].size() == 0) watches[ mkLit(v)].clear(true);
    if (watches[~mkLit(v)].size() == 0) watches[~mkLit(v)].clear(true);

    return backwardSubsumptionCheck();
}


// AG: Dead function. Ignore.
bool SimpSolver::substitute(Var v, Lit x)
{
  assert (!proofLogging ());
  
    assert(!frozen[v]);
    assert(!isEliminated(v));
    assert(value(v) == l_Undef);

    if (!ok) return false;

    eliminated[v] = true;
    setDecisionVar(v, false);
    const vec<CRef>& cls = occurs.lookup(v);
    
    vec<Lit>& subst_clause = add_tmp;
    for (int i = 0; i < cls.size(); i++){
        Clause& c = ca[cls[i]];

        subst_clause.clear();
        for (int j = 0; j < c.size(); j++){
            Lit p = c[j];
            subst_clause.push(var(p) == v ? x ^ sign(p) : p);
        }

        // AG: this call never happens because the enclosing function is never called
        // AG: (we update proof in removeClause)
        removeClause(cls[i]);

        if (!addClause_(subst_clause))
            return ok = false;
    }

    return true;
}


void SimpSolver::extendModel()
{
    int i, j;
    Lit x;

    for (i = elimclauses.size()-1; i > 0; i -= j){
        for (j = elimclauses[i--]; j > 1; j--, i--)
            if (modelValue(toLit(elimclauses[i])) != l_False)
                goto next;

        x = toLit(elimclauses[i]);
        model[var(x)] = lbool(!sign(x));
    next:;
    }
}


bool SimpSolver::eliminate(bool turn_off_elim)
{
    if (!simplify())
        return false;
    else if (!use_simplification)
        return true;

    int initialClauses = nClauses();
    // Main simplification loop:
    //
    while (n_touched > 0 || bwdsub_assigns < trail.size() || elim_heap.size() > 0){

        gatherTouchedClauses();
        // printf("  ## (time = %6.2f s) BWD-SUB: queue = %d, trail = %d\n", cpuTime(), subsumption_queue.size(), trail.size() - bwdsub_assigns);
        if ((subsumption_queue.size() > 0 || bwdsub_assigns < trail.size()) && 
            !backwardSubsumptionCheck(true)){
            ok = false; goto cleanup; }

        // Empty elim_heap and return immediately on user-interrupt:
        if (asynch_interrupt){
            assert(bwdsub_assigns == trail.size());
            assert(subsumption_queue.size() == 0);
            assert(n_touched == 0);
            elim_heap.clear();
            goto cleanup; }

        // printf("  ## (time = %6.2f s) ELIM: vars = %d\n", cpuTime(), elim_heap.size());
        for (int cnt = 0; !elim_heap.empty(); cnt++){
            Var elim = elim_heap.removeMin();
            
            if (asynch_interrupt) break;

            if (isEliminated(elim) || value(elim) != l_Undef) continue;

            if (verbosity >= 2 && cnt % 100 == 0)
                printf("elimination left: %10d\r", elim_heap.size());

            if (use_asymm){
                // Temporarily freeze variable. Otherwise, it would immediately end up on the queue again:
                bool was_frozen = frozen[elim];
                frozen[elim] = true;
                if (!asymmVar(elim)){
                    ok = false; goto cleanup; }
                frozen[elim] = was_frozen; }

            // At this point, the variable may have been set by assymetric branching, so check it
            // again. Also, don't eliminate frozen variables:
            if (use_elim && value(elim) == l_Undef && !frozen[elim] && !eliminateVar(elim)){
                ok = false; goto cleanup; }

            checkGarbage(simp_garbage_frac);
        }

        assert(subsumption_queue.size() == 0);
    }
 cleanup:

    // If no more simplification is needed, free all simplification-related data structures:
    if (turn_off_elim){
        touched  .clear(true);
        occurs   .clear(true);
        n_occ    .clear(true);
        elim_heap.clear(true);
        subsumption_queue.clear(true);

        use_simplification    = false;
        remove_satisfied      = true;
        ca.extra_clause_field = false;

        proofLoc.clear ();
        // Force full cleanup (this is safe and desirable since it only happens once):
        rebuildOrderHeap();
        garbageCollect();
    }else{
        // Cheaper cleanup:
        cleanUpClauses(); // TODO: can we make 'cleanUpClauses()' not be linear in the problem size somehow?
        checkGarbage();
    }

    if (verbosity >= 1 && elimclauses.size() > 0)
        printf("|  Eliminated clauses:     %10.2f Mb                                      |\n", 
               double(elimclauses.size() * sizeof(uint32_t)) / (1024*1024));

    eliminated_clauses = initialClauses - nClauses();

    return ok;
}


void SimpSolver::cleanUpClauses()
{
    occurs.cleanAll();
    int i,j;
    for (i = j = 0; i < clauses.size(); i++)
        if (ca[clauses[i]].mark() == 0)
            clauses[j++] = clauses[i];
    clauses.shrink(i - j);
}


//=================================================================================================
// Garbage Collection methods:


void SimpSolver::relocAll(ClauseAllocator& to)
{
    if (!use_simplification) return;

    // All occurs lists:
    //
    for (int i = 0; i < nVars(); i++){
        vec<CRef>& cs = occurs[i];
        for (int j = 0; j < cs.size(); j++)
            ca.reloc(cs[j], to);
    }

    // Subsumption queue:
    //
    for (int i = 0; i < subsumption_queue.size(); i++)
        ca.reloc(subsumption_queue[i], to);

    // Temporary clause:
    //
    ca.reloc(bwdsub_tmpunit, to);
}


void SimpSolver::garbageCollect()
{
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(ca.size() - ca.wasted()); 

    cleanUpClauses();
    to.extra_clause_field = ca.extra_clause_field; // NOTE: this is important to keep (or lose) the extra fields.
    relocAll(to);
    Solver::relocAll(to);
    if (verbosity >= 2)
        printf("|  Garbage collection:   %12d bytes => %12d bytes             |\n", 
               ca.size()*ClauseAllocator::Unit_Size, to.size()*ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}

int SimpSolver::solveWithRefinement(vec<Lit>& assumps, SolverPdt *pdtS) {
  // try with second solver
  SimpSolver S22r;
  int sat;

  if (pdtS == NULL) return -1;

  vec<int>& map = pdtS->getMap(), &invMap = pdtS->getInvMap();
  vec<char>& disClause = pdtS->getDisClause();
  int nCl0, nCl, nV0, nV, nLearntsLoaded=0, nDisabled=0, nChecked=0;
  int nChances = 2;

  double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity
  double  var_extra_lim = var_inc / nVars();    // Remove any clause below this activity
  double var_min_act = var_extra_lim/1000;

  nCl0 = disClause.size();
  nCl = clauses.size();

  invMap.clear();
  map.clear();
  //  disClause.clear();

  disClause.growTo(clauses.size());
  map.growTo(nVars());
  for (int v = 0; v < nVars(); v++){
    if (v>=nV0)
      map[v] = -1;
    if (assigns[v] != l_Undef) {
      int v2 = S22r.nVars();
      map[v] = v2;
      S22r.newVar();
      if (frozen[v]) S22r.setFrozen(v2, true);
      if (invMap.size()<=v2) {
	invMap.growTo(v2+1);
	invMap[v2] = v;
      }
      int s = assigns[v] == l_False;
      Lit p = mkLit(v2, s);
      S22r.addClause(p);
    }
  }

  for (int i = 0; i < learnts.size(); i++){
    Clause& c = ca[learnts[i]];
    vec<Lit> res;
    if (c.activity() < extra_lim) continue;
    for(int j=0;j<c.size();j++) {
      int s = sign(c[j]);
      int v = var(c[j]);
      int v2 = map[v];
      if (v2<0) {
	v2 = S22r.nVars();
	map[v] = v2;
	S22r.newVar();
	if (frozen[v]) S22r.setFrozen(v2, true);
	if (invMap.size()<=v2) {
	  invMap.growTo(v2+1);
	  invMap[v2] = v;
	}
      }
      res.push(mkLit(v2, s));
    }
    S22r.addClause(res);
    nLearntsLoaded++;
  }
  for (int i = 0; i < nCl0; i++){
    if (disClause[i]) {
      Clause& c = ca[clauses[i]];
      nChecked++;
      if (0) {
	int useClause = 0;
	for(int j=0; j<c.size();j++) {
	  int s = sign(c[j]);
	  int v = var(c[j]);
	  int v2 = map[v];
	  if (activity[v] > var_min_act) {
	    useClause = 1;
	    break;
	  }
	}
	if (!useClause) {
	  disClause[i]--;
	  if (disClause[i]==0) {
	    nDisabled++;
	    continue;
	  }
	}
      }
      vec<Lit> res;
      for(int j=0; j<c.size();j++) {
	int s = sign(c[j]);
	int v = var(c[j]);
	int v2 = map[v];
	if (v2<0) {
	  v2 = S22r.nVars();
	  map[v] = v2;
	  S22r.newVar();
	  if (frozen[v]) S22r.setFrozen(v2, true);
	  if (invMap.size()<=v2) {
	    invMap.growTo(v2+1);
	    invMap[v2] = v;
	  }
	}
	res.push(mkLit(v2, s));
      }
      S22r.addClause(res);
    }
  }


  vec<Lit> assumps2;
  for(int j=0;j<assumps.size();j++) {
    int s = sign(assumps[j]);
    int v = var(assumps[j]);
    int v2 = map[v];
    if (v2<0) {
      v2 = S22r.nVars();
      map[v] = v2;
      S22r.newVar();
      if (frozen[v]) S22r.setFrozen(v2, true);
    }
    assumps2.push(mkLit(v2, s));
  }

  int nRef, doRefine=1, nActive=0;
  for (nRef=0; doRefine; nRef++) {
    printf("aux solver iter %d: %d(%d) vars, %d/%d learnts, %d/%d clauses (%d/%d disabled)\n",
	   nRef+1, S22r.nVars(), nVars(), nLearntsLoaded, learnts.size(), 
	   S22r.nClauses()-nLearntsLoaded,clauses.size(), nDisabled, nChecked);

    S22r.use_simplification = false;
    sat = S22r.solve(assumps2);
    if (!sat) {
      doRefine = 0;
#if 0
      S22r.use_simplification = true;
      S22r.eliminate(false);

      printf("Solver stats: %ld/%ld vars/clauses - eliminated v/c: %d/%d\n",
	    S22r.nVars(), S22r.nClauses(),
	    S22r.eliminated_vars, S22r.eliminated_clauses);
      for (int v = 0; v < nVars(); v++){
	int v2 = map[v];
	if (v2>=0) {
	  if (!frozen[v] && S22r.frozen[v2]) 
	    setFrozen(v, true);
	}
      }
#endif
    }
    else {
      int nRefCl = 0, nPartialRef=0;
      for (int i = 0; i < clauses.size(); i++){
	if (!disClause[i]) {
	  Clause& c = ca[clauses[i]];
	  int useClause = 1, foundLits=0;
	  for(int j=0; useClause && j<c.size();j++) {
	    int s = sign(c[j]);
	    int v = var(c[j]);
	    int v2 = map[v];
	    if (1 && nRef==0 && activity[v] >= var_extra_lim) {
	      nActive++;
	      foundLits++;
	      //	      useClause = 1;
	      if (foundLits>1) break;
	    }
	    else if (v2>=0) {
	      int s2 = S22r.model[v2] == l_False;
	      if (s2==s) {
		useClause = 0;
		break;
	      }
	      foundLits++;
	    }
	  }
	  if (useClause && foundLits>0) {
	    vec<Lit> res;
	    for(int j=0; j<c.size();j++) {
	      int s = sign(c[j]);
	      int v = var(c[j]);
	      int v2 = map[v];
	      if (v2<0) {
		v2 = S22r.nVars();
		map[v] = v2;
		S22r.newVar();
		if (frozen[v]) S22r.setFrozen(v2, true);
		if (invMap.size()<=v2) {
		  invMap.growTo(v2+1);
		  invMap[v2] = v;
		}
	      }
	      res.push(mkLit(v2, s));
	    }
	    S22r.addClause(res);
	    if (foundLits<c.size()) nPartialRef++;
	    nRefCl++;
	    //if (nRefCl>clauses.size()/100 && nRef>0) break;
	    disClause[i] = nChances;
	  }
	}
      }
      printf("refined with %d clauses (%d partial) active: %d\n", nRefCl, nPartialRef, nActive);
    }
  }
  S22r.use_simplification = true;
  return sat;
}

int SimpSolver::solveWithLearntsAndSimplify(vec<Lit>& assumps, SolverPdt *pdtS) {
  // try with second solver
  int sat;

  int nCl0, nCl, nV0, nV, nLearntsLoaded=0, nDisabled=0, nChecked=0;
  int nChances = 2;

  double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity
  double  var_extra_lim = var_inc / nVars();    // Remove any clause below this activity
  double var_min_act = var_extra_lim/1000;



  for (int i = 0; i < learnts.size(); i++){
    Clause& c = ca[learnts[i]];
    vec<Lit> res;
    if (c.activity() < extra_lim) continue;
    for(int j=0;j<c.size();j++) {
      int s = sign(c[j]);
      int v = var(c[j]);
      res.push(mkLit(v, s));
    }
    addClause(res);
    nLearntsLoaded++;
  }

  eliminate(false);

  printf("Solver after learnts stats: %ld/%ld vars/clauses - eliminated v/c: %d/%d\n",
	    nVars(), nClauses(),
	    eliminated_vars, eliminated_clauses);

  sat = solve(assumps);

  return sat;
}

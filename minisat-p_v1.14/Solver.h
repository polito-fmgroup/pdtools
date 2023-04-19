/****************************************************************************************[Solver.h]
MiniSat -- Copyright (c) 2003-2005, Niklas Een, Niklas Sorensson

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

#ifndef Solver_h
#define Solver_h

#include "SolverTypes.h"
#include "VarOrder.h"
#include "Proof.h"

#define reportf(format, args...) ( printf(format , ## args), fflush(stdout) )


//=================================================================================================
// Solver -- the main class:


struct SolverStats {
  int64   starts, decisions, storeddecisions, propagations, conflicts, smallconflicts;
    int64   clauses_literals, learnts_literals, max_literals, tot_literals;
SolverStats() : starts(0), decisions(0), storeddecisions(0), propagations(0), conflicts(0), 
      smallconflicts(0)
      , clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0) { }
};


struct SearchParams {
  double  var_decay, clause_decay, random_var_freq, time_limit;    // (reasonable values are: 0.95, 0.999, 0.02)    
  SearchParams(double v = 1, double c = 1, double r = 0, double t = -1) : var_decay(v), clause_decay(c), random_var_freq(r) , time_limit(t) { }
};


class Solver {
    Clause tmp;
protected:
    // Solver state:
    //
    bool                ok;               // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    bool                undef;            // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    vec<Clause*>        clauses;          // List of problem clauses.
    vec<Clause*>        learnts;          // List of learnt clauses.
    //<marco> 31/01/2013
	int                 n_bin_clauses;    // Keep track of number of binary clauses "inlined" into the watcher lists (we do this primarily to get identical behavior to the version without the binary clauses trick).
	//<marco>
    vec<ClauseId>       unit_id;          // 'unit_id[var]' is the clause ID for the unit literal 'var' or '~var' (if set at toplevel).
    double              cla_inc;          // Amount to bump next clause with.
    double              cla_decay;        // INVERSE decay factor for clause activity: stores 1/decay.

    vec<double>         activity;         // A heuristic measurement of the activity of a variable.
    double              var_inc;          // Amount to bump next variable with.
    double              var_decay;        // INVERSE decay factor for variable activity: stores 1/decay. Use negative value for static variable order.
    VarOrder            order;            // Keeps track of the decision variable order.
	
    //<GpC> 15/04/2019
    char useStoredRes;
    vec<vec<Lit> >  *storedRes;            // stored resolution clauses from previous run - used for preliminary decisions
	//<marco> 31/01/2013
    vec<vec<GClause> >  watches;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
	//<marco>
    vec<char>           corevars;         // GpC: core vars have precedence in var order (received externally)
    vec<int>            coreids;          // GpC: indexes of core vars

    vec<char>           assigns;          // The current assignments (lbool:s stored as char:s).
    vec<char>           polarity;         // The preferred polarity of each variable.
    vec<Lit>            trail;            // Assignment stack; stores all assigments made in the order they were made.
    vec<int>            trail_lim;        // Separator indices for different decision levels in 'trail[]'.
	//<marco> 31/01/2013
    vec<GClause>        reason;           // 'reason[var]' is the clause that implied the variables current value, or 'NULL' if none.
	//<marco>
    vec<int>            level;            // 'level[var]' is the decision level at which assignment was made.
    vec<int>            trail_pos;        // 'trail_pos[var]' is the variable's position in 'trail[]'. 
                                          // This supersedes 'level[]' in some sense, and 'level[]' will probably be removed in future releases.
    int                 root_level;       // Level of first proper decision.
    int                 qhead;            // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    int                 simpDB_assigns;   // Number of top-level assignments since last execution of 'simplifyDB()'.
    int64               simpDB_props;     // Remaining number of propagations that must be made before next execution of 'simplifyDB()'.

    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which is used:
    //
    vec<char>           analyze_seen;
    vec<Lit>            analyze_stack;
    vec<Lit>            analyze_toclear;
    Clause*             propagate_tmpbin;
    Clause*             analyze_tmpbin;
    vec<Lit>            addBinary_tmp;
    vec<Lit>            addTernary_tmp;

    // Main internal methods:
    //
    bool        assume           (Lit p);
    void        cancelUntil      (int level);
    void        record           (const vec<Lit>& clause);

    void        analyze          (Clause* _confl, vec<Lit>& out_learnt, int& out_btlevel); // (bt = backtrack) //<marco> 04/02/2013 (underscore) </marco>
    bool        analyze_removable(Lit p, uint min_level);                                 // (helper method for 'analyze()')
    void        analyzeFinal     (Clause* confl, bool skip_first = false);
    bool        enqueue          (Lit fact, GClause from = GClause_NULL); //<marco> 04/02/2013 </marco>
    Clause*     propagate        ();
    void        reduceDB         ();
    Lit         pickBranchLit    (const SearchParams& params);
    lbool       search           (int nof_conflicts, int nof_learnts, const SearchParams& params);
    double      progressEstimate ();

    // Activity:
    //
    void     varBumpActivity(Lit p) {
        if (var_decay < 0) return;     // (negative decay means static variable order -- don't bump)
        if ( (activity[var(p)] += var_inc) > 1e100 ) varRescaleActivity();
        order.update(var(p)); }
    void     varDecayActivity  () { if (var_decay >= 0) var_inc *= var_decay; }
    void     varRescaleActivity();
    void     claDecayActivity  () { cla_inc *= cla_decay; }
    void     claRescaleActivity();

    // Operations on clauses:
    //
    void     newClause(const vec<Lit>& ps, bool learnt = false, ClauseId id = ClauseId_NULL);
    void     claBumpActivity (Clause* c) { if ( (c->activity() += cla_inc) > 1e20 ) claRescaleActivity(); }
    void     remove          (Clause* c, bool just_dealloc = false);
    bool     locked          (const Clause* c) const { GClause r = reason[var((*c)[0])]; return !r.isLit() && r.clause() == c; } //<marco> 04/02/2013 </marco>
    bool     simplify        (Clause* c) const;

    int      decisionLevel() const { return trail_lim.size(); }
    //<GpC> 15/04/2019
    Lit      getNextFromStoredRes(int &curCl, int &curLit);

public:
    Solver() : ok               (true)
             , undef            (false)
	//<marco> 31/01/2013
	, n_bin_clauses    (0)
	//<marco>
             , cla_inc          (1)
             , cla_decay        (1)
             , var_inc          (1)
             , var_decay        (1)
             , useStoredRes     (0)
      , order            (assigns, activity, corevars, coreids)
             , qhead            (0)
             , simpDB_assigns   (0)
             , simpDB_props     (0)
             , default_params   (SearchParams(0.95, 0.999, 0.02))
             , expensive_ccmin  (true)
             , pdt_opt_neg_first(false)
             , pdt_opt_one_first(false)
	     , bin_clause_opt   (false) //<marco> 06/02/2013 </marco>
             , minisat20_opt    (false)
             , proof            (NULL)
             , verbosity        (0)
             , progress_estimate(0)
             , conflict_id      (ClauseId_NULL)
             {
                vec<Lit> dummy(2,lit_Undef);
                propagate_tmpbin = tmp.Clause_new(false, dummy);
                // printf("APT: %lx\n", (unsigned long)propagate_tmpbin);
                analyze_tmpbin   = tmp.Clause_new(false, dummy);
                addBinary_tmp .growTo(2);
                addTernary_tmp.growTo(3);
             }

   ~Solver() {
       for (int i = 0; i < learnts.size(); i++) remove(learnts[i], true);
       for (int i = 0; i < clauses.size(); i++) if (clauses[i] != NULL) { 
	   assert (clauses[i]->size() > 0);
	   remove(clauses[i], true); 
       }
       if (proof!=NULL) {
         delete proof;
       }
       //       printf("FPT: %lx\n", (unsigned long)propagate_tmpbin);
       free(propagate_tmpbin);
       free(analyze_tmpbin);
   }

    // Helpers: (semi-internal)
    //

   //   void    varBumpActivityExternal(Lit p) {return varBumpActivity(p); }
   Clause *readLearnt(int i) {
     if (i>learnts.size()) return NULL;
     else 
       return learnts[i];
   }
   int     countImplLearnts  (int max) {
     int cnt=0;
     for (int i=0; i<learnts.size(); i++) {
       if (learnts[i]->size()<=max) {
         cnt++;
       }
     }
     return cnt;
   }
   
   void     varBumpActivityExternal(Lit p, double incr) {
        if (var_decay < 0) return;     // (negative decay means static variable order -- don't bump)
        if ( (activity[var(p)] += incr) > 1e100 ) varRescaleActivity();
        order.update(var(p)); }
   int     varReadActivityExternal(Lit p) {
     return (activity[var(p)]);
   }

    lbool   value(Var x) const { return toLbool(assigns[x]); }
    lbool   value(Lit p) const { return sign(p) ? ~toLbool(assigns[var(p)]) : toLbool(assigns[var(p)]); }

    int     nAssigns() { return trail.size(); }
    int     nClauses() { return clauses.size() + n_bin_clauses;}//<marco> 31/01/2013<marco>
    int     nLearnts() { return learnts.size(); }

    // Statistics: (read-only member variable)
    //
    SolverStats     stats;

    // Mode of operation:
    //
    SearchParams    default_params;     // Restart frequency etc.
    bool            expensive_ccmin;    // Controls conflict clause minimization. TRUE by default.
    bool            pdt_opt_neg_first;  // tries complemented lit first
    bool            pdt_opt_one_first;  // tries positive lit first
    bool 	    bin_clause_opt;   //<marco> 06/02/2013 - Enables binary clauses optimization </marco>
    bool            minisat20_opt;      // Controls application of
					// minisat20 opt.
    Proof*          proof;              // Set this directly after constructing 'Solver' to enable proof logging. Initialized to NULL.
    int             verbosity;          // Verbosity level. 0=silent, 1=some progress report, 2=everything

    // Problem specification:
    //
    Var     newVar    ();
    int     nVars     ()                    { return assigns.size(); }
    void    addUnit   (Lit p)               { if (ok) ok = enqueue(p); }
    void    addBinary (Lit p, Lit q)        { addBinary_tmp [0] = p; addBinary_tmp [1] = q; addClause(addBinary_tmp); }
    void    addTernary(Lit p, Lit q, Lit r) { addTernary_tmp[0] = p; addTernary_tmp[1] = q; addTernary_tmp[2] = r; addClause(addTernary_tmp); }
    void    addClause (const vec<Lit>& ps)  { newClause(ps); }  // (used to be a difference between internal and external method...)

    // Solving:
    //
    bool    okay() { return ok; }       // FALSE means solver is in an conflicting state (must never be used again!)
    bool    undefined() { return undef; }       // FALSE means solver is in an conflicting state (must never be used again!)
    void    dump(char name[]) {vec<Lit> tmp; dump(name, 1, tmp);}
    void    dump(char name[], int all) {vec<Lit> tmp; dump(name, all, tmp);}
    void    dump(char name[], int all, vec<Lit>& assumps);
    void    simplifyDB();
    void    resetDB();
    bool    solve(const vec<Lit>& assumps, double time_limit);
    bool    solve(const vec<Lit>& assumps) {return solve(assumps,-1); }
    bool    solve() { vec<Lit> tmp; return solve(tmp,-1); }
    bool    solve(double time_limit) { vec<Lit> tmp; return solve(tmp,time_limit) ;}

    double      progress_estimate;  // Set by 'search()'.
    vec<lbool>  model;              // If problem is satisfiable, this vector contains the model (if any).
    vec<Lit>    conflict;           // If problem is unsatisfiable under assumptions, this vector represent the conflict clause expressed in the assumptions.
    ClauseId    conflict_id;        // (In proof logging mode only.) ID for the clause 'conflict' (for proof traverseral). NOTE! The empty clause is always the last clause derived, but for conflicts under assumption, this is not necessarly true.

    // Temp removal
    void    deleteProofTemps();
    int     storePreviousRes(vec<vec<Lit> >  *previousRes);
    bool    mySimplify(Clause* c);
    int     storePreviousCoreVars(vec<Var>  *prevCorevars);
};


//=================================================================================================
// Debug:


#define L_LIT    "%s%d"
#define L_lit(p) sign(p)?"-":"", var(p)+1

// Just like 'assert()' but expression will be evaluated in the release version as well.
inline void check(bool expr) { assert(expr); }


//=================================================================================================
#endif

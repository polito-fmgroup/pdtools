/****************************************************************************************[Solver.h]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRAreasNTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Minisat_Solver_h
#define Minisat_Solver_h

#include "mtl/Vec.h"
#include "mtl/Heap.h"
#include "mtl/Alg.h"
#include "utils/System.h"
#include "utils/Options.h"
#include "coreDir/SolverTypes.h"
#include "coreDir/PdtLink.h"


namespace Minisat {

class ProofVisitor;

//=================================================================================================
// Solver -- the main class:

class Solver {
public:

    // Constructor/Destructor:
    //
    Solver();
    virtual ~Solver();

    // Problem specification:
    //
    Var     newVar    (bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.

    bool    addClause (const vec<Lit>& ps);                     // Add a clause to the solver.
    bool    addEmptyClause();                                   // Add the empty clause, making the solver contradictory.
    bool    addClause (Lit p);                                  // Add a unit clause to the solver.
    bool    addClause (Lit p, Lit q);                           // Add a binary clause to the solver.
    bool    addClause (Lit p, Lit q, Lit r);                    // Add a ternary clause to the solver.
  bool addClause_ (vec<Lit> &ps);
  bool    addClause_(      vec<Lit>& ps, Range part);                     // Add a clause to the solver without making superflous internal copy. Will
                                                                // change the passed vector 'ps'.

    // Solving:
    //
    bool    simplify     ();                        // Removes already satisfied clauses.
    bool    solve        (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions.
    lbool   solveLimited (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions (With resource constraints).
    int   solveLimitedInt (const vec<Lit>& assumps); // Search for a model that respects a given set of assumptions (With resource constraints).
    bool    solve        ();                        // Search without assumptions.
    bool    solve        (Lit p);                   // Search for a model that respects a single assumption.
    bool    solve        (Lit p, Lit q);            // Search for a model that respects two assumptions.
    bool    solve        (Lit p, Lit q, Lit r);     // Search for a model that respects three assumptions.
    bool    okay         () const;                  // FALSE means solver is in a conflicting state
    void    toDimacs     (FILE* f, const vec<Lit>& assumps);            // Write CNF to file in DIMACS-format.
    void    toDimacs     (const char *file, const vec<Lit>& assumps);
    void    toDimacs     (FILE* f, Clause& c, vec<Var>& map, Var& max);

    // Convenience versions of 'toDimacs()':
    void    toDimacs     (const char* file);
    void    toDimacs     (const char* file, Lit p);
    void    toDimacs     (const char* file, Lit p, Lit q);
    void    toDimacs     (const char* file, Lit p, Lit q, Lit r);

    // Proof validation / traversal
    inline bool isAttached(CRef r) {
      Clause &rc = ca [r];
      return find(watches[~rc[0]], Watcher(r, rc[1]));}
    void    reInitClause(CRef r);
    bool    validate (ProofVisitor& v, bool coreFirst=false);  // validates clausal proof
    bool    replay (ProofVisitor& v); // replays clausal proof AFTER validation

    bool    proofLogging () { return log_proof;}
    void    proofLogging (bool v) { log_proof = v;}
    bool    orderedPropagate () { return ordered_propagate; }
    void    orderedPropagate (bool v) { ordered_propagate = v; }


    void  dumpClause(int id, CRef cr);
    void  dumpCNF();

    int start;
    void labelLevel0(ProofVisitor& v);
    bool traverseProof(ProofVisitor& v, CRef proofClause, CRef confl);
    void labelFinal(ProofVisitor& v, CRef confl);

    // Variable mode:
    //
    void    setPolarity    (Var v, bool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
    void    setDecisionVar (Var v, bool b); // Declare if a variable should be eligible for selection in the decision heuristic.

    // Read state:
    //
    lbool   value      (Var x) const;       // The current value of a variable.
    lbool   value      (Lit p) const;       // The current value of a literal.
    lbool   modelValue (Var x) const;       // The value of a variable in the last model. The last call to solve must have been satisfiable.
    lbool   modelValue (Lit p) const;       // The value of a literal in the last model. The last call to solve must have been satisfiable.
    int     nAssigns   ()      const;       // The current number of assigned literals.
    int     nClauses   ()      const;       // The current number of original clauses.
    int     nLearnts   ()      const;       // The current number of learnt clauses.
    int     nVars      ()      const;       // The current number of variables.
    int     nFreeVars  ()      const;

    // Resource contraints:
    //
    void    setConfBudget(int64_t x);
    void    setPropBudget(int64_t x);
    void    setTimeBudget(double x);
    void    budgetOff();
    void    interrupt();          // Trigger a (potentially asynchronous) interruption of the solver.
    void    clearInterrupt();     // Clear interrupt indicator flag.

    // Memory managment:
    //
    virtual void garbageCollect();
    void    checkGarbage(double gf);
    void    checkGarbage();

    // Extra results: (read-only member variable)
    //
    vec<lbool> model;             // If problem is satisfiable, this vector contains the model (if any).
    vec<Lit>   conflict;          // If problem is unsatisfiable (possibly under assumptions),
                                  // this vector represent the final conflict clause expressed in the assumptions.


    vec<bool> globals;
    vec<int>  chainLevels;
    vec<int>  maxPivots;
    vec<uint64_t> varDecisions;
  
    // Mode of operation:
    //
    int       verbosity;
    bool      log_proof; // Enable proof logging
    bool      ordered_propagate;
    double    var_decay;
    double    clause_decay;
    double    random_var_freq;
    double    random_seed;
    bool      luby_restart;
    int       ccmin_mode;         // Controls conflict clause minimization (0=none, 1=basic, 2=deep).
    int       phase_saving;       // Controls the level of phase saving (0=none, 1=limited, 2=full).
    bool      rnd_pol;            // Use random polarities for branching heuristics.
    bool      rnd_init_act;       // Initialize variable activities with a small random value.
    double    garbage_frac;       // The fraction of wasted memory allowed before a garbage collection is triggered.

    int       restart_first;      // The initial restart limit.                                                                (default 100)
    double    restart_inc;        // The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
    double    learntsize_factor;  // The intitial limit for learnt clauses is a factor of the original clauses.                (default 1 / 3)
    double    learntsize_inc;     // The limit for learnt clauses is multiplied with this factor each restart.                 (default 1.1)

    int       learntsize_adjust_start_confl;
    double    learntsize_adjust_inc;

    // Statistics: (read-only member variable)
    //
    uint64_t solves, starts, decisions, rnd_decisions, propagations, conflicts;
    uint64_t dec_vars, clauses_literals, learnts_literals, max_literals, tot_literals;

    void setCurrentPart(unsigned n)     { currentPart = n; }
    unsigned getCurrentPart ()          { return currentPart; }
    Range    getTotalPart ()            { return totalPart; }


  Range part (Var v) const   { assert(partInfo.size() > v); return partInfo[v]; }
  Range getVarRange(Var v) const { return part (v); }
  Range getClsRange(CRef cls) const   { assert(cls != CRef_Undef); return ca[cls].part(); }
  CRef getClauseCRef(int i) const    {return clauses[i];}
  const Clause& getClause(CRef cr) const    {return ca[cr];}
  const Clause& getClause2(CRef cr) const    {return ca[clauses[cr]];}
  const Lit& getAssign(int i) const    {return trail[i];}
  CRef getReason(Var x) const      {return reason(x);}
  CRef getLevel(Var x) const      {return level(x);}
  void printTrail(void);

  

  int maxPivot(int i){
    if (i>=maxPivots.size()) return -1;
    return maxPivots[i];
  }

  void enableVarDecisions(){
    //    varDecisions.clear();
    varDecisions.growTo(nVars());
  }
  void topVarDecisions(vec<Var>& topv, vec<int>& topd, int *cnf2aig, int n, int mind) {
    topv.clear();
    topd.clear();
    while(n-->0) {
      Var vMax = 0; 
      for (Var v=1; v<nVars(); v++) {
        if (varDecisions[v]>varDecisions[vMax])
          vMax = v;
      }
      if (varDecisions[vMax]>mind) {
        if (cnf2aig!=NULL && cnf2aig[vMax]<=2) {
          n++; // don't count it
        }
        topv.push(vMax);
        topd.push(varDecisions[vMax]);
        varDecisions[vMax] = 0;
      }
    }
    for (int i=0; i<topv.size(); i++) {
      varDecisions[topv[i]] = topd[i];
    }
  }
  
  void topVarActivity(vec<Var>& topv, vec<double>& topa, int *cnf2aig, int n, double mina) {
    topv.clear();
    topa.clear();
    while(n-->0) {
      Var vMax = 0; 
      for (Var v=1; v<nVars(); v++) {
        if (activity[v]>activity[vMax])
          vMax = v;
      }
      if (activity[vMax]>mina) {
        if (cnf2aig!=NULL && cnf2aig[vMax]<=2) {
          n++; // don't count it
        }
        topv.push(vMax);
        topa.push(activity[vMax]);
        activity[vMax] = 0.0;
      }
    }
    for (int i=0; i<topv.size(); i++) {
      activity[topv[i]] = topa[i];
    }
  }

  void topFilteredVarActivity(vec<Var>& topv, vec<double>& topa, vec<bool>& active, int n) {
    topv.clear();
    topa.clear();
    while(n-->0) {
      Var vMax = 0; 
      for (Var v=1; v<nVars(); v++) {
        if (active[v] && activity[v]>activity[vMax])
          vMax = v;
      }
      topv.push(vMax);
      topa.push(activity[vMax]);
      activity[vMax] = 0.0;
    }
    for (int i=0; i<topv.size(); i++) {
      activity[topv[i]] = topa[i];
    }
  }

  void initGlobals(){
    globals.growTo(nVars(), false);
  }

  void setGlobal(Var v){
    globals[v] = true;
  }

  bool isGlobal(Lit lit){
    return globals[var(lit)] == true;
  }


  void clearGlobals(){
    globals.shrink(0);
  }

  void updateChainLevels(vec<Lit>& clause, int level);
  void restructureProof();
  void countPivots();
  void dumpGlobalPivots(int i);
  void checkChainSoundness(int i);

  void getFilteredLearnts(vec<CRef> &undefTopClauses,
			  vec<Var>& filterVars, vec<Var>& filter2Vars) {
    int useABvars = 0;
    int maxLits = 20;
    vec<int> selectVar;
    selectVar.growTo(nVars(), 0);
    for(int i = 0; i<filter2Vars.size(); i++){
      Var v = filter2Vars[i];
      assert(v>=0&&v<nVars());
      selectVar[v] = 1;
    }
    for(int i = 0; i<filterVars.size(); i++){
      Var v = filterVars[i];
      assert(v>=0&&v<nVars());
      selectVar[v] = 2;
    }
    int k=0;
    for(int i = 0; i<learnts.size(); i++){
      Clause& cl = ca[learnts[i]];
      int selected = 0, selectedPlus = 0, notSelected=0;
      if (maxLits<=0 || cl.size()<=maxLits) {
	for(int j=0; j<cl.size(); ++j) {
	  Var v = var(cl[j]);
	  if (!selectVar[v]) {
	    notSelected++;
	  }
	  else if (selectVar[v]==1) {
	    selected++;
	  }
	  else if (selectVar[v]==2) {
	    selectedPlus++;
	  }
	}
	if (selectedPlus>=1 && (selected==0 || notSelected==0)) {
	  undefTopClauses.push(learnts[i]);
	  //	printf("Top cl: %d - size: %d\n",learnts[i], cl.size());
	}
      }
    }
 
  }

  void getLearntsTopLits(vec<Lit>& topLits, int maxLits=20, int balanced=1, int doPrint=1) {
    int cnt[10]={0}, cntFull[10]={0}, a=0, b=0, g=0;
    vec<int> litPos;
    vec<int> litNeg;
    vec<int> clSizeNeg;
    vec<int> clSizePos;
    vec<int> cntTot;
    vec<Lit> sorted;
    int maxCnt=0, maxSize=0;
    litPos.growTo(nVars(), 0);
    litNeg.growTo(nVars(), 0);
    clSizePos.growTo(nVars(), 0);
    clSizeNeg.growTo(nVars(), 0);
    sorted.growTo(2*nVars(), lit_Undef);
    for(int i = 0; i<learnts.size(); i++){
      Clause& cl = ca[learnts[i]];
      if (cl.size() > maxSize) maxSize = cl.size();
      for(int j=0; j<cl.size(); ++j) {
        Var v = var(cl[j]);
        if (sign(cl[j])) {
          litNeg[v]++;
          clSizeNeg[v] += cl.size();
        }
        else {
          litPos[v]++;
          clSizePos[v] += cl.size();
        }
      }
    }
    for(int v = 0; v<nVars(); v++){
      if (litPos[v]>0) {
        clSizePos[v] /= litPos[v];
        if (litPos[v] > maxCnt) maxCnt = litPos[v];
      }
      if (litNeg[v]>0) {
        clSizeNeg[v] /= litNeg[v];
        if (litNeg[v] > maxCnt) maxCnt = litNeg[v];
      }
    }
    cntTot.growTo(maxCnt+1, 0);
    for(int v = 0; v<nVars(); v++){
      cntTot[litPos[v]]++;
      cntTot[litNeg[v]]++;
    }
    for(int i = 1; i<=maxCnt; i++){
      cntTot[i] += cntTot[i-1];
    }
    for (int v=nVars()-1; v>=0; v--) {
      Lit p = mkLit(v, 0);
      int j=--cntTot[litPos[v]];
      assert(j>=0 && j<2*nVars());
      sorted[j] = p;
      p = mkLit(v, 1);
      j=--cntTot[litNeg[v]];
      assert(j>=0 && j<2*nVars());
      sorted[j] = p;
    }
    if (doPrint)
      printf("learnts stats for literals in %d learnt clauses - max size %d\n",
             learnts.size(), maxSize);
    vec<int> countLits;
    countLits.growTo(nVars(),0);
    int cntTh = maxCnt/10;
    for (int i=0; i<2*nVars(); i++) {
      int j = 2*nVars()-i-1;
      Lit p = sorted[j];
      Var v = var(p);
      countLits[v]++;
      if (balanced) {
        if (sign(p) && litPos[v]>litNeg[v]/4) countLits[v]++;
        if (!sign(p) && litNeg[v]>litPos[v]/4) countLits[v]++;
        if (litNeg[v] < cntTh) break;
      }
      else {
        if (sign(p) && litPos[v]>10) countLits[v]++;
        if (!sign(p) && litNeg[v]>10) countLits[v]++;
        if (litPos[v] < cntTh) break;
      }
    }
    topLits.clear();
    int notActiveCnt=0;
    for (int i=0; i<2*nVars() && topLits.size()<maxLits; i++) {
      int j = 2*nVars()-i-1;
      Lit p = sorted[j];
      Var v = var(p);
      int select = (countLits[v] > 0) && (balanced ^ countLits[v]<2); 
      if (!select) {
        if (doPrint)
          printf("skipping var %d with two lits selected\n", v);
        if (++notActiveCnt > 20) break;
        continue;
      }
      notActiveCnt = 0;
      countLits[v] = 0;
      char s = sign(p)?'-':' ';
      int cnt = sign(p)?litNeg[v]:litPos[v]; 
      int sz = sign(p)?clSizeNeg[v]:clSizePos[v]; 
      int cnt1 = sign(p)?litPos[v]:litNeg[v]; 
      int sz1 = sign(p)?clSizePos[v]:clSizeNeg[v]; 
      if (doPrint)
        printf("lit: %c%d - count: %d - avg cl size: %d (compl: %d %d)\n", s, v, cnt, sz, cnt1, sz1);
      topLits.push(p);  
    }
  }

  void printLearntsStats(vec<Var>& filterVars, vec<Var>& filter2Vars) {
    int useABvars = 0;
    int maxLits = 20;
    int cnt[10]={0}, cntFull[10]={0}, a=0, b=0, g=0;
    vec<int> selectVar;
    selectVar.growTo(nVars(), 0);
    for(int i = 0; i<filter2Vars.size(); i++){
      Var v = filter2Vars[i];
      assert(v>=0&&v<nVars());
      selectVar[v] = 1;
    }
    for(int i = 0; i<filterVars.size(); i++){
      Var v = filterVars[i];
      assert(v>=0&&v<nVars());
      selectVar[v] = 2;
    }
    int k=0;
    for(int i = 0; i<learnts.size(); i++){
      Clause& cl = ca[learnts[i]];
      int selected = 0, selectedPlus = 0, notSelected=0;
      if (maxLits<=0 || cl.size()<=maxLits) {
	for(int j=0; j<cl.size(); ++j) {
	  Var v = var(cl[j]);
	  if (!selectVar[v]) {
	    notSelected++;
	  }
	  else if (selectVar[v]==1) {
	    selected++;
	  }
	  else if (selectVar[v]==2) {
	    selectedPlus++;
	  }
	}
	if (selected==0 && notSelected>0) {
	  a++;
	}
	else if (selected>0 && notSelected==0) {
	  b++;
	}
	else if (selected==0 && notSelected==0) {
	  g++;
	}
	else {
	  int j = cl.size()/10;
	  if (j>9) j=9;
	  cntFull[j]++;
	}
	int i = cl.size()/10;
	if (i>9) i=9;
	cnt[i]++;
      }
    }

    printf("learnts stats - a: %d, b: %d, g: %d\n", a, b, g);
    printf("split cnt: \n");
    for (int i=0; i<10; i++) printf("[%d] %d ", i*10, cnt[i]); printf("\n");
    printf("split cnt full: ");
    for (int i=0; i<10; i++) printf("[%d] %d ", i*10, cntFull[i]); printf("\n");
  }

  //N.B: call only after validate
  void printCore(void) {
    vec<bool> isCoreVar;
    isCoreVar.growTo(nVars(), false);
    for(int i = 0; i<clauses.size(); i++){
      const Clause& cl = getClause(clauses[i]);
        if(cl.core()){
	  fprintf (stdout, "CORE[%d] ", i);
	  for (int j = 0; j < cl.size (); ++j)
	    {
	      fprintf (stdout, "%s%d", sign (cl[j]) ? "-" : "",
		       var (cl[j]) + 1);
	      fprintf (stdout, " ");
	    }
	  fprintf (stdout, " 0\n");
        }
    }
  }

  void getCore(vec< vec<Lit> >& coreClauses, vec<Var>& coreVars) {
    vec<int> isCoreVar;
    isCoreVar.growTo(nVars(), 0);
    int k=0;
    for(int i = 0; i<clauses.size(); i++){
        Clause& cl = ca[clauses[i]];
        if(cl.core()){
            coreClauses.push();
            for(int j=0; j<cl.size(); ++j){
                coreClauses[k].push(cl[j]);
                Var v = var(cl[j]);
		if (sign(cl[j])) {
		  // negative lit
		  isCoreVar[v] |= 0x1;
		}
		else {
		  // positive lit
		  isCoreVar[v] |= 0x2;
		}
            }
	    k++;
        }
    }
    for(int i = 0; i<isCoreVar.size(); i++){
      int ii = isCoreVar[i];
      if (ii!=0) {
	coreVars.push((i<<2)+ii);
      }
    }

  }

  //N.B: call only after validate
  void getSignedCore(vec< vec<Lit> >& coreClauses, vec<Var>& coreVars) {
    vec<int> isCoreVar;
    isCoreVar.growTo(nVars(), 0);
    for(int i = 0; i<clauses.size(); i++){
        Clause& cl = ca[proof[i]];
        if(cl.core()){
            coreClauses.push();
            for(int j=0; j<cl.size(); ++j){
                coreClauses[i].push(cl[j]);
                Var v = var(cl[j]);
                if(sign(cl[j])){
                    isCoreVar[v] |= 0x1;
                } else {
                    isCoreVar[v] |= 0x2;
                }
            }
        }
    }
    for(int v = 0; v<nVars(); v++){
        if(isCoreVar[v]){
          coreVars.push((v<<2)+(isCoreVar[v]));
        }
    }
  }

  //NB: call only after replay
  int getProof(vec< vec<Lit> >& Clauses,
	       int& nAClausesCore,
	       vec< vec<int> >& proofNodes,
	       vec< vec<Lit> >& pivots,
	       vec< vec<Lit> > *topResClP
	       );
  
  int getProofClauses(vec< vec<Lit> >& Clauses);
  int checkSolverProofClauses(void);
  int getSolverProofClauses(vec< vec<Lit> >& Clauses,
                                bool coreOnly);
  
  void getPartialProof(vec< vec<Lit> >& Clauses,
                int& nSolverAClauses, int& nAClausesCore,
                vec< vec<int> >& proofNodes,
                vec< vec<Lit> >& pivots,
                vec<bool> *isAvar, vec<bool> *isGlobal,
                float extraRatio);

  void remapProofVars(vec<int>& remap);
  void remapClauses(vec<int>& remap, vec< vec<Lit> >& Clauses);
                            

  void printProofNode(int i, int verbosity);
  void printProof(int verbosity);
  void printSolverProof(int verbosity);
  int proofNodeFindAntecedentsWithVar(int i, int v, int verbosity);
  int proofNodeCheckAntecedents(int i, vec<lbool>& mark, int verbosity);
  int proofCompactResolvents(void);
  int proofRecyclePivotsReduction(void);

  void proofCheck(void);
  void proofSave(void);

  void getProof1(vec< vec<Lit> >& clauses,
                int& nAClauses, int& nAClausesCore, 
                vec< vec<int> >& proofNodes,
                vec< vec<Lit> >& pivots,
                vec< vec<Lit> >& topResClauses,
                int partial, Solver *auxS);

  int copyAllClausesToSolverIfHaveNoVars (Solver *S,
					  vec<bool>& noVars);

  void getPartialNewProof(vec< vec<Lit> >& Clauses,
                               int& nAClauses, int& nAClausesCore,
                               int& nASolverClausesCore,
                               Solver *auxS, 
                               float extraRatio);

  void getProofClausesAfterMove(vec< vec<Lit> >& ClA,
                                vec< vec<Lit> >& ClB,
                                vec< vec<Lit> >& ClAorig,
                                vec< vec<Lit> >& ClAfromB,
                                vec< vec<Lit> >& ClAfromRes);
  void genProofAuxClausesFromBres(vec< vec<Lit> >& ClBaux,
                                  vec<Lit>& actLits,
                                  vec<Lit>& noActClause,
                                  float minSizeRatio
                                  );
  void genProofActLitsFromBres(vec<Lit>& actLits, int nLits);

  void proofClassifyNodes(int nAClauses, bool enSimplify);
  void proofEndAClauses(void);
  void proofCompactNodes(void);
  int proofMoveNodesToA(float extraRatio);
  void proofSetupProofVars(void);
  void proofSetupVars(void);
  void proofReverseAB(void);
  int proofSize(void){
    return(proof.size());
  }

  void proofRestart(void){
    proof.clear();
  }

  void pdtAddClauses(vec< vec<Lit> >& clauses) {
    for (int i=0; i<clauses.size(); i++) {
      vec<Lit>& c = clauses[i];
      for (int i = 0; i < c.size (); ++i) {
        Var v = var(c[i]);
        while (v>=nVars()) newVar();
      }
      vec<Lit> cNew;
      c.copyTo(cNew);
      addClause(cNew);
    }
  }
    
  /// -- a clause is shared if all of its variables appear in clauses
  /// -- with a higher partition
  bool shared (CRef cr)
  {
    Clause &c = ca[cr];
    for (int i = 0; i < c.size (); ++i)
      if (part (var (c[i])).max () <= c.part ().max ()) return false;
    return true;
  }

  void    varBumpActivityExternal  (Var v, double inc) {
    varBumpActivity  (v, inc); 
  } 


  // PDT proof to work one
  ProofPdt proofPdt;

protected:

    // Helper structures:
    //
    struct VarData { CRef reason; int level; };
    static inline VarData mkVarData(CRef cr, int l){ VarData d = {cr, l}; return d; }

    struct Watcher {
        CRef cref;
        Lit  blocker;
        Watcher () : cref(CRef_Undef), blocker(lit_Undef) {}
        Watcher(CRef cr, Lit p) : cref(cr), blocker(p) {}
        bool operator==(const Watcher& w) const { return cref == w.cref; }
        bool operator!=(const Watcher& w) const { return cref != w.cref; }
    };

    struct WatcherDeleted
    {
        const ClauseAllocator& ca;
        WatcherDeleted(const ClauseAllocator& _ca) : ca(_ca) {}
        bool operator()(const Watcher& w) const { return ca[w.cref].mark() == 1; }
    };

    struct WatcherLt
    {
      const ClauseAllocator& ca;
      WatcherLt (const ClauseAllocator& _ca) : ca(_ca) {}
      bool operator () (Watcher const &x, Watcher const &y)
      { return x.cref > y.cref; }
    };

    struct VarOrderLt {
        const vec<double>&  activity;
        bool operator () (Var x, Var y) const { return activity[x] > activity[y]; }
        VarOrderLt(const vec<double>&  act) : activity(act) { }
    };

    // Solver state:
    //
    bool                ok;               // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
    vec<CRef>           clauses;          // List of problem clauses.
    vec<CRef>           learnts;          // List of learnt clauses.
    vec<CRef>           proof;            // Clausal proof
    vec<Range>          trail_part;       // Partition of variables on the trail
    double              cla_inc;          // Amount to bump next clause with.
    vec<double>         activity;         // A heuristic measurement of the activity of a variable.
    double              var_inc;          // Amount to bump next variable with.
    OccLists<Lit, vec<Watcher>, WatcherDeleted>
                        watches;          // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    vec<lbool>          assigns;          // The current assignments.
    vec<char>           polarity;         // The preferred polarity of each variable.
    vec<char>           decision;         // Declares if a variable is eligible for selection in the decision heuristic.
    vec<Lit>            trail;            // Assignment stack; stores all assigments made in the order they were made.
    vec<int>            trail_lim;        // Separator indices for different decision levels in 'trail'.
    vec<VarData>        vardata;          // Stores reason and level for each variable.
    int                 qhead;            // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    int                 simpDB_assigns;   // Number of top-level assignments since last execution of 'simplify()'.
    int64_t             simpDB_props;     // Remaining number of propagations that must be made before next execution of 'simplify()'.
    vec<Lit>            assumptions;      // Current set of assumptions provided to solve by the user.
    Heap<VarOrderLt>    order_heap;       // A priority queue of variables ordered with respect to the variable activity.
    double              progress_estimate;// Set by 'search()'.
    bool                remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.

    ClauseAllocator     ca;

    // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
    // used, exept 'seen' wich is used in several places.
    //
    vec<char>           seen;
    vec<Lit>            analyze_stack;
    vec<Lit>            analyze_toclear;
    vec<Lit>            add_tmp;

    double              max_learnts;
    double              learntsize_adjust_confl;
    int                 learntsize_adjust_cnt;

    // Resource contraints:
    //
    int64_t             conflict_budget;    // -1 means no budget.
    int64_t             propagation_budget; // -1 means no budget.
    double              time_budget;        // <0 meand no budget

    bool                asynch_interrupt;

    // Interpolation related data structures
    vec<Range> partInfo;
    unsigned currentPart;
    // Range that includes all partitions of clauses in the database
    Range  totalPart;

    // Main internal methods:
    //
    void     insertVarOrder   (Var x);                                                 // Insert a variable in the decision order priority queue.
    Lit      pickBranchLit    ();                                                      // Return the next decision variable.
    void     newDecisionLevel ();                                                      // Begins a new decision level.
    void     uncheckedEnqueue (Lit p, CRef from = CRef_Undef);                         // Enqueue a literal. Assumes value of literal is undefined.
    bool     enqueue          (Lit p, CRef from = CRef_Undef);                         // Test if fact 'p' contradicts current state, enqueue otherwise.
    CRef     propagate        (bool coreOnly = false, int maxPart = 0, bool coreFirst = false);                                                      // Perform unit propagation. Returns possibly conflicting clause.
    void     cancelUntil      (int level);                                             // Backtrack until a certain level.
    void     analyze          (CRef confl, vec<Lit>& out_learnt, int& out_btlevel,
                               Range &part);    // (bt = backtrack)
    void     analyzeFinal     (Lit p, vec<Lit>& out_conflict);                         // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
  bool     litRedundant     (Lit p, uint32_t abstract_levels, Range &part);                       // (helper method for 'analyze()')
    lbool    search           (int nof_conflicts);                                     // Search for a given number of conflicts.
    lbool    solve_           ();                                                      // Main solve method (assumptions given in 'assumptions').
    void     reduceDB         ();                                                      // Reduce the set of learnt clauses.
    void     removeSatisfied  (vec<CRef>& cs);                                         // Shrink 'cs' to contain only non-satisfied clauses.
    void     rebuildOrderHeap ();

    bool     validateLemma (CRef c, bool coreFirst);
    // Maintaining Variable/Clause activity:
    //
    void     varDecayActivity ();                      // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    void     varBumpActivity  (Var v, double inc);     // Increase a variable with the current 'bump' value.
    void     varBumpActivity  (Var v);                 // Increase a variable with the current 'bump' value.
    void     claDecayActivity ();                      // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.
    void     claBumpActivity  (Clause& c);             // Increase a clause with the current 'bump' value.

    // Operations on clauses:
    //
    void     attachClause     (CRef cr);               // Attach a clause to watcher lists.
    void     detachClause     (CRef cr, bool strict = false); // Detach a clause to watcher lists.
    void     removeClause     (CRef cr);               // Detach and free a clause.
    bool     locked           (const Clause& c) const; // Returns TRUE if a clause is a reason for some implication in the current state.
    bool     satisfied        (const Clause& c) const; // Returns TRUE if a clause is satisfied in the current state.

    void     relocAll         (ClauseAllocator& to);

    // Misc:
    //
    int      decisionLevel    ()      const; // Gives the current decisionlevel.
    uint32_t abstractLevel    (Var x) const; // Used to represent an abstraction of sets of decision levels.
    CRef     reason           (Var x) const;
    int      level            (Var x) const;
    double   progressEstimate ()      const; // DELETE THIS ?? IT'S NOT VERY USEFUL ...
    bool     withinBudget     ()      const;

    // Static helpers:
    //

    // Returns a random float 0 <= x < 1. Seed must never be 0.
    static inline double drand(double& seed) {
        seed *= 1389796;
        int q = (int)(seed / 2147483647);
        seed -= (double)q * 2147483647;
        return seed / 2147483647; }

    // Returns a random integer 0 <= x < size. Seed must never be 0.
    static inline int irand(double& seed, int size) {
        return (int)(drand(seed) * size); }

};


//=================================================================================================
// Implementation of inline methods:

inline CRef Solver::reason(Var x) const { return vardata[x].reason; }
inline int  Solver::level (Var x) const { return vardata[x].level; }

inline void Solver::insertVarOrder(Var x) {
    if (!order_heap.inHeap(x) && decision[x]) order_heap.insert(x); }

inline void Solver::varDecayActivity() { var_inc *= (1 / var_decay); }
inline void Solver::varBumpActivity(Var v) { varBumpActivity(v, var_inc); }
inline void Solver::varBumpActivity(Var v, double inc) {
    if ( (activity[v] += inc) > 1e100 ) {
        // Rescale:
        for (int i = 0; i < nVars(); i++)
            activity[i] *= 1e-100;
        var_inc *= 1e-100; }

    // Update order_heap with respect to new activity:
    if (order_heap.inHeap(v))
        order_heap.decrease(v); }

inline void Solver::claDecayActivity() { cla_inc *= (1 / clause_decay); }
inline void Solver::claBumpActivity (Clause& c) {
        if ( (c.activity() += cla_inc) > 1e20 ) {
            // Rescale:
            for (int i = 0; i < learnts.size(); i++)
                ca[learnts[i]].activity() *= 1e-20;
            cla_inc *= 1e-20; } }

inline void Solver::checkGarbage(void){ return checkGarbage(garbage_frac); }
inline void Solver::checkGarbage(double gf){
    if (ca.wasted() > ca.size() * gf)
        garbageCollect(); }

// NOTE: enqueue does not set the ok flag! (only public methods do)
inline bool     Solver::enqueue         (Lit p, CRef from)      { return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true); }
inline bool     Solver::addClause       (const vec<Lit>& ps)    { ps.copyTo(add_tmp); return addClause_(add_tmp); }
  inline bool     Solver::addClause_      (vec<Lit>&ps)           { return addClause_ (ps, Range (currentPart)); }
inline bool     Solver::addEmptyClause  ()                      { add_tmp.clear(); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p)                 { add_tmp.clear(); add_tmp.push(p); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p, Lit q)          { add_tmp.clear(); add_tmp.push(p); add_tmp.push(q); return addClause_(add_tmp); }
inline bool     Solver::addClause       (Lit p, Lit q, Lit r)   { add_tmp.clear(); add_tmp.push(p); add_tmp.push(q); add_tmp.push(r); return addClause_(add_tmp); }
inline bool     Solver::locked          (const Clause& c) const { return value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && ca.lea(reason(var(c[0]))) == &c; }
inline void     Solver::newDecisionLevel()                      { trail_lim.push(trail.size()); }

inline int      Solver::decisionLevel ()      const   { return trail_lim.size(); }
inline uint32_t Solver::abstractLevel (Var x) const   { return 1 << (level(x) & 31); }
inline lbool    Solver::value         (Var x) const   { return assigns[x]; }
inline lbool    Solver::value         (Lit p) const   { return assigns[var(p)] ^ sign(p); }
inline lbool    Solver::modelValue    (Var x) const   { return model[x]; }
inline lbool    Solver::modelValue    (Lit p) const   { return model[var(p)] ^ sign(p); }
inline int      Solver::nAssigns      ()      const   { return trail.size(); }
inline int      Solver::nClauses      ()      const   { return clauses.size(); }
inline int      Solver::nLearnts      ()      const   { return learnts.size(); }
inline int      Solver::nVars         ()      const   { return vardata.size(); }
inline int      Solver::nFreeVars     ()      const   { return (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]); }
inline void     Solver::setPolarity   (Var v, bool b) { polarity[v] = b; }
inline void     Solver::setDecisionVar(Var v, bool b)
{
    if      ( b && !decision[v]) dec_vars++;
    else if (!b &&  decision[v]) dec_vars--;

    decision[v] = b;
    insertVarOrder(v);
}
inline void     Solver::setConfBudget(int64_t x){ conflict_budget    = conflicts    + x; }
inline void     Solver::setPropBudget(int64_t x){ propagation_budget = propagations + x; }
inline void     Solver::setTimeBudget(double x){ time_budget = cpuTimeThread() + x; }
inline void     Solver::interrupt(){ asynch_interrupt = true; }
inline void     Solver::clearInterrupt(){ asynch_interrupt = false; }
 inline void    Solver::budgetOff(){ conflict_budget = propagation_budget = -1; time_budget = -1.0;}
inline bool     Solver::withinBudget() const {
    return !asynch_interrupt &&
           (conflict_budget    < 0 || conflicts < (uint64_t)conflict_budget) &&
           (propagation_budget < 0 || propagations < (uint64_t)propagation_budget) &&
      (time_budget    < 0.0 || cpuTimeThread() < time_budget); }

// FIXME: after the introduction of asynchronous interrruptions the solve-versions that return a
// pure bool do not give a safe interface. Either interrupts must be possible to turn off here, or
// all calls to solve must return an 'lbool'. I'm not yet sure which I prefer.
inline bool     Solver::solve         ()                    { budgetOff(); assumptions.clear(); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p)               { budgetOff(); assumptions.clear(); assumptions.push(p); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p, Lit q)        { budgetOff(); assumptions.clear(); assumptions.push(p); assumptions.push(q); return solve_() == l_True; }
inline bool     Solver::solve         (Lit p, Lit q, Lit r) { budgetOff(); assumptions.clear(); assumptions.push(p); assumptions.push(q); assumptions.push(r); return solve_() == l_True; }
inline bool     Solver::solve         (const vec<Lit>& assumps){ budgetOff(); assumps.copyTo(assumptions); return solve_() == l_True; }
inline lbool    Solver::solveLimited  (const vec<Lit>& assumps){ assumps.copyTo(assumptions); return solve_(); }
inline int    Solver::solveLimitedInt  (const vec<Lit>& assumps){ lbool ret = solveLimited(assumps); return ret==l_Undef ? -1 : (ret==l_True ? 1 : 0); }
inline bool     Solver::okay          ()      const   { return ok; }
inline void     Solver::toDimacs     (const char* file){ vec<Lit> as; toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p){ vec<Lit> as; as.push(p); toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p, Lit q){ vec<Lit> as; as.push(p); as.push(q); toDimacs(file, as); }
inline void     Solver::toDimacs     (const char* file, Lit p, Lit q, Lit r){ vec<Lit> as; as.push(p); as.push(q); as.push(r); toDimacs(file, as); }

//=================================================================================================
// Debug etc:


//=================================================================================================
}

#endif

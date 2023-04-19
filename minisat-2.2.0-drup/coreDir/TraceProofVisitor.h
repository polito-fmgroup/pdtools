#ifndef _TRACE_PROOF_VISITOR_H_
#define _TRACE_PROOF_VISITOR_H_

#include "ProofVisitor.h"
#include "Solver.h"

#include <cstdio>

namespace Minisat
{
 class TraceProofVisitor : public ProofVisitor
 {
 protected:
   Solver &m_Solver;       //Link to solver
   CMap<int> m_visited;    //Map of visited ids

   vec<int> m_units;       //IDs of unit clauses
   int m_ids;              //ID Counter
   FILE *m_out;

   void doAntecendents ();

 public:

   TraceProofVisitor (Solver &solver, FILE* out);

   int visitResolvent (Lit parent, Lit p1, CRef p2);
   int visitChainResolvent (Lit parent);
   int visitChainResolvent (CRef parent);
   int genUndefTopClauses (vec<Var>& filterVars, vec<Var>& filter2Vars);
   void printUndefTopClauses ();
   int isUndefTopClause (CRef id);
   int nTopClauses();
 };

  inline int      TraceProofVisitor::nTopClauses      ()      { return undefTopClauses.size(); }

  inline int TraceProofVisitor::isUndefTopClause (CRef id)
  {
    for (int i=0; i<undefTopClauses.size(); i++)
      if (undefTopClauses[i]==id) return i;
    return -1;
  }


}

#endif

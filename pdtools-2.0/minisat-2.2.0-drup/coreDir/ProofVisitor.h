/*
 * ProofVisitor.h
 *
 * Proof traversal functionality.
 *
 *  Created on: Jan 4, 2014
 *      Author: yakir
 */

#ifndef PROOF_VISITOR_H_
#define PROOF_VISITOR_H_

#include "SolverTypes.h"

namespace Minisat {

  class ProofVisitor
  {
  public:
    ProofVisitor() {}
    virtual ~ProofVisitor () {}

    virtual int visitResolvent      (Lit parent, Lit p1, CRef p2) { return 0; }
    virtual int visitChainResolvent (Lit parent)                  { return 0; }
    virtual int visitChainResolvent (CRef parent)                 { return 0; }

    vec<Lit>        chainPivots;
    vec<CRef>       chainClauses;
    bool doUndef;
    vec<CRef> undefTopClauses;
  };
}

#endif /* PROOF_VISITOR_H_ */

/*
 * PdtLink.h
 *
 *  Created on: Jun 3, 2015
 *      Author: Pale, Pas
 */

#ifndef PDTLINK_H_
#define PDTLINK_H_

#include "SolverTypes.h"

namespace Minisat {

  typedef enum {proof_root, proof_rootA, proof_rootB,
                proof_res, proof_resA, proof_resB,
                proof_redundant, proof_undef} proofCode;

  typedef enum {str_maxChain, str_maxRes,
                str_maxRef, str_closeGbl} relabelStrategy;
  
  struct ResolutionNode
  {
    vec<Lit>        resolvents;
    vec<Lit>        pivots;
    vec<int>        antecedents;
    CRef            resClauseId;
    bool            topNode;
    bool            mark;
    proofCode       code;
    proofCode       savedCode;
    
    ResolutionNode():resolvents(), antecedents(), pivots() {}
    ResolutionNode(vec<Lit>& resolvents) : antecedents(), pivots(){
      resolvents.copyTo(this->resolvents);
      pivots.clear();
      antecedents.clear();
      resClauseId = CRef_Undef;
      topNode = false;
      mark = false;
      code = proof_undef;
      savedCode = proof_undef;
    }
    ResolutionNode(vec<Lit>& resolvents, vec<int>& antecedents, vec<Lit>& pivots) {
      resolvents.copyTo(this->resolvents);
      antecedents.copyTo(this->antecedents);
      pivots.copyTo(this->pivots);
      resClauseId = CRef_Undef;
      topNode = false;
      mark = false;
      code = proof_undef;
      savedCode = proof_undef;
    }
    ~ResolutionNode () {}
    ResolutionNode(const ResolutionNode& rn) {
      rn.resolvents.copyTo(this->resolvents);
      rn.antecedents.copyTo(this->antecedents);
      rn.pivots.copyTo(this->pivots);
      this->resClauseId = rn.resClauseId;
      this->topNode = rn.topNode;
      this->mark = rn.mark;
      this->code = rn.code;
      this->savedCode = rn.savedCode;
    }
    ResolutionNode& operator = (const ResolutionNode& rn) {
      rn.resolvents.copyTo(this->resolvents);
      rn.antecedents.copyTo(this->antecedents);
      rn.pivots.copyTo(this->pivots);
      this->resClauseId = rn.resClauseId;
      this->topNode = rn.topNode;
      this->mark = rn.mark;
      this->code = rn.code;
      this->savedCode = rn.savedCode;
      return *this;
    }

    void setResClauseId(CRef id) { resClauseId = id; }
    void setTopNode(bool val) { topNode = val; }
    void setCode(proofCode val) { code = val; }
    void setSavedCode(proofCode val) { savedCode = val; }
    CRef getResClauseId() { return resClauseId; }
    bool isTopNode() { return topNode; }
    proofCode getCode() { return code; }
    proofCode getSavedCode() { return savedCode; }
    int isOriginal() { return antecedents.size() == 0; }
    int isResolvent() { return !isOriginal(); }
    int isChain() { return (isResolvent() && antecedents.size() > 2); }

  };

  struct SolverPdt
  {
    void *S2;
    vec<int> map, invMap;
    vec<char> disClause;

    SolverPdt() {
      map.clear();
      invMap.clear();
      disClause.clear();
    }
    ~SolverPdt () {}

    vec<int> (&getInvMap()) { return invMap; }
    vec<int> (&getMap()) { return map; }
    vec<char> (&getDisClause()) { return disClause; }

  };

  typedef enum {ratio, ratiob, bframes, fanout, lev, noop} proofStrategy;

  struct ProofPdt
  {
    /// -- ResolutionNodes management

    enum {labeled, undef} status;
    proofStrategy strategy = noop;
    char *stratNames[noop+1] =
      {"ratio","ratiob", "bframes", "fanout","lev","noop"};
    
    int nSolverAClauses;
    unsigned int nSolverACr;
    int nANodes, nBNodes, nResNodes, nRedNodes;
    int verbosityLevel;
    bool saved;
    bool solverUndef;
    vec<ResolutionNode> resNodes;
    vec<bool> isProofVar;
    vec<bool> isUsedVar;
    vec<bool> isAvar;
    vec<bool> isBvar;
    vec<bool> isGlobal;
    vec<bool> isB2Avar;
    vec<bool> litsA;
    vec<bool> litsB;
    vec<bool> unitA;
    vec<bool> unitB;

    vec<bool> savedIsAvar;
    vec<bool> savedIsGlobal;
    vec<int> remapVars;

    ProofPdt(void) {
      status = undef;
      solverUndef = false;
      saved = false;
      verbosityLevel = 0;
      nANodes = nBNodes = nResNodes = nRedNodes =
        nSolverAClauses = nSolverACr = 0;
      resNodes.clear();
      isUsedVar.clear();
      isProofVar.clear();
      isBvar.clear();
      isAvar.clear();
      isGlobal.clear();
      isB2Avar.clear();
      savedIsAvar.clear();
      savedIsGlobal.clear();
      remapVars.clear();
    }

    bool UsedVar(Var v) {return isUsedVar[v];}
    bool ProofVar(Var v) {return isProofVar[v];}
    bool Avar(Var v) {return isAvar[v];}
    bool Bvar(Var v) {return isBvar[v];}
    bool Global(Var v) {return isGlobal[v];}
    void setVerbosity(int v) {verbosityLevel = v;}
    void setSolverUndef(void) {solverUndef = true;}
    int verbosity(void) {return verbosityLevel;}
    void setAvar(Var v, bool val) {isAvar[v] = val;}
    void setBvar(Var v, bool val) {isBvar[v] = val;}
    void setGlobal(Var v, bool val) {isGlobal[v] = val;}
    void varSetB2A(Var v) {
      if (isB2Avar.size()<=v) isB2Avar.growTo(v+1,false);
      isB2Avar[v] = true;
    }
    bool B2AvarActive(void) {return isB2Avar.size()>0;}
    bool B2Avar(Var v) {
      if (v>= isB2Avar.size()) return false;
      else return isB2Avar[v];
    }
    bool GlobalUnateA(Var v, int& sign) {
      int l0 = toInt(mkLit(v,0));
      int l1 = toInt(mkLit(v,1));
      if (!Global(v)) return false;
      if (litsA[l0]!=litsA[l1]) {
        sign = litsA[l0] ? 0 : 1;
        return true;
      }
      return false;
    }

  };
}

#endif /* PDTLINK_H_ */

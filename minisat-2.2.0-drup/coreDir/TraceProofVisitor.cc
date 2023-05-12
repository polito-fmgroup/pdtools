#include "TraceProofVisitor.h"

namespace Minisat
{
  namespace
  {
    void toDimacs (FILE* out, Lit lit)
    {
      fprintf (out, "%s%d", sign (lit) ? "-" : "", var (lit) + 1);
    }

    void toDimacs (FILE* out, const Clause &c)
    {
      for (int i = 0; i < c.size (); ++i)
      {
        toDimacs (out, c[i]);
        fprintf (out, " ");
      }
    }

  }

  //Initialize proof visitor
  TraceProofVisitor::TraceProofVisitor (Solver &solver, FILE* out)
    : m_Solver (solver), m_ids(1), m_out (out)
   {
     m_units.growTo (m_Solver.nVars (), -1);
     doUndef = false;
     undefTopClauses.clear();
   }

  int TraceProofVisitor::genUndefTopClauses (
	vec<Var>& filterVars, vec<Var>& filter2Vars)
  {
    int quantify = 0;
    vec<int> selectVar;

    doUndef = true;
    undefTopClauses.clear();

    m_Solver.printLearntsStats(filterVars,filter2Vars);
    m_Solver.getFilteredLearnts(undefTopClauses,filterVars,filter2Vars);

  }

  //Trace a single resolvent
  //The rolution proof is traced as a sequence of binary resolutions
  int TraceProofVisitor::visitResolvent (Lit parent, Lit p1, CRef p2)
  {

    //If the unit resolving clause doesn't already have a node id, one is assigned
    //and the node is outputted
    vec<Lit> res;
    vec<int> ants;
    vec<Lit> pivots;

    if (m_units [var (p1)] < 0)
    {
      m_units [var (p1)] = m_ids++;
      //      fprintf (m_out, "%d ", m_ids - 1);
      //      toDimacs (m_out, p1);
      //      fprintf (m_out, " 0 0\n");
      res.push(p1);
      m_Solver.proofPdt.resNodes.growTo(m_ids-1);
      m_Solver.proofPdt.resNodes[m_ids-2] = ResolutionNode(res);
      CRef cr = m_Solver.getReason(var(p1));
      m_Solver.proofPdt.resNodes[m_ids-2].setResClauseId(cr);
      res.clear();
    }
    int id;

    //If the other resolving clause is already assigned a node id get it otherwise
    //assign it. Then print it to output.
    if (!m_visited.has (p2, id))
    {
      m_visited.insert (p2, m_ids++);
      //      fprintf (m_out, "%d ", m_ids - 1);
      //      toDimacs (m_out, m_Solver.getClause (p2));
      //      fprintf (m_out, " 0 0\n");
      const Clause& cl = m_Solver.getClause (p2);
      for(int i=0;i<cl.size();i++) {
        res.push(cl[i]);
      }
      m_Solver.proofPdt.resNodes.growTo(m_ids-1);
      m_Solver.proofPdt.resNodes[m_ids-2] = ResolutionNode(res);
      m_Solver.proofPdt.resNodes[m_ids-2].setResClauseId(p2);
      res.clear();
    }

    //Assign a node id to the unit clause resulting from resolution
    m_units [var (parent)] = m_ids++;

    //Add a line to output for the current resolution
    //Format: ResolventID ResolventClause 0 Clause1ID Clause2ID 0\n
    //    fprintf (m_out, "%d ", m_ids - 1);
    //    toDimacs (m_out, parent);
    //    fprintf (m_out, " 0 ");

    id = -1;
    m_visited.has (p2, id);
    //    fprintf (m_out, "%d %d 0\n", m_units [var (p1)], id);

    res.push(parent);
    ants.push(m_units [var (p1)]-1);
    ants.push(id-1);
    pivots.push(p1);
    m_Solver.proofPdt.resNodes.growTo(m_ids-1);
    m_Solver.proofPdt.resNodes[m_ids-2] = ResolutionNode(res, ants, pivots);
    return 0;
  }

  //Trace a resolution chain.
  //chainClauses
  int TraceProofVisitor::visitChainResolvent (Lit parent)
  {

    //Create noodes for the antecedent nodes
    doAntecendents ();

    vec<Lit> res;
    vec<int> ants;
    vec<Lit> pivots;

    Var vp = var (parent);

    //Record resolution
    //Format: ResolventID ResolventClause 0 Clause1ID ... ClauseNID 0\n
    int resId = m_ids++;
    m_units [vp] = resId;
    //    fprintf (m_out, "%d ", resId);
    //    toDimacs (m_out, parent);
    //    fprintf (m_out, " 0 ");

    int id;
    m_visited.has (chainClauses [0], id);
    //    fprintf (m_out, "%d ", id);
    ants.push(id-1);
    for (int i = 0; i < chainPivots.size (); ++i)
    {
      pivots.push(chainPivots[i]);
      if (chainClauses[i+1] != CRef_Undef)
      {
        m_visited.has (chainClauses [i+1], id);
	//        fprintf (m_out, "%d ", id);
        ants.push(id-1);
      }
      else {
	//        fprintf (m_out, "%d ", m_units [var (chainPivots [i])]);
        ants.push(m_units [var (chainPivots [i])]-1);
      }
    }
    //    fprintf (m_out, " 0\n");

    res.push(parent);
    m_Solver.proofPdt.resNodes.growTo(resId);
    m_Solver.proofPdt.resNodes[resId-1] = ResolutionNode(res, ants, pivots);
    return 0;
  }

  void TraceProofVisitor::doAntecendents ()
  {

    //Create a node for the first clause of the chain
    int id;
    vec<Lit> res;

    static int nSteps = 0;
    
    if (!m_visited.has (chainClauses [0], id))
    {
      m_visited.insert (chainClauses [0], m_ids++);
      //      fprintf (m_out, "%d ", m_ids-1);
      //      toDimacs (m_out, m_Solver.getClause (chainClauses [0]));
      //      fprintf (m_out, " 0 0\n");
      const Clause& cl = m_Solver.getClause (chainClauses [0]);
      for(int i=0;i<cl.size();i++) {
        res.push(cl[i]);
      }
      m_Solver.proofPdt.resNodes.growTo(m_ids-1);
      m_Solver.proofPdt.resNodes[m_ids-2] = ResolutionNode(res);
      m_Solver.proofPdt.resNodes[m_ids-2].setResClauseId(chainClauses [0]);
      res.clear();
    }

    //Scan the chain pivots
    for (int i = 0; i < chainPivots.size (); ++i)
    {
      if (i + 1 < chainClauses.size () && chainClauses [i+1] != CRef_Undef)
      {
        if (!m_visited.has (chainClauses [i+1], id))
        {
          m_visited.insert (chainClauses [i+1], m_ids++);
	  //          fprintf (m_out, "%d ", m_ids-1);
	  //          toDimacs (m_out, m_Solver.getClause (chainClauses [i+1]));
	  //          fprintf (m_out, " 0 0\n");
          const Clause& cl = m_Solver.getClause (chainClauses [i+1]);
          for(int i=0;i<cl.size();i++) {
            res.push(cl[i]);
          }
          m_Solver.proofPdt.resNodes.growTo(m_ids-1);
          m_Solver.proofPdt.resNodes[m_ids-2] = ResolutionNode(res);
          m_Solver.proofPdt.resNodes[m_ids-2].setResClauseId(chainClauses [i+1]);
          res.clear();
        }
      }
      else
      {

        //Otherwise assign an id to each unassigned pivot literals
        Var vp = var (chainPivots [i]);
        if (m_units [vp] < 0)
        {
          m_units [vp] = m_ids++;
	  //          fprintf (m_out, "%d ", m_ids-1);
	  //          toDimacs (m_out, chainPivots [i]);
	  //          fprintf (m_out, " 0 0\n");
	  //  fprintf (m_out, "REASON: %d\n", m_Solver.getReason(vp));
	  CRef r = m_Solver.getReason(vp);

	  assert(r>=0);
	  assert(r!=CRef_Undef);

	  const Clause& cl = m_Solver.getClause(r);
	  //	  assert(cl.size()==1);
	  if (cl.size()!=1) {
	    fprintf(stderr,"ERROR: unit clause expected\n");
	    assert(0);
	  }
          for(int i=0;i<cl.size();i++) { // @@@@ seg fault !!! (6s35)
	    assert(var(cl[i])==vp);
            res.push(cl[i]);
          }
	  // res.push(chainPivots [i]);
          m_Solver.proofPdt.resNodes.growTo(m_ids-1);
          m_Solver.proofPdt.resNodes[m_ids-2] = ResolutionNode(res);
          m_Solver.proofPdt.resNodes[m_ids-2].setResClauseId(r);
          res.clear();
        }
      }
    }
  }

  int TraceProofVisitor::visitChainResolvent (CRef parent)
  {
    doAntecendents ();

    vec<Lit> res;
    vec<int> ants;
    vec<Lit> pivots;

    int resId = m_ids++;
    if (parent != CRef_Undef) {
      m_visited.insert (parent, resId);
      const Clause& cl = m_Solver.getClause(parent);
      if (cl.size()==1) {
        Var vp = var (cl[0]);
        assert (m_units [vp] < 0); 
	m_units [vp] = resId;
      }
    }
    //    fprintf (m_out, "%d ", resId);
    //    if (parent != CRef_Undef) toDimacs (m_out, m_Solver.getClause (parent));
    //    fprintf (m_out, " 0 ");

    int id;
    if (!m_visited.has (chainClauses [0], id)) {
      fprintf (m_out, "error: missing visited id\n");
      assert(0);
    }
    ants.push(id-1);
    //    fprintf (m_out, "%d ", id);
    for (int i = 0; i < chainPivots.size (); ++i)
    {
      pivots.push(chainPivots[i]);
      if (chainClauses[i+1] != CRef_Undef)
      {
	m_visited.has (chainClauses [i+1], id);
	//        fprintf (m_out, "%d ", id);
	assert(id>0);
        ants.push(id-1);
      }
      else {
	//        fprintf (m_out, "%d ", m_units [var (chainPivots [i])]);
        assert(m_units [var (chainPivots [i])]>0);
        ants.push(m_units [var (chainPivots [i])]-1);
      }
    }
    //    fprintf (m_out, " 0\n");
    if(parent != CRef_Undef){
      const Clause& cl = m_Solver.getClause (parent);
      for(int i=0;i<cl.size();i++) {
        res.push(cl[i]);
      }
    }
    m_Solver.proofPdt.resNodes.growTo(resId);
    m_Solver.proofPdt.resNodes[resId-1] = ResolutionNode(res, ants, pivots);
    m_Solver.proofPdt.resNodes[resId-1].setResClauseId(parent);
    if (doUndef && isUndefTopClause (parent)>=0) {
      m_Solver.proofPdt.resNodes[resId-1].setResClauseId(parent);
      // printf("topNode: %d - size: %d\n", parent, res.size());
    }

    return 0;
  }

  void TraceProofVisitor::printUndefTopClauses ()
  {
    for (int i=0; i<undefTopClauses.size(); i++) {
      CRef id;
      if ((id = undefTopClauses[i]) != CRef_Undef) {
	const Clause& cl = m_Solver.getClause (id);
	printf("Top cl[%d] - size: %d\n", i, cl.size());
      }
    }
  }

#ifdef NDEBUG
  static bool pdtChecks = 0;
#else
  static bool pdtChecks = 1;
#endif
  
  static int reLabelResNodes1(ProofPdt& proofPdt,
                              float extraRatio) {
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;
    int nAdded=0, i;
    int nRes = proofPdt.nResNodes;
    int nB = proofPdt.nBNodes;
    int nExtra = extraRatio * nRes;
    if (proofPdt.strategy == ratiob)
      nExtra = extraRatio * nB;
    for (i = 0; i < resNodes.size(); i++) {
      proofCode code_i = resNodes[i].getCode();
      resNodes[i].mark = false;
      if (i==(resNodes.size()-1)) {
        break; // skip last node as clause is void
      }
      if (proofPdt.strategy == ratio && code_i == proof_res) {
        resNodes[i].setCode(proof_rootA);
        resNodes[i].mark = true;
        proofPdt.nResNodes--;
        proofPdt.nANodes++;
        if (++nAdded >= nExtra) break;
      }
      else if (proofPdt.strategy == ratiob &&
               code_i == proof_rootB) {
        resNodes[i].setCode(proof_rootA);
        resNodes[i].mark = true;
        proofPdt.nBNodes--;
        proofPdt.nANodes++;
        if (++nAdded >= nExtra) break;
      }
    }
    if (proofPdt.verbosity()>0)
      printf("added %d root A nodes from res\n", nAdded);
    return nAdded;
  }

  static int reLabelResNodes3(ProofPdt& proofPdt) {
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;
    int nAdded=0, i;
    int nRes = proofPdt.nResNodes;
    int nB = proofPdt.nBNodes;
    assert (proofPdt.strategy == bframes);

    for (i = 0; i < resNodes.size(); i++) {
      proofCode code_i = resNodes[i].getCode();
      resNodes[i].mark = false;
      if (i==(resNodes.size()-1)) {
        break; // skip last node as clause is void
      }
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      for (int j=0; j<c.size(); j++) {
        int v = var(c[j]);
        if (proofPdt.B2Avar(v)) {
          resNodes[i].setCode(proof_rootA);
          resNodes[i].mark = true;
          proofPdt.nBNodes--;
          proofPdt.nANodes++;
          nAdded++;
          break;
        }
      }
    }
    if (proofPdt.verbosity()>0)
      printf("added %d root A nodes from bframes\n", nAdded);
    return nAdded;
  }

  static void resNodesReduceCore(ProofPdt& proofPdt) {
    int nCore=0;
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;
    int nNodes = resNodes.size();
    vec<bool>mark(nNodes,false);
    for (int i = 0; i < nNodes; i++) {
      proofCode code_i = resNodes[i].getCode();
      if (code_i == proof_res) {
        for(int j=0; j < resNodes[i].antecedents.size(); j++){
          int antId = resNodes[i].antecedents[j];
          mark[antId] = true;
        }
      }
    }
    nCore = nNodes;
    for (int i = 0; i < nNodes; i++) {
      proofCode code_i = resNodes[i].getCode();
      if (code_i == proof_res) continue;
      if (mark[i]) {
        if (code_i == proof_resA) {
          resNodes[i].setCode(proof_rootA);
          proofPdt.nANodes++;
          proofPdt.nResNodes--;
        }
        if (code_i == proof_resB) {
          resNodes[i].setCode(proof_rootB);
          proofPdt.nBNodes++;
          proofPdt.nResNodes--;
        }
        continue;
      }
      if (code_i == proof_rootA) proofPdt.nANodes--;
      if (code_i == proof_rootB) proofPdt.nBNodes--;
      if (code_i == proof_res) proofPdt.nResNodes--;
      if (code_i == proof_resA) proofPdt.nResNodes--;
      if (code_i == proof_resB) proofPdt.nResNodes--;
      proofPdt.nRedNodes++;
      resNodes[i].setCode(proof_redundant);
      nCore--;
    }
    if (proofPdt.verbosity()>0)
      printf("RES PROOF has %d core and %d out of core nodes\n",
           nCore, proofPdt.resNodes.size()-nCore);
  }
  
  static int reLabelResNodes2(ProofPdt& proofPdt,
                              int nRoot, float extraRatio) {
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;
    int nAdded=0;
    int nNodes = resNodes.size();
    vec<int> refCount(nNodes,0);
    vec<int> levelMin(nNodes,0);
    vec<int> levelMax(nNodes,0);
    vec<int> levelChainMax(nNodes,0);
    int nExtra = 10;

    relabelStrategy strategy = str_maxRef;
    assert(proofPdt.isGlobal.size()>0);
    
    for (int i = 0; i < nNodes; i++) {
      int minl=0, maxl=0, maxchl=0;
      //      int add = ((j==0)?1:j); 
      int add = 1;
      proofCode code_i = resNodes[i].getCode();
      if (code_i!=proof_rootA && code_i!=proof_rootB) {
        for(int j=0; j < resNodes[i].antecedents.size(); j++){
          int antId = resNodes[i].antecedents[j];
          int lmin_i = levelMin[antId] + add;
          int lmax_i = levelMax[antId] + add;
          int lchmax_i = levelChainMax[antId] + 1;
          assert (antId>=0 && antId<nNodes);
          refCount[antId]++;
          if ((j==0) || (lmin_i<minl)) minl = lmin_i;
          if ((j==0) || (lmax_i>maxl)) maxl = lmax_i;
          if ((j==0) || (lchmax_i>maxchl)) maxchl = lchmax_i;
        }
        if (strategy == str_closeGbl) {
          bool hasGbl = false;
          vec<Lit>& c = proofPdt.resNodes[i].resolvents;
          for (int j=0; j<c.size(); j++) {
            int v = var(c[j]);
            if (proofPdt.Global(v)) {
              hasGbl = true;
              break;
            }
          }              
          if (!hasGbl) maxchl += 100;
        }
      }
      levelMin[i] = minl;
      levelMax[i] = maxl;
      levelChainMax[i] = maxchl;
      //      printf("levelMax[%d] %d\n", i, maxl);
    }

    vec<bool>mark(nNodes,false);
    // refCount
    vec<int> topRefs;
    for (int k=0; k<nExtra; k++) {
      int imax = -1;
      for (int i = 0; i < nNodes; i++) {
        if (mark[i]) continue;
        if (imax<0 || refCount[i]>refCount[imax]) imax = i;
      }
      topRefs.push(imax);
      mark[imax] = true;
    }
    // levelMax
    vec<int> topLevelMax;
    mark.clear(); mark.growTo(nNodes,false);
    for (int k=0; k<nExtra; k++) {
      int imax = -1;
      for (int i = 0; i < nNodes; i++) {
        if (mark[i]) continue;
        if (imax<0 || levelMax[i]>levelMax[imax]) imax = i;
      }
      topLevelMax.push(imax);
      mark[imax] = true;
    }
    // levelMax
    vec<int> topLevelChainMax;
    mark.clear(); mark.growTo(nNodes,false);
    for (int k=0; k<nExtra; k++) {
      int imax = -1;
      for (int i = 0; i < nNodes; i++) {
        if (mark[i]) continue;
        if (imax<0||levelChainMax[i]>levelChainMax[imax]) imax = i;
      }
      topLevelChainMax.push(imax);
      mark[imax] = true;
    }
    // levelMin
    vec<int> topLevelMin;
    mark.clear(); mark.growTo(nNodes,false);
    for (int k=0; k<nExtra; k++) {
      int imax = -1;
      for (int i = 0; i < nNodes; i++) {
        if (mark[i]) continue;
        if (imax<0 || levelMin[i]>levelMin[imax]) imax = i;
      }
      mark[imax] = true;
      topLevelMin.push(imax);
    }

    if (proofPdt.verbosity()>1) {
      printf("top ref Count:");
      for (int k=0; k<nExtra; k++) {
        printf(" [%d]%d", topRefs[k], refCount[topRefs[k]]);
      }
      printf("\n");
      printf("top level max:");
      for (int k=0; k<nExtra; k++) {
        printf(" [%d]%d", topLevelMax[k], levelMax[topLevelMax[k]]);
      }
      printf("\n");
      printf("top level chain max:");
      for (int k=0; k<nExtra; k++) {
        printf(" [%d]%d", topLevelMax[k], levelMax[topLevelMax[k]]);
      }
      printf("\n");
      printf("top level min:");
      for (int k=0; k<nExtra; k++) {
        printf(" [%d]%d", topLevelMin[k], levelMin[topLevelMin[k]]);
      }
      printf("\n");
    }
    
    //nExtra = extraRatio * (nNodes-(nRoot));
    //   nExtra = extraRatio * nB;
    int levelThreshold;
    bool selectUpper = false;
    vec<int> *lev;
    switch (strategy) {
    case str_maxChain:
    case str_closeGbl:
      levelThreshold = 1;
      lev = &levelChainMax;
      if (proofPdt.verbosity()>1) {
        printf("using level max threshold: %d\n", levelThreshold);
        printf("level max of last node: %d\n",
             levelChainMax[nNodes-1]);
      }
      break;
    case str_maxRes:
      lev = &levelMax;
      levelThreshold = levelMax[topLevelMax.last()] / 100;
      if (proofPdt.verbosity()>1) {
        printf("using level max threshold: %d\n", levelThreshold);
        printf("level max of last node: %d\n", levelMax[nNodes-1]);
      }
      break;
    case str_maxRef:
      selectUpper = true;
      lev = &refCount;
      levelThreshold = (*lev)[topRefs[0]] / 4;
      if (proofPdt.verbosity()>1)
        printf("using ref count min threshold: %d\n", levelThreshold);
      break;
    default: break;
    }
    for (int i = 0; i < nNodes; i++) {
      proofCode code_i = resNodes[i].getCode();
      if (code_i == proof_res) {
        bool select = selectUpper ?
          (*lev)[i]>=levelThreshold :
          (*lev)[i]<=levelThreshold;
        if (select) {
          resNodes[i].setCode(proof_rootA);
          ++nAdded;
          resNodes[i].mark = true;
          if (i==(nNodes-1)) {
            assert(0);
            printf("oops: %d\n",i);
          }
        }
      }
    }

    return nAdded;
  }

  static void resNodesStats(ProofPdt& proofPdt) {
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;
    int nAdded=0;
    int nNodes = resNodes.size();
    vec<int> refCount(nNodes,0);
    vec<int> levelMin(nNodes,0);
    vec<int> levelMax(nNodes,0);
    vec<int> levelChainMax(nNodes,0);
    int nExtra = 10;

    relabelStrategy strategy = str_maxRef;
    assert(proofPdt.isGlobal.size()>0);
    
    for (int i = 0; i < nNodes; i++) {
      int minl=0, maxl=0, maxchl=0;
      //      int add = ((j==0)?1:j); 
      int add = 1;
      proofCode code_i = resNodes[i].getCode();
      if (code_i!=proof_rootA && code_i!=proof_rootB) {
        for(int j=0; j < resNodes[i].antecedents.size(); j++){
          int antId = resNodes[i].antecedents[j];
          int lmin_i = levelMin[antId] + add;
          int lmax_i = levelMax[antId] + add;
          int lchmax_i = levelChainMax[antId] + 1;
          assert (antId>=0 && antId<nNodes);
          refCount[antId]++;
          if ((j==0) || (lmin_i<minl)) minl = lmin_i;
          if ((j==0) || (lmax_i>maxl)) maxl = lmax_i;
          if ((j==0) || (lchmax_i>maxchl)) maxchl = lchmax_i;
        }
        if (strategy == str_closeGbl) {
          bool hasGbl = false;
          vec<Lit>& c = proofPdt.resNodes[i].resolvents;
          for (int j=0; j<c.size(); j++) {
            int v = var(c[j]);
            if (proofPdt.Global(v)) {
              hasGbl = true;
              break;
            }
          }              
          if (!hasGbl) maxchl += 100;
        }
      }
      levelMin[i] = minl;
      levelMax[i] = maxl;
      levelChainMax[i] = maxchl;
      //      printf("levelMax[%d] %d\n", i, maxl);
    }

    vec<bool>mark(nNodes,false);
    // refCount
    vec<int> topRefs;
    for (int k=0; k<nExtra; k++) {
      int imax = -1;
      for (int i = 0; i < nNodes; i++) {
        if (mark[i]) continue;
        proofCode code_i = resNodes[i].getCode();
        if (levelChainMax[i]>1) continue;
        if (code_i != proof_res) continue;
        if (imax<0 || refCount[i]>refCount[imax]) imax = i;
      }
      if (imax>=0) {
        topRefs.push(imax);
        assert((imax>=0)&&(imax<nNodes));
        mark[imax] = true;
      }
      else
        break;
    }
    // levelMax
    vec<int> topLevelMax;
    mark.clear(); mark.growTo(nNodes,false);
    for (int k=0; k<nExtra; k++) {
      int imax = -1;
      for (int i = 0; i < nNodes; i++) {
        if (mark[i]) continue;
        if (imax<0 || levelMax[i]>levelMax[imax]) imax = i;
      }
      if (imax>=0) {
        topLevelMax.push(imax);
        assert((imax>=0)&&(imax<nNodes));
        mark[imax] = true;
      }
    }
    // levelMax
    vec<int> topLevelChainMax;
    mark.clear(); mark.growTo(nNodes,false);
    for (int k=0; k<nExtra; k++) {
      int imax = -1;
      for (int i = 0; i < nNodes; i++) {
        if (mark[i]) continue;
        if (imax<0||levelChainMax[i]>levelChainMax[imax]) imax = i;
      }
      if (imax>=0) {
        topLevelChainMax.push(imax);
        assert((imax>=0)&&(imax<nNodes));
        mark[imax] = true;
      }
    }
    // levelMin
    vec<int> topLevelMin;
    mark.clear(); mark.growTo(nNodes,false);
    for (int k=0; k<nExtra; k++) {
      int imax = -1;
      for (int i = 0; i < nNodes; i++) {
        if (mark[i]) continue;
        if (imax<0 || levelMin[i]>levelMin[imax]) imax = i;
      }
      if (imax>=0) {
        mark[imax] = true;
        assert((imax>=0)&&(imax<nNodes));
        topLevelMin.push(imax);
      }
    }

    int n1=0, n2=0, nRes1=0, sizeRes1=0, totRes=0, totChain=0,
      nG1=0, nG2=0, nA1=0, nB1=0, nAtot=0, nBtot=0, nGtot=0,
      refTot=0;
    
    for (int i = 0; i < nNodes; i++) {
      proofCode code_i = resNodes[i].getCode();
      if (code_i != proof_res) continue;
      vec<Lit>& pivots = resNodes[i].pivots;
      vec<int>& antecedents = resNodes[i].antecedents;
      totRes += pivots.size();
      totChain++;
      if (levelChainMax[i]>1) continue;


      int nGl=0, nB=0, nA=0;
      for(int j=0; j<pivots.size(); j++) {
        Lit p = pivots[j];
        Var v = var(p);
        if (proofPdt.Global(v)) nGl++;
        else if (proofPdt.Avar(v)) nA++;
        else if (proofPdt.Bvar(v)) nB++;
      }
      //      assert(nGl>0);
      refTot += refCount[i]; 
      if (nGl==1) nG1++;
      if (nGl==2) nG2++;
      if (nA==1) nA1++;
      if (nB==1) nB1++;
      nGtot += nGl;
      nAtot += nA;
      nBtot += nB;
      nRes1++;
      sizeRes1 += pivots.size();
      if (pivots.size()==1) n1++;
      if (pivots.size()==2) n2++;
    }

    if (proofPdt.verbosity()>1) {
      printf("top ref Count (chain level 1:");
      for (int k=0; k<nExtra && k<topRefs.size(); k++) {
        printf(" [%d]%d", topRefs[k], refCount[topRefs[k]]);
      }
      printf("\n");
      printf("top level max:");
      for (int k=0; k<topLevelMax.size(); k++) {
        printf(" [%d]%d", topLevelMax[k], levelMax[topLevelMax[k]]);
      }
      printf("\n");
      printf("top level chain max:");
      for (int k=0; k<topLevelChainMax.size(); k++) {
        printf(" [%d]%d", topLevelChainMax[k], levelMax[topLevelChainMax[k]]);
      }
      printf("\n");
      printf("top level min:");
      for (int k=0; k<topLevelMin.size(); k++) {
        printf(" [%d]%d", topLevelMin[k], levelMin[topLevelMin[k]]);
      }
      printf("\n");
      printf("res level 1 stats: chain/tot %d/%d res/tot: %d/%d  avg size: %f - avg ref: %f\n",
             nRes1, totChain, sizeRes1, totRes,
             (float(sizeRes1))/nRes1, (float(refTot))/nRes1);
      printf("res level 1 stats:  A: %d B: %d G: %d\n",
             nAtot, nBtot, nGtot);
      printf("res level 1 stats: A1: %d B1: %d G1: %d, G2: %d\n",
             nA1, nB1, nG1, nG2);
    }    
  }

  void Solver::proofReverseAB(void) {
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;
    assert(proofPdt.status==proofPdt.labeled);
    
    // A clauses
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      proofCode code_i = resNodes[i].getCode();
      proofCode new_code_i = code_i;
      if (code_i == proof_rootA) new_code_i = proof_rootB;
      if (code_i == proof_resA) new_code_i = proof_resB;
      if (code_i == proof_rootB) new_code_i = proof_rootA;
      if (code_i == proof_resB) new_code_i = proof_resA;
      proofPdt.resNodes[i].setCode(new_code_i);
      int t = proofPdt.nANodes;
      proofPdt.nANodes = proofPdt.nBNodes;
      proofPdt.nBNodes = t;
    }
    for (int i=0; i<proofPdt.isAvar.size(); i++) {
      if (!proofPdt.isGlobal[i]) {
        proofPdt.isAvar[i] = !proofPdt.isAvar[i];
      }
    }
  }


  void Solver::proofSetupProofVars(void) {
    if (proofPdt.isProofVar.size()>0) return;

    proofPdt.isUsedVar.growTo(nVars(),false);
    proofPdt.isProofVar.growTo(nVars(),false);
    proofPdt.isAvar.growTo(nVars(),false);
    proofPdt.isBvar.growTo(nVars(),false);
    proofPdt.isGlobal.growTo(nVars(),false);

    unsigned int nACr = proofPdt.nSolverACr;

    for (int i=0; i<proof.size(); i++) {
      CRef cr = proof [i];
      const Clause& c = getClause (cr);
      for(int j=0;j<c.size();j++) {
        int v = var(c[j]);
        proofPdt.isUsedVar[v] = true; 
        if (c.core()) {
          proofPdt.isProofVar[v] = true; 
          if (cr>nACr) { // B clause
            proofPdt.isBvar[v] = true; 
          }
          else { // a clause
            proofPdt.isAvar[v] = true; 
          }
        }
      }
    }
    for (int i=0; i<clauses.size(); i++) {
      CRef cr = clauses [i];
      const Clause& c = getClause (cr);
      for(int j=0;j<c.size();j++) {
        int v = var(c[j]);
        proofPdt.isUsedVar[v] = true; 
        if (c.core()) {
          proofPdt.isProofVar[v] = true; 
          if (cr>nACr) { // B clause
            proofPdt.isBvar[v] = true; 
          }
          else { // a clause
            proofPdt.isAvar[v] = true; 
          }
        }
      }
    }
    for (int i=0; i<nVars(); i++) {
      proofPdt.isGlobal[i] = proofPdt.isAvar[i] &&
        proofPdt.isBvar[i];
    }
  }
  
  void Solver::proofSetupVars(void) {
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;

    proofPdt.isAvar.clear();
    proofPdt.isBvar.clear();
    proofPdt.isGlobal.clear();
    proofPdt.isAvar.growTo(nVars(),false);
    proofPdt.isBvar.growTo(nVars(),false);
    proofPdt.isGlobal.growTo(nVars(),false);

    vec<bool>& litsA = proofPdt.litsA;
    vec<bool>& litsB = proofPdt.litsB;
    vec<bool>& unitA = proofPdt.unitA;
    vec<bool>& unitB = proofPdt.unitA;

    unitA.clear(); unitB.clear(); litsA.clear(); litsB.clear();
    unitA.growTo(2*nVars(),false);
    unitB.growTo(2*nVars(),false);
    litsA.growTo(2*nVars(),false);
    litsB.growTo(2*nVars(),false);

    assert(proofPdt.status==proofPdt.labeled);
    
    // A clauses
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      proofCode code_i = resNodes[i].getCode();
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      int isACl = (code_i==proof_rootA || code_i==proof_resA);
      if (isACl) {
        for (int j=0; j<c.size(); j++) {
          int v = var(c[j]);
          proofPdt.isAvar[v] = true;
          int l = toInt(c[j]);
          litsA[l]=true;
          if (c.size()==1) unitA[j]=true;
        }
      }
    }
    // B clauses
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      proofCode code_i = resNodes[i].getCode();
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      int isBCl = (code_i==proof_rootB || code_i==proof_resB);
      if (isBCl) {
        for (int j=0; j<c.size(); j++) {
          int v = var(c[j]);
          proofPdt.isBvar[v] = true;
          if (proofPdt.isAvar[v])
            proofPdt.isGlobal[v] = true;
          int l = toInt(c[j]);
          litsB[l]=true;
          if (c.size()==1) unitB[j]=true;
        }
      }
    }

    int nGbl=0;

    int monoA=0, monoB=0, monoGA=0, monoGB=0, nUnitA=0, nUnitB=0;
    int gUnitA=0, gUnitB=0, gUnitAB=0, gABeq=0, gABdif=0;
    for (int v=0; v<nVars(); v++) {
      int l0 = toInt(mkLit(v,0));
      int l1 = toInt(mkLit(v,1));
      bool glA=false;
      bool glB=false;
      if (proofPdt.Global(v)) nGbl++;
      if (litsA[l0]!=litsA[l1]) {
        assert(proofPdt.Avar(v));
        if (proofPdt.Global(v)) {
          monoGA++; glA=true;
        }
        else
          monoA++;
      }
      if (litsB[l0]!=litsB[l1]) {
        assert(proofPdt.Bvar(v));
        if (proofPdt.Global(v)) {
          monoGB++; glB=true;
        }
        else
          monoB++;
      }
      if (glA||glB) {
        if (glA && glB) {
          if (litsA[l0]!=litsB[l0]) {
            gABdif++;
          }
          else gABeq++;
        }
      }
      if (unitA[l0] || unitA[l1]) {
        nUnitA++;
        if (glA) gUnitA++;
      }
      if (unitB[l0] || unitB[l1]) {
        nUnitB++;
        if (glA) gUnitB++;
        if (glA && glB) gUnitAB++;
      }

    }

    if (proofPdt.verbosity()>1) {
      printf("PROOF single phased vars: A: %d - GA: %d - B: %d - GB: %d\n",
           monoA, monoGA, monoB, monoGB);
      printf("PROOF single phased vars: UA: %d - UB: %d - GUA: %d - GUB: %d\n",
           nUnitA, nUnitB, gUnitA, gUnitB);
      printf("PROOF single phased vars: GUAB: %d - GABeq: %d - GABdif: %d - Gtot: %d\n",
           gUnitAB, gABeq, gABdif, nGbl);
    }
  }

  int Solver::proofMoveNodesToA(float extraRatio){
    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);

    int nNodes = proofPdt.resNodes.size();

    // enum codes: ratio, ratiob, fanout, lev, noop
    proofPdt.strategy = noop;
    if (proofPdt.B2AvarActive())
      proofPdt.strategy = bframes;
      
    int nResA=0, nResB=0, nRes=0, nRed=0, nRedRoot=0;
    int nA=0, nB=0, nExtra, nAdded=0;
    static int i;

    proofSave();
    
    switch (proofPdt.strategy) {
    case ratio:
    case ratiob:
      nAdded = reLabelResNodes1(proofPdt,extraRatio);
      break;
    case lev:
    case fanout:
      nAdded = reLabelResNodes2(proofPdt,nA+nB,extraRatio);
      break;
    case bframes:
      nAdded = reLabelResNodes3(proofPdt);
      break;
    case noop:
    default:
      break;
    }

    for (int i=nNodes-1; i>=0; i--) {
      if (!proofPdt.resNodes[i].mark) continue;
      for(int j=0; j < proofPdt.resNodes[i].antecedents.size(); j++){
        int antId = proofPdt.resNodes[i].antecedents[j];
        proofPdt.resNodes[antId].mark = true;
      }
    }
    int nBnew=0, nBfree=0, nBcovered=0, added0=nAdded;
    for (i = 0; i < nNodes; i++) {
      proofCode code_i = proofPdt.resNodes[i].getCode();
      if (code_i == proof_rootB || code_i == proof_resB) {
        nBnew++;
        if (proofPdt.resNodes[i].mark) {
          if (code_i == proof_rootB)
            proofPdt.nBNodes--;
          else 
            proofPdt.nResNodes--;
          proofPdt.nANodes++;
          nAdded++; 
          nBcovered++;
          proofPdt.resNodes[i].setCode(proof_rootA);
        }
        else {
          nBfree++;
        }
      }
    }
    if (proofPdt.verbosity()>1) {
      printf("total B: %d - covered + not covered: %d+%d - A added: %d+%d\n",
           nBnew, nBcovered, nBfree, added0, nAdded-added0);
    }

    nResA = nResB = 0;
    // update resolution Nodes
    for(i = 0; i < proofPdt.resNodes.size(); i++){
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      proofCode code_i = proofPdt.resNodes[i].getCode();
      // append new b clauses
      if (code_i == proof_redundant) continue;
      if (code_i == proof_rootB || code_i == proof_rootA) continue;
      assert (!proofPdt.resNodes[i].isOriginal());
      proofCode code = proof_undef;
      bool isA = true;
      bool isB = true;
      bool isUndef = true;
      for(int j=0; j < proofPdt.resNodes[i].antecedents.size(); j++){
        int antId = proofPdt.resNodes[i].antecedents[j];
        assert (antId>=0 && antId<proofPdt.resNodes.size());
        proofCode code_j = proofPdt.resNodes[antId].getCode();
        switch (code_j) {
        case proof_rootA:
        case proof_resA:
          isB = false; isUndef = false;
          break;
        case proof_rootB:
        case proof_resB:
          isA = false; isUndef = false;
          break;
        case proof_root:
        case proof_res:
          isA = isB = false; isUndef = false;
          break;
        case proof_undef:
        default:
          assert(0); // problem - resolution node not coded
        }
        if (!isA && !isB) break;
      }
      if (!isUndef) {
        assert(!isA || !isB);
        if (i == proofPdt.resNodes.size()-1)
            isA = isB = 0;
        if (isA) code = proof_resA;
        else if (isB) code = proof_resB;
        else code = proof_res;
        if (code==proof_resA) nResA++;
        if (code==proof_resB) nResB++;
        if (code==proof_res) nRes++;
        proofPdt.resNodes[i].setCode(code);
      }
    }
    if (proofPdt.verbosity()>0)
      printf("RES PROOF (%s) MOVED from (A:%d+B:%d) %d B->A: %d resA, %d resB, %d res, %d redundant (red root: %d)\n",
           proofPdt.stratNames[proofPdt.strategy],
           nA, nB, nAdded, nResA, nResB, nRes, nRed, nRedRoot);

    proofSetupVars();

    return nAdded;
  }
 
  inline bool clauseHasLit (vec<Lit>& c, Lit l) {
    for (int j=0; j<c.size(); j++) {
      if (var(c[j])==var(l)) {
        assert(c[j]==l);
        return true;
      }
    }
    return false;
  }

  inline void proofResolveClearMark (vec<lbool>& mark,
                                     vec<Lit>& c) {
    for (int j=0; j<c.size(); j++) {
      Var v = var(c[j]);
      assert(mark[v]!=l_Undef);
      assert(mark[v]==l_True?!sign(c[j]):sign(c[j]));
      mark[v] = l_Undef;
    }
  }

  inline void proofResolveClearMarkNoCheck (vec<lbool>& mark,
                                     vec<Lit>& c) {
    for (int j=0; j<c.size(); j++) {
      Var v = var(c[j]);
      mark[v] = l_Undef;
    }
  }

  inline bool hasVar (vec<Lit>& c, Var v) {
    for (int j=0; j<c.size(); j++) {
      Var v_j = var(c[j]);
      if (v_j==v) return true;
    }
    return false;
  }

  inline void proofResolveKeepAndClearMark (vec<lbool>& mark,
                                     vec<Lit>& c) {
    int j, jj=0;
    for (j=0; j<c.size(); j++) {
      Var v = var(c[j]);
      if (mark[v]!=l_Undef) {
        if (mark[v]==l_True?!sign(c[j]):sign(c[j])) {
          mark[v] = l_Undef;
          c[jj++] = c[j];
        }
      }
    }
    if (jj<j) {
      c.shrink(j-jj);
    }
  }


#pragma GCC push_options
#pragma GCC optimize ("00")
  
  static int proofResolveStart(vec<Lit>& main,
                               vec<lbool>& mark) {
    int s=main.size();
    int j=0;
    for (j=0; j<s; j++) {
      // reset var usage
      //      static int cnt=0; cnt++;
      Lit l_j = main[j];
      Var v_j = var(l_j);
      //      printf("j: %d, v_j: %d - s: %d - s-j: %d\n", j, v_j, s, s-j);
      if (mark[v_j]!=l_Undef) {
        // common literal
        assert(mark[v_j]==l_True ? !sign(l_j) : sign(l_j));
      }
      mark[v_j] = sign(l_j) ? l_False : l_True;
    }
  }   

  static int proofResolveClauses(vec<Lit>& main,
                                 vec<Lit>& other,
                                 Lit l,
                                 vec<lbool>& mark) {
    Var x = var(l);
    int s=other.size();
    int j=0;
    for (j=0; j<s; j++) {
      // reset var usage
      //      static int cnt=0; cnt++;
      Lit l_j = other[j];
      Var v_j = var(l_j);
      //      printf("j: %d, v_j: %d - s: %d - s-j: %d\n", j, v_j, s, s-j);
      if (x != var_Undef && v_j == x) {
        // remove from array and skip
        assert(sign(l_j)!=sign(l));
        assert(mark[v_j]==(sign(l)?l_False:l_True));
        mark[v_j] = l_Undef;
        continue;
      } 
      if (mark[v_j]!=l_Undef) {
        // common literal
        assert(mark[v_j]==l_True ? !sign(l_j) : sign(l_j));
      }
      else if (x != var_Undef) {
        main.push(l_j);
      }
      mark[v_j] = sign(l_j) ? l_False : l_True;
    }
  }   

#pragma GCC pop_options
  
  //NB: call only after replay
  static int proofNodeSwapResolutions(ProofPdt& proofPdt,
                                      int i, bool doStrengthen) {
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;
    assert(i>=0 && i< resNodes.size());
    int nSwap=0;
    proofCode code_i = resNodes[i].getCode();
    if (code_i != proof_res) return 0;
    vec<Lit>& pivots = resNodes[i].pivots;
    vec<int>& antecedents = resNodes[i].antecedents;
    // check antecedents
    int lev1Only = 0;
    if (lev1Only) {
      for(int j=0; j<antecedents.size(); j++) {
        int antId = antecedents[j];
        proofCode code_j = resNodes[antId].getCode();
        if (code_j == proof_res) return 0;
      }
    }
    assert(pivots.size()>0);
    if (proofPdt.Global(var(pivots[0]))) {
      int ant0 = antecedents[0];
      int ant1 = antecedents[1];
      proofCode code_0 = resNodes[ant0].getCode();
      proofCode code_1 = resNodes[ant1].getCode();
      if ((code_0 == proof_resA || code_0 == proof_rootA) &&
          (code_1 == proof_resB || code_1 == proof_rootB)) {
        // swap so that antecedents[0] is B
        antecedents[0] = ant1;
        antecedents[1] = ant0;
        pivots[0] = ~pivots[0];
      }
    }
    int nRounds = 4;
    int pSz = pivots.size(); 
    for (int k=0; k<2*nRounds; k++) {
      int nLocalSwaps=0;
      for(int jj=0; jj<pSz-1; jj++) {
        int j;
        if (k%2)
          j = pSz-2-jj;
        else
          j = jj;
        Lit p0 = pivots[j];
        Var v0 = var(p0);
        Lit p1 = pivots[j+1];
        Var v1 = var(p1);
        int ant0 = antecedents[j+1];
        int ant1 = antecedents[j+2];
        bool gbl0 = proofPdt.Global(v0);
        bool gbl1 = proofPdt.Global(v1);
        bool a0 = !proofPdt.Global(v0) && proofPdt.Avar(v0);
        bool a1 = !proofPdt.Global(v1) && proofPdt.Avar(v1);
        bool enSwap = doStrengthen ? gbl0 && a1 : gbl1 && a0;
        if (enSwap) {
          proofCode code_0 = resNodes[ant0].getCode();
          proofCode code_1 = resNodes[ant1].getCode();
          vec<Lit>& c0 = resNodes[ant0].resolvents;
          vec<Lit>& c1 = resNodes[ant1].resolvents;
          if ((code_0 == proof_resA || code_0 == proof_rootA) &&
              (code_1 == proof_resA || code_1 == proof_rootA)) {
            assert(!hasVar(c1,v0));
            if (!hasVar(c0,v1)) {
              // swap
              pivots[j] = p1; pivots[j+1] = p0;
              antecedents[j+1] = ant1; antecedents[j+2] = ant0;
              nSwap++; nLocalSwaps++;
            }
          }
        }
      }
      if (nLocalSwaps==0) break;
    }
    return (nSwap);
    
  }
  int proofSwapResolutions(ProofPdt& proofPdt) {
    int totSwap=0;
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      if(proofPdt.resNodes[i].isOriginal()) continue;
      totSwap += proofNodeSwapResolutions(proofPdt,i,false);
    }
    return totSwap;
  }

  //NB: call only after replay
  static int proofNodeRestructProof(ProofPdt& proofPdt,
      int i, vec<lbool>& mark) {
    assert(i>=0 && i< proofPdt.resNodes.size());
    assert (!proofPdt.resNodes[i].isOriginal());
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;
    vec<Lit>& pivots = resNodes[i].pivots;
    vec<int>& antecedents = resNodes[i].antecedents;

    bool ok=true;
    vec<Lit>& c = resNodes[i].resolvents;
    int aId0 = resNodes[i].antecedents[0];
    vec<Lit> res;
    resNodes[aId0].resolvents.copyTo(res);

    // start
    proofResolveStart(res, mark);

    int nRemoved=0;    
    vec<bool> removed(pivots.size(),false);
    int start_i = 0;
    for(int j=0; j<pivots.size(); j++) {
      Lit p = pivots[j];
      Var v = var(p);
      int aId = antecedents[j+1];
      assert(aId>=0 && aId<i);

      vec<Lit>& other = resNodes[aId].resolvents;
      bool mainHas_p = mark[v]!=l_Undef;
      assert(!mainHas_p || (mark[v] == (sign(p)?l_False:l_True)));
      bool otherHas_p = clauseHasLit(other,~p);
      if (mainHas_p && otherHas_p) {
        // resolve
        proofResolveClauses(res, other, p, mark);
      }
      else if (!mainHas_p && otherHas_p) {
        // keep main - remove other branch
        removed[j] = true;
        nRemoved++;
      }
      else if (mainHas_p && !otherHas_p) {
        // take other branch - remove main
        proofResolveClearMarkNoCheck (mark,res);
        other.copyTo(res);
        proofResolveStart(res, mark);
        antecedents[0] = aId;
        start_i = j+1; // skip up to here
        nRemoved = j+1;
      }
      else {
        // none has p - so far keep main
        removed[j] = true;
        nRemoved++;
      }
    }
    if (nRemoved>0) {
      int j=0;
      for (int i=start_i; i<pivots.size(); i++) {
        if (!removed[i]) {
          antecedents[j+1] = antecedents[i+1];
          pivots[j] = pivots[i];
          j++;
        }
      }
      antecedents.shrink(nRemoved);
      pivots.shrink(nRemoved);
    }

    proofResolveKeepAndClearMark (mark,res);
    resNodes[i].resolvents.clear();
    res.copyTo(resNodes[i].resolvents);
    //    assert(i==resNodes.size()-1 || res.size()>0);
    //    assert(pivots.size()>0);
    return (nRemoved);
  }

  static int proofRestructProof(ProofPdt& proofPdt, int nVars) {
    int totRemoved=0;
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;
    vec<lbool> mark(nVars,l_Undef);
    for(int i = 0; i < resNodes.size(); i++){
      if(resNodes[i].isOriginal()) continue;
      //      printf("restruct: %d/%d\n", i, resNodes.size());
      totRemoved += proofNodeRestructProof(proofPdt,i,mark);
      if (i<(resNodes.size()-1) &&
          (resNodes[i].resolvents.size()==0)) {
        int toRemove = resNodes.size()-i-1;
        totRemoved += toRemove;
        resNodes.shrink(toRemove);        
        break;
      }
    }
    return totRemoved;
  }

  void Solver::proofEndAClauses(void) {
    assert(log_proof);
    assert(proofPdt.status==proofPdt.undef);
    
    proofPdt.nSolverACr = clauses.size()==0?-1:clauses.last();
  }

  //NB: call only after replay
  void Solver::proofClassifyNodes(int nSolverAClauses,
                                  bool enSimplify){
    //Sanity check
    assert(proofPdt.resNodes.size() >= 0);
    assert(log_proof);
    assert(proofPdt.status==proofPdt.undef);

    if (nSolverAClauses>0) {
      assert(proofPdt.nSolverACr==0);
      proofPdt.nSolverAClauses = nSolverAClauses;
      proofPdt.nSolverACr = clauses[nSolverAClauses-1];
    }
    unsigned int nACr = proofPdt.nSolverACr;
    //Load original core clauses
    //    printf("CORE CLAUSES\n");

    static int nRemoved;
    if (enSimplify) {
      nRemoved = proofCompactResolvents();
      if (nRemoved>0) {
        if (proofPdt.verbosity()>0) {
          printf("RES PROOF REMOVED %d redundant  literals\n",
                 nRemoved);
          printf("REMOVED: %d\n", nRemoved);
        }
        nRemoved = proofRestructProof(proofPdt, nVars());
      }
      if (pdtChecks) proofCheck();
    }
    if (enSimplify) {
      nRemoved = proofRecyclePivotsReduction();
      if (proofPdt.verbosity()>0)
        printf("RES PROOF RECYCL PIV. REMOVED %d resolutions\n",
             nRemoved);
      nRemoved = proofRestructProof(proofPdt, nVars());
      if (proofPdt.verbosity()>0)
        printf("RESTRUCT PROOF REMOVED %d redundant resolutions\n",
             nRemoved);
      //    nRemoved = proofCompactResolvents();
      //printf("RES PROOF REMOVED %d redundant  literals\n",
      //       nRemoved);

    }
    if (pdtChecks) proofCheck();
    
    proofPdt.status = proofPdt.labeled;
    int nResA=0, nResB=0, nRes=0, nA=0, nB=0;
    static int i;
    for(i = 0; i < proofPdt.resNodes.size(); i++){
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      if(proofPdt.resNodes[i].isOriginal()){
        CRef cr;
        if (c.size()==1) {
          cr = getReason(var(c[0]));
          assert(cr != CRef_Undef);
          proofPdt.resNodes[i].setResClauseId(cr);
        }
        else 
          cr = proofPdt.resNodes[i].resClauseId;
        assert(cr!=CRef_Undef);
        if (cr>nACr) {
          proofPdt.resNodes[i].setCode(proof_rootB);
          nB++;
        }
        else {
          proofPdt.resNodes[i].setCode(proof_rootA);
          nA++;
        }
      }
      else {
        proofCode code = proof_undef;
        bool isA = true;
        bool isB = true;
        for(int j=0; j < proofPdt.resNodes[i].antecedents.size(); j++){
          int antecedentId = proofPdt.resNodes[i].antecedents[j];
          assert (antecedentId>=0 && antecedentId<proofPdt.resNodes.size());
          proofCode code_i = proofPdt.resNodes[antecedentId].getCode();
          switch (code_i) {
          case proof_rootA:
          case proof_resA:
            isB = false;
            break;
          case proof_rootB:
          case proof_resB:
            isA = false;
            break;
          case proof_root:
          case proof_res:
            isA = isB = false;
            break;
          case proof_undef:
          default:
            assert(0); // problem - resolution node not coded
          }
          if (!isA && !isB) break;
        }
        assert(!isA || !isB);
        if (!proofPdt.solverUndef && i == proofPdt.resNodes.size()-1)
            isA = isB = 0;
        if (isA) code = proof_resA;
        else if (isB) code = proof_resB;
        else code = proof_res;
        if (code==proof_resA) nResA++;
        if (code==proof_resB) nResB++;
        if (code==proof_res)
          nRes++;
        proofPdt.resNodes[i].setCode(code);
      }
    }

    if (proofPdt.verbosity()>0)
      printf("RES PROOF PROCESSED: %d A, %d B, %d resA, %d resB, %d res (tot: %d)\n",
           nA, nB, nResA, nResB, nRes, proofPdt.resNodes.size());
    proofPdt.nANodes = nA;
    proofPdt.nBNodes = nB;
    proofPdt.nResNodes = nResA + nResB + nRes;
    proofPdt.nRedNodes = proofPdt.resNodes.size() -
      (nA + nB + nResA + nResB + nRes);

    if (pdtChecks) proofCheck();
    resNodesReduceCore(proofPdt);
    if (pdtChecks) proofCheck();

    //printProof(NULL,NULL,1);
    proofCompactNodes();
    //printProof(NULL,NULL,1);

    proofSetupVars();

    static int enMoveResolutions = 1;
    if (enMoveResolutions) { 
      int nSwap = proofSwapResolutions(proofPdt);
      if (proofPdt.verbosity()>0)
        printf("RES PROOF SWAPPED %d resolution vars\n",
             nSwap);
    }

    resNodesStats(proofPdt);
    if (pdtChecks) proofCheck();
    
  }

  //NB: call only after replay
  void Solver::proofCompactNodes(void){

    //Sanity check
    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);
    assert(proofPdt.status==proofPdt.labeled);
    int nNodes = proofPdt.resNodes.size();
    vec<int> remap(nNodes,-1);
    int nA=0, nB=0;
    //Load original core clauses
    //    printf("CORE CLAUSES\n");
    int i, iNew;
    for(i = iNew = 0; i < nNodes; i++){
      proofCode code_i = proofPdt.resNodes[i].getCode();
      if (code_i==proof_undef || code_i==proof_redundant) {
        continue;
      }
      if (code_i!=proof_rootA && code_i!=proof_rootB) {
        if (proofPdt.resNodes[i].pivots.size()==0) {
          // redundant node - redirect
          int aId = proofPdt.resNodes[i].antecedents[0];
          remap[i] = remap[aId];
          proofPdt.resNodes[i].code = proof_redundant;
          proofPdt.nRedNodes++;
          proofPdt.nResNodes--;
          continue;
        }
        else {
          for(int j=0;
              j < proofPdt.resNodes[i].antecedents.size(); j++){
            int aId = proofPdt.resNodes[i].antecedents[j];
            assert(remap[aId]>=0&& remap[aId]<iNew);
            proofPdt.resNodes[i].antecedents[j] = remap[aId];
          }
        }
      }
      if (code_i == proof_rootA) nA++;
      if (code_i == proof_rootB) nB++;
      if (i>iNew) {
        proofPdt.resNodes[iNew] = proofPdt.resNodes[i];
      }
      remap[i] = iNew++;
    }
    if (proofPdt.resNodes.size()>iNew) {
      proofPdt.resNodes.shrink(nNodes-iNew);
    }
    assert(nA == proofPdt.nANodes);
    assert(nB == proofPdt.nBNodes);
    if (proofPdt.verbosity()>0)
      printf("RES PROOF compacted from %d to %d nodes\n",
           nNodes, iNew);
    assert(proofPdt.nRedNodes == (nNodes-iNew));
    proofPdt.nRedNodes = 0;
    nNodes = iNew;

  }

  //NB: call only after replay
  void Solver::proofCheck(void){
    //Sanity check
    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);
    int nNodes = proofPdt.resNodes.size();
    int nA=0, nB=0, nRed=0, nRes=0;
    //Load original core clauses
    //    printf("CORE CLAUSES\n");
    int i;
    vec<lbool> mark(nVars(),l_Undef);
    for(i = 0; i < nNodes; i++){
      bool doCheckAnt = true;
      if (proofPdt.status == proofPdt.labeled) {
        proofCode code_i = proofPdt.resNodes[i].getCode();
        assert (code_i!=proof_undef);
        if (code_i==proof_redundant) nRed++;
        if (code_i == proof_resA || code_i == proof_resB
            || code_i == proof_res) nRes++;
        if (code_i == proof_rootA) nA++;
        if (code_i == proof_rootB) nB++;
        if (code_i != proof_res) doCheckAnt = false;
      }
      if (doCheckAnt && !proofNodeCheckAntecedents(i, mark, 0)) {
          assert(0);
      }
    }
    for (int v=0; v<nVars(); v++) {
      assert(mark[v]==l_Undef);
    }
    assert(proofPdt.resNodes.last().resolvents.size() == 0);
    if (proofPdt.status == proofPdt.labeled) {
      assert(nA == proofPdt.nANodes);
      assert(nB == proofPdt.nBNodes);
      assert(nRes == proofPdt.nResNodes);
      assert(nRed == proofPdt.nRedNodes);
      if (proofPdt.verbosity()>0)
        printf("RES PROOF checked A:%d B:%d - res:%d (redundant: %d - tot: %d)\n",
             nA, nB, nRes, nRed, nNodes);
    }
  }

  //NB: call only after replay
  void Solver::proofSave(void){
    //Sanity check
    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);
    assert(proofPdt.status==proofPdt.labeled);
    int nNodes = proofPdt.resNodes.size();
    //Load original core clauses
    //    printf("CORE CLAUSES\n");
    int i;
    for(i = 0; i < nNodes; i++){
      proofCode code_i = proofPdt.resNodes[i].getCode();
      assert (code_i!=proof_undef);
      proofPdt.resNodes[i].setSavedCode(code_i);
    }
    proofPdt.isAvar.copyTo(proofPdt.savedIsAvar);
    proofPdt.isGlobal.copyTo(proofPdt.savedIsGlobal);
    proofPdt.saved = true;
  }

  static void myLoadClause (Solver *S, vec<Lit>& c) {
    vec<Lit> lits;
    for (int j = 0; j < c.size(); j++) {
      Lit l = c[j];
      lits.push(l);
    }
    S->addClause(lits);
  }

  int Solver::copyAllClausesToSolverIfHaveNoVars (Solver *S,
				     vec<bool>& noVars) {
    vec<Lit> lits;
    int nLoad = 0;

    while (S->nVars() < nVars())
      S->newVar();
    
    for(int i = 0; i<clauses.size(); i++){
      Clause& cl = ca[clauses[i]];
      bool doLoad = true;
      lits.clear();
      for(int j=0; j<cl.size(); ++j){
	Var v = var(cl[j]);
	Lit l = cl[j];
	lits.push(l);
	if (noVars[var(l)]) doLoad=false;
      }
      if (doLoad || rand()%100 != 0) {
	S->addClause(lits);
	nLoad++;
      }
    }

    for(int i = 0; i<learnts.size(); i++){
      Clause& cl = ca[learnts[i]];
      lits.clear();
      for(int j=0; j<cl.size(); ++j){
	Lit l = cl[j];
	lits.push(l);
      }
      S->addClause(lits);
      nLoad++;
    }

    return nLoad;
  }

  //NB: call only after replay
  void Solver::getPartialNewProof(vec< vec<Lit> >& Clauses,
                               int& nAClauses,
                               int& nAClausesCore,
                               int& nASolverClausesCore,
                               Solver *auxS, 
                               float extraRatio){

    //Sanity check
    assert(nAClauses>0);
    assert(auxS!=NULL);
    
    //Sanity check
    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);
    
    Clauses.clear();

    //Initialize solver vars
    while (auxS->nVars() < nVars())
      auxS->newVar();

    int nA=0, nB=0, nAdded=0, nExtra;
    /* A roots */
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      proofCode code = proofPdt.resNodes[i].getCode();
      if (code != proof_rootA && code != proof_resA) continue;
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      myLoadClause(auxS,c);
      Clauses.push();
      c.copyTo(Clauses.last());
      nA++;
    }
    /* B roots */
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      proofCode code = proofPdt.resNodes[i].getCode();
      if (code != proof_rootB && code != proof_resB) continue;
      nB++;
    }
    nExtra = extraRatio * (proofPdt.resNodes.size()-(nA+nB));
    /* A roots by resolutions */
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      proofCode code = proofPdt.resNodes[i].getCode();
      if (code != proof_res) continue;
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      myLoadClause(auxS,c);
      Clauses.push();
      c.copyTo(Clauses.last());
      nAdded++;
      if (nAdded >= nExtra) break;
    }

    nAClausesCore = nA+nAdded;
    nASolverClausesCore=auxS->nClauses();
    assert(nAClausesCore==Clauses.size());
    
    // now append B clauses
    /* B roots */
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      proofCode code = proofPdt.resNodes[i].getCode();
      if (code != proof_rootB && code != proof_resB) continue;
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      myLoadClause(auxS,c);
      Clauses.push();
      c.copyTo(Clauses.last());
    }

  }

  //NB: call only after replay
  void Solver::printProofNode(int i, int verbosity) {
    char *names[]={"root", "rootA", "rootB",
                   "res", "resA", "resB", "redundant", "undef"};
    vec<bool>& isAvar = proofPdt.isAvar;
    vec<bool>& isGlobal = proofPdt.isGlobal;
    assert(i>=0 && i< proofPdt.resNodes.size());

    printf("RES[%d] %s ", i, names[(int)proofPdt.resNodes[i].code]);
    if (verbosity>1)
      for(int j=0; j < proofPdt.resNodes[i].antecedents.size(); j++){
        int aId = proofPdt.resNodes[i].antecedents[j];
        if (j>0) {
          int v = var(proofPdt.resNodes[i].pivots[j-1]);
          char ab = '-', op = '&';
          if (isAvar.size()>v) {
            ab = 'B';
            if (isGlobal[v]) ab = 'G';
            else if (isAvar[v]) ab = 'A';
          }
          if (ab=='A') op='|';
          printf(" (%c<%c>:%d)",ab,op,v);
        }
        printf(" %d", aId);
      }
    if (verbosity>0) {
      printf(" - RESOLVENT: ");
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      for (int j=0; j<c.size(); j++) {
        Lit l = c[j];
        printf("%s%d ", sign(l)?"-":"", var(l));
      }
    }
    printf("\n");
  }
  
  //NB: call only after replay
  static int proofNodeMarkAntecedentVar(ProofPdt& proofPdt,
      int i, vec<bool>& mark, bool val) {
    assert(i>=0 && i< proofPdt.resNodes.size());
    int nFound=0;    
    for(int j=0; j<proofPdt.resNodes[i].antecedents.size(); j++) {
      int aId = proofPdt.resNodes[i].antecedents[j];
      assert(aId>=0 && aId<i);
      vec<Lit>& c = proofPdt.resNodes[aId].resolvents;
      for (int jj=0; jj<c.size(); jj++) {
        Var v_jj = var(c[jj]);
        if (mark[v_jj]!=val) {
          mark[v_jj] = val;
          nFound++;
        }
      }
    }
    return nFound;
  }


  //NB: call only after replay
  int Solver::proofNodeFindAntecedentsWithVar(int i, int v, int verbosity) {
    assert(i>=0 && i< proofPdt.resNodes.size());
    int nFound = 0;
    if (verbosity>0)
      printf("looking for antecendents with var %d\n", v);
    for(int j=0; j < proofPdt.resNodes[i].antecedents.size(); j++){
      int aId = proofPdt.resNodes[i].antecedents[j];
      assert(aId>=0 && aId<i);
      vec<Lit>& c = proofPdt.resNodes[aId].resolvents;
      for (int jj=0; jj<c.size(); jj++) {
        Var v_jj = var(c[jj]);
        if (v_jj == v) {
          if (verbosity>0)
            printf("antecendent[%d] %d has var - lit[%d]\n",
                 j, aId, jj);
          nFound++;
          if (verbosity<0) break;
        }
      }
    }
    if (verbosity>0)
      printf("\n");
    return nFound;
  }

  //NB: call only after replay
  int Solver::proofNodeCheckAntecedents(int i,
                                        vec<lbool>& mark,
                                        int verbosity) {
    vec<ResolutionNode>& resNodes = proofPdt.resNodes;
    assert(i>=0 && i<resNodes.size());
    bool ok=true;
    if (resNodes[i].pivots.size()==0) return 1;
    // light weigth check
    vec<Lit>& c = resNodes[i].resolvents;
    for (int j=0; j<c.size(); j++) {
      Var v = var(c[j]);
      if (proofNodeFindAntecedentsWithVar(i, v,
               verbosity>0?verbosity-1:0)<=0) {
        ok=false;
        if (verbosity) {
          printf("var: %d was not found\n", v);
        }
      }
    }
   
    // resolutions
    int aId0 = resNodes[i].antecedents[0];
    vec<Lit> res;
    vec<Lit>& pivots = resNodes[i].pivots;
    vec<int>& antecedents = resNodes[i].antecedents;
    res.clear();
    resNodes[aId0].resolvents.copyTo(res);

    // start
    proofResolveStart(res, mark);
    for(int j=0; j<pivots.size(); j++) {
      Lit p = pivots[j];
      Var v = var(p);
      int aId = antecedents[j+1];
      assert(aId>=0 && aId<i);

      vec<Lit>& other = resNodes[aId].resolvents;
      bool mainHas_p = mark[v]!=l_Undef;
      assert(!mainHas_p || (mark[v] == (sign(p)?l_False:l_True)));
      bool otherHas_p = clauseHasLit(other,~p);
      assert(mainHas_p && otherHas_p);
      proofResolveClauses(res, other, p, mark);
    }
    for (int j=0; j<c.size(); j++) {
      Var v = var(c[j]);
      assert(mark[v]!=l_Undef);
      mark[v]=l_Undef;
    }

    assert(ok);
    return (ok==true);
  }
  
  //NB: call only after replay
  static int proofNodeCompactResolvents(ProofPdt& proofPdt,
      int i, vec<bool>& mark) {
    assert(i>=0 && i< proofPdt.resNodes.size());
    bool ok=true;
    int nRemoved = 0;
    vec<Lit>& c = proofPdt.resNodes[i].resolvents;
    vec<Lit> cNew;
    int nMark1 = proofNodeMarkAntecedentVar(proofPdt,i,mark,true);
    for (int j=0; j<c.size(); j++) {
      Var v = var(c[j]);
      if (!mark[v]) {
        ok=false;
        nRemoved++;
      }
      else {
        cNew.push(c[j]);
      }
    }
    if (!ok) {
      cNew.copyTo(c);
    }
    int nMark0 = proofNodeMarkAntecedentVar(proofPdt,i,mark,false);
    assert(nMark1==nMark0);
    //assert(c.size()>0 || i==proofPdt.resNodes.size()-1);
    if (c.size()==0 && i<proofPdt.resNodes.size()-1) {
      printf("ooouck\n");
      assert(0);
    }
    return (nRemoved);
  }


  int Solver::proofCompactResolvents(void) {
    int totRemoved=0;
    vec<bool> mark(nVars(),false);
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      if(proofPdt.resNodes[i].isOriginal()) continue;
      totRemoved += proofNodeCompactResolvents(proofPdt,i,mark);
    }
    return totRemoved;
  }

  static int proofNodeRecyclePivotsReduction(ProofPdt& proofPdt,
    int id, vec<lbool>& mark, vec<bool>& done, vec<int>& ref
  ) {
    static bool enRed = true; static int nCalls=0;

    assert(id>=0 && id< proofPdt.resNodes.size());
    if (done[id]) return 0;
    done[id] = true;
    if (proofPdt.resNodes[id].isOriginal()) return 0;
    int myNCalls = nCalls++;
    
    bool ok=true;
    int nRemoved = 0, recRemoved = 0;
    vec<Lit>& res = proofPdt.resNodes[id].resolvents;
    vec<Var> saved;
    for (int j=0; j<res.size(); j++) {
      Var v = var(res[j]);
      if (mark[v]==l_Undef) {
        mark[v] = sign(res[j]) ? l_False:l_True;
        saved.push(v);
      }
    }
    vec<Lit>& pivots = proofPdt.resNodes[id].pivots;
    vec<int>& antecedents = proofPdt.resNodes[id].antecedents;
    // down iteration to find redundancies

    vec<bool> removed(pivots.size(),false);
    int i;
    for(i=pivots.size()-1; i>=0; i--) {
      Lit l = pivots[i];
      Var v = var(l);
      if (mark[v]==l_Undef) {
        mark[v] = sign(l) ? l_False:l_True;
        saved.push(v);
        // try recurring on antecedent
        int antId = antecedents[i+1];
        assert(antId<id);
        if (ref[antId]==1) {
          mark[v] = mark[v]==l_False ? l_True:l_False;
          recRemoved +=
            proofNodeRecyclePivotsReduction(proofPdt,antId,
                                          mark,done,ref);
          mark[v] = mark[v]==l_False ? l_True:l_False;
        }
      }
      else if (enRed && i>0) {
        bool keepChain = (mark[v]==l_True && !sign(l)) ||
          (mark[v]==l_False && sign(l));
        if (keepChain) {
          removed[i] = true;
          nRemoved++;
        }
        else if (1) {
          // remove all remaining chain
          antecedents[0]=antecedents[i+1];
          for  (int ii=i; ii>=0; ii--) {
            removed[ii] = true;
            nRemoved++;
          }
          break;
        }
      }
    }
    if (nRemoved>0) {
      int j=0;
      for (int i=0;i<pivots.size(); i++) {
        if (!removed[i]) {
          antecedents[j+1] = antecedents[i+1];
          pivots[j] = pivots[i];
          j++;
        }
      }
      antecedents.shrink(nRemoved);
      pivots.shrink(nRemoved);
    }

    for (int i=0; i<saved.size(); i++) {
      int v = saved[i];
      mark[v] = l_Undef;
    }
    if (0 && nRemoved>0) {
      if (proofPdt.verbosity()>0)
        printf("REC PIV (nCalls: %d): removed: %d\n",
             myNCalls, nRemoved);
    }
    
    return (nRemoved+recRemoved);
  }

  int Solver::proofRecyclePivotsReduction(void) {
    int totRemoved=0;
    vec<lbool> mark(nVars(),l_Undef);
    vec<bool> done(proofPdt.resNodes.size(),false);
    vec<int> ref(proofPdt.resNodes.size(),0);
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      if(proofPdt.resNodes[i].isOriginal()) continue;
      for(int j=0; j<proofPdt.resNodes[i].antecedents.size(); j++){
        int antId = proofPdt.resNodes[i].antecedents[j];
        ref[antId]++;
      }
    }
    for(int i = proofPdt.resNodes.size()-1; i>=0; i--){
      if(proofPdt.resNodes[i].isOriginal()) continue;
      totRemoved += proofNodeRecyclePivotsReduction(proofPdt,
                      i,mark,done,ref);
    }
    return totRemoved;
  }


  //NB: call only after replay
  void Solver::printProof(int verbosity) {
    char *names[]={"root", "rootA", "rootB",
                   "res", "resA", "resB", "redundant", "undef"};
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      printProofNode(i, verbosity);
    }
  }

  //NB: call only after replay
  void Solver::printSolverProof(int verbosity) {
    for(int i = 0; i < proof.size(); i++){
      CRef cr = proof [i];
      assert (cr != CRef_Undef);
      Clause &c = ca [cr];
      if (c.learnt() && verbosity == 0) continue;
      printf("[%d] core: %d - mark: %d - learned: %d: ",
             i, c.core(), c.mark(), c.learnt());
      for (int j=0; j<c.size(); j++) {
        Lit l = c[j];
        printf("%s%d ", sign(l)?"-":"", var(l));
      }
      printf("\n");
    }
  }

  void Solver::remapProofVars(vec<int>& remap) {
    remap.copyTo(proofPdt.remapVars);
    for(int j = 0; j < proofPdt.resNodes.size(); j++){
      vec<Lit>& c = proofPdt.resNodes[j].resolvents;
      for (int i=0; i<c.size(); i++) {
        Var v = var(c[i]);
        Var v1 = remap[v];
        if (v1 != var_Undef) {
          Lit l = mkLit(v1,sign(c[i]));
          c[i] = l;
        }
      }
      vec<Lit>& p = proofPdt.resNodes[j].pivots;
      for (int i=0; i<p.size(); i++) {
        Var v = var(p[i]);
        Var v1 = remap[v];
        if (v1 != var_Undef) {
          Lit l = mkLit(v1,sign(p[i]));
          p[i] = l;
        }
      }
    }
  }

  void Solver::remapClauses(vec<int>& remap,
                            vec< vec<Lit> >& clauses) {
    for(int j = 0; j < clauses.size(); j++){
      vec<Lit>& c = clauses[j];
      for (int i=0; i<c.size(); i++) {
        Var v = var(c[i]);
        Var v1 = remap[v];
        if (v1 != var_Undef) {
          Lit l = mkLit(v1,sign(c[i]));
          c[i] = l;
        }
      }
    }
  }
    
  //NB: call only after replay
  void Solver::getProofClausesAfterMove(vec< vec<Lit> >& ClA,
                                       vec< vec<Lit> >& ClB,
                                       vec< vec<Lit> >& ClAorig,
                                       vec< vec<Lit> >& ClAfromB,
                                       vec< vec<Lit> >& ClAfromRes
  ){
    assert(proofPdt.saved);
    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);

    ClA.clear();
    ClB.clear();
    ClAorig.clear();
    ClAfromB.clear();
    ClAfromRes.clear();

    //Initialize id map
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      proofCode code = proofPdt.resNodes[i].getCode();
      proofCode code0 = proofPdt.resNodes[i].getSavedCode();
      assert(code!=proof_resA && code!=proof_resB);
      if (code0 == proof_rootA) {
        ClA.push();
        c.copyTo(ClA.last());
      }
      if (code == proof_rootA && code0 == proof_rootA) {
        ClAorig.push();
        c.copyTo(ClAorig.last());
      }
      if (code == proof_rootA && code0 == proof_rootB) {
        ClAfromB.push();
        c.copyTo(ClAfromB.last());
      }
      if (code == proof_rootA && code0 == proof_res) {
        ClAfromRes.push();
        c.copyTo(ClAfromRes.last());
      }
      if (code == proof_rootA && code0 == proof_resA) {
        assert(0);
      }
      if (code == proof_rootA && code0 == proof_resB) {
        assert(0);
      }
      if (code == proof_rootB) {
        ClB.push();
        c.copyTo(ClB.last());
      }

    }
  }

  //NB: call only after replay
  void Solver::genProofAuxClausesFromBres(vec< vec<Lit> >& ClBaux,
                                          vec<Lit>& actLits,
                                          vec<Lit>& actClause,
                                          float minSizeRatio
                                          ){
    assert(proofPdt.saved);
    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);

    ClBaux.clear();
    actLits.clear();
    actClause.clear();
    int maxSize=0, minSize;
    int newVar = nVars();
    int avgLits=0;    
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      proofCode code = proofPdt.resNodes[i].getCode();
      if (code == proof_res) {
        if (c.size() > maxSize)
          maxSize = c.size();
      }
    }
    minSize = maxSize*minSizeRatio;
    if (minSize<100) minSize=100;

    actLits.push(mkLit(newVar,false));
    actClause.push(mkLit(newVar,true));
    newVar++;

    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      proofCode code = proofPdt.resNodes[i].getCode();
      if (code != proof_res) continue;
      if (c.size() < minSize) continue;
      //      actLits.push(mkLit(newVar,false));
      actClause.push(mkLit(newVar,false));
#if 0
      vec<Lit> cNew;
      cNew.push(mkLit(newVar,true));
      for (int j=0; j<c.size(); j++) {
        if (proofPdt.Bvar(var(c[j]))) {
          if (rand()%100 > 50)
            cNew.push(c[j]);
        }
      }
      avgLits += cNew.size();
      ClBaux.push();
      cNew.copyTo(ClBaux.last());
#else
      for (int j=0; j<c.size(); j++) {
        if (proofPdt.Bvar(var(c[j])) ||
            proofPdt.Global(var(c[j]))) {
          if (1 || (rand()%100 > 20)) {
            vec<Lit> cNew;
            cNew.push(mkLit(newVar,true));
            cNew.push(~c[j]);
            avgLits += cNew.size();
            ClBaux.push();
            cNew.copyTo(ClBaux.last());
          }
        }
      }
#endif
      newVar++;
    }
    if (ClBaux.size()>0)
      if (proofPdt.verbosity()>0)
        printf("GENERATED %d AUX clauses of avg size: %d\n",
           ClBaux.size(), avgLits/ClBaux.size());

  }

  //NB: call only after replay
  void Solver::genProofActLitsFromBres(vec<Lit>& actLits, int nLits) {
    assert(proofPdt.saved);
    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);

    vec<int> count(2*nVars(),0);
    actLits.clear();
    
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      proofCode code = proofPdt.resNodes[i].getCode();
      if (code != proof_res) continue;
      for (int j=0; j<c.size(); j++) {
        if (proofPdt.Bvar(var(c[j])) ||
            proofPdt.Global(var(c[j]))) {
          count[toInt(c[j])]++;
        }
      }
    }

    int m0=-1, m1=-1; 
    int useComplLit = 0;
    for (int k=0; k<nLits; k++) {
      // find max
      int maxv = -1, maxD=0, d;
      for (int v=0; v<nVars(); v++) {
        d = abs(count[2*v]-count[2*v+1]);
        if (maxv<0 || d>maxD) {
          maxD = d; maxv = v;
        }
      }
      m1 = maxD;
      if (m0<0) m0 = maxD;
      Lit l = count[2*maxv] < count[2*maxv+1] ?
                              mkLit(maxv,useComplLit):
                              mkLit(maxv,!useComplLit);
      count[2*maxv] = count[2*maxv+1] = 0;
      actLits.push(l);
    }

    if (proofPdt.verbosity()>0)
      printf("GENERATED %d ACT lits with activity %d-%d\n",
           actLits.size(), m1, m0);

  }

  //NB: call only after replay
  int Solver::getProof(vec< vec<Lit> >& Clauses,
		       int& nAClausesCore, 
		       vec< vec<int> >& proofNodes,
		       vec< vec<Lit> >& pivots,
		       vec< vec<Lit> > *topResClP
    ){

    static int myprint=0;
    //Sanity check
    
    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);
    int nResB=0, nRed=0, nResA=0, nRes=0, nA=0, nB=0;    
    //Initialize id map
    vec<int> idMap;
    idMap.growTo(proofPdt.resNodes.size(), -1);
    
    nAClausesCore = 0;
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      proofCode code = proofPdt.resNodes[i].getCode();
      assert(code!=proof_root);
      if (code == proof_redundant) nRed++;
      if (code != proof_rootA && code != proof_resA) continue;
      nA++; if (code == proof_resA) nResA++;
      proofPdt.resNodes[i].setCode(proof_rootA);
      vec<Lit>& c = proofPdt.resNodes[i].resolvents;
      nAClausesCore++;
      int extId = Clauses.size();
      idMap[i] = extId;

      //Copy original clause
      Clauses.push();
      c.copyTo(Clauses[extId]);
    }
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      proofCode code = proofPdt.resNodes[i].getCode();
      assert(code!=proof_root);
      if (code != proof_rootB && code != proof_resB) continue;
      nB++; if (code == proof_resB) nResB++;

      vec<Lit>& c = proofPdt.resNodes[i].resolvents;

      int extId = Clauses.size();
      idMap[i] = extId;

      //Copy original clause
      Clauses.push();
      c.copyTo(Clauses[extId]);
    }

    if (proofPdt.verbosity()>0)
      printf("resB: %d\n", nResB);
    //Initialize proofNodes and pivots
    proofNodes.growTo(Clauses.size());
    pivots.growTo(Clauses.size());

    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      proofCode code = proofPdt.resNodes[i].getCode();

      //      if (code!=proof_resA && code!=proof_resB &&
      //    code!=proof_res) continue;
      if (code!=proof_res) continue;
      nRes++;
      int extId = proofNodes.size();
      idMap[i] = extId;
      
      //Convert antecedents id to external
      proofNodes.push();
      if (myprint) {
        printf("L: %d - extid: %d - code %d\n", i, extId, code);
      }

      vec<int> extAntecedents;
      for(int j=0; j < proofPdt.resNodes[i].antecedents.size(); j++){
        int antecedentId = proofPdt.resNodes[i].antecedents[j];
        assert(idMap[antecedentId] != -1);
        extAntecedents.push(idMap[antecedentId]);
        if (myprint) {
          printf("ant: %d - %d\n", antecedentId,idMap[antecedentId]);
        }
      }
      if (myprint) {
        printf("AS: %d\n", extAntecedents.size());
      }
      extAntecedents.copyTo(proofNodes[extId]);
      
      //Copy pivots
      pivots.push();
      proofPdt.resNodes[i].pivots.copyTo(pivots[extId]);

      if (topResClP!=NULL) {
	vec<Lit>& c = proofPdt.resNodes[i].resolvents;

	if (topResClP->size()<=extId) {
	  topResClP->growTo(extId+1);
	}
	c.copyTo((*topResClP)[extId]);
      }

    }

    if (!proofPdt.solverUndef) {
      if (proofPdt.resNodes.last().code == proof_resA) {
	// A is unsat
	return 0;
      }
      if (proofPdt.resNodes.last().code == proof_resB) {
	// A is unsat
	return 1;
      }
    }
    
    if (proofPdt.verbosity()>0)
      printf("GET PROOF %d A (%d resA) - %d B (%d resB) - %d res - %d red\n", nA, nResA, nB, nResB, nRes, nRed);
    return -1;    
  }
  
  //NB: call only after replay
  int Solver::getProofClauses(vec< vec<Lit> >& Clauses){

    static int myprint=0;
    //Sanity check
    
    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);


    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      if(proofPdt.resNodes[i].isOriginal()){
        vec<Lit>& c = proofPdt.resNodes[i].resolvents;
        Clauses.push();
        c.copyTo(Clauses.last());
      }
    }
    return Clauses.size();
  }

  int Solver::checkSolverProofClauses(void){

    //Sanity check

    assert(log_proof);
    vec<bool> coveredClause;
    coveredClause.growTo(ca.size(),false);

    for(int i = 0; i < proofPdt.resNodes.size(); i++){
      if(proofPdt.resNodes[i].isOriginal()){
        vec<Lit>& c = proofPdt.resNodes[i].resolvents;
        CRef cr = proofPdt.resNodes[i].resClauseId;
        coveredClause[cr] = true;
      }
    }

    int notCovered = 0;
    for (int i = 0; i < clauses.size (); ++i)
    {
      CRef cr = clauses [i];
      assert (cr != CRef_Undef);
      Clause &c = ca [cr];
      if (!c.core ()) continue;
      if (!coveredClause[cr]) {
        notCovered++;
      }
    }
    return notCovered;
  }


  int Solver::getSolverProofClauses(vec< vec<Lit> >& Clauses,
                                    bool coreOnly){
    //Sanity check

    assert(log_proof);
    Clauses.clear();

    for (int i = 0; i < clauses.size (); ++i)
    {
      CRef cr = clauses [i];
      assert (cr != CRef_Undef);
      Clause &c = ca [cr];
      if (coreOnly && !c.core ()) continue;
      vec<Lit> cv;
      for(int j=0;j<c.size();j++) {
        cv.push(c[j]);
      }
      Clauses.push();
      cv.copyTo(Clauses.last());
    }
    return Clauses.size();
  }

  //NB: call only after replay
  void Solver::getProof1(vec< vec<Lit> >& Clauses,
                int& nAClauses, int& nAClausesCore, 
                vec< vec<int> >& proofNodes,
                vec< vec<Lit> >& pivots,
                vec< vec<Lit> >& topResClauses,
                int partial, Solver *auxS){

    //Sanity check

    assert(proofPdt.resNodes.size() > 0);
    assert(log_proof);
    
    //Initialize id map
    vec<int> idMap;
    idMap.growTo(proofPdt.resNodes.size(), -1);
    
    if (auxS!=NULL) {
      while (auxS->nVars() < nVars())
        auxS->newVar();
    }
    
    //Load original core clauses
    //    printf("CORE CLAUSES\n");
    unsigned int nACr = clauses[nAClauses-1];
    if (nAClauses>0) {
      nAClausesCore = 0;
      for(int i = 0; i < proofPdt.resNodes.size(); i++){
        if(proofPdt.resNodes[i].isOriginal()){
          vec<Lit>& c = proofPdt.resNodes[i].resolvents;
          CRef cr;
          if (c.size()==1) {
            cr = getReason(var(c[0]));
            assert(cr != CRef_Undef);
            proofPdt.resNodes[i].setResClauseId(cr);
          }
          else 
            cr = proofPdt.resNodes[i].resClauseId;
          assert(cr!=CRef_Undef);
          if (cr>=nACr) continue;
          //Assign external id to original clause
          nAClausesCore++;
          int extId = Clauses.size();
          idMap[i] = extId;

          //Copy original clause
          Clauses.push();
          proofPdt.resNodes[i].resolvents.copyTo(Clauses[extId]);
          if (1 && (auxS!=NULL)) {
            vec<Lit> lits;
            for (int j = 0; j < c.size(); j++) {
              Lit l = c[j];
              lits.push(l);
              //      printf("%s%d ", sign(l)?"-":"", var(l));
            }
            //              printf("\n");
            auxS->addClause(lits);
          }
        }
      }
    }
 
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
        if(proofPdt.resNodes[i].isOriginal()){
            //Assign external id to original clause
          vec<Lit>& c = proofPdt.resNodes[i].resolvents;
          if (nAClauses>0) {
            CRef cr = proofPdt.resNodes[i].resClauseId;
            assert(cr!=CRef_Undef);
            if (cr < nACr) continue;
          }
          int extId = Clauses.size();
          idMap[i] = extId;
            
          //Copy original clause
          Clauses.push();
          proofPdt.resNodes[i].resolvents.copyTo(Clauses[extId]);
          if (1 && (auxS!=NULL)) {
            vec<Lit> lits;
            assert(c.size()==1 || proofPdt.resNodes[i].resClauseId != CRef_Undef);
            if (c.size()==1) {
              CRef cr = getReason(var(c[0]));
              assert(cr != CRef_Undef);
            }
            for (int j = 0; j < c.size(); j++) {
              Lit l = c[j];
              lits.push(l);
              //      printf("%s%d ", sign(l)?"-":"", var(l));
            }
            //              printf("\n");
            auxS->addClause(lits);
          }
        }
    }
    
    //Initialize proofNodes and pivots
    proofNodes.growTo(Clauses.size());
    pivots.growTo(Clauses.size());

    //Load proof nodes and pivots
    //    printf("RESOLUTION CLAUSES\n");
    for(int i = 0; i < proofPdt.resNodes.size(); i++){
        if(proofPdt.resNodes[i].isResolvent()){

            //Assign external id to resolution node
            int extId = proofNodes.size();
            idMap[i] = extId;

            //Convert antecedents id to external
            vec<int> extAntecedents;
            for(int j=0; j < proofPdt.resNodes[i].antecedents.size(); j++){
                int antecedentId = proofPdt.resNodes[i].antecedents[j];
                assert(idMap[antecedentId] != -1);
                extAntecedents.push(idMap[antecedentId]);
            }

            //Copy antecedents
            proofNodes.push();
            extAntecedents.copyTo(proofNodes[extId]);

            //Copy pivots
            pivots.push();
            proofPdt.resNodes[i].pivots.copyTo(pivots[extId]);

	    CRef id = proofPdt.resNodes[i].getResClauseId();
	    if (id != CRef_Undef) {
	      Clause &c = ca[id];
	      vec<Lit> res;
	      for(int i=0;i<c.size();i++) {
		res.push(c[i]);
	      }

	      if (topResClauses.size()<=extId) {
		topResClauses.growTo(extId+1);
	      }
	      res.copyTo(topResClauses[extId]);
	    }

        }
    }

    if (auxS!=NULL) {
      int nOriginal = auxS->nClauses();
      int nRes = proofPdt.resNodes.size()-nOriginal;
      float loadFactor = 0.5;
      int nLoad = nRes*loadFactor;
      //      int sat = auxS->solve();
      for(int i = 0; i < proofPdt.resNodes.size(); i++){
        if(proofPdt.resNodes[i].isResolvent()){
          vec<Lit>& c = proofPdt.resNodes[i].resolvents;
          vec<Lit> lits;
          for (int j = 0; j < c.size(); j++) {
            Lit l = c[j];
            lits.push(l);
            //            printf("%s%d ", sign(l)?"-":"", var(l));
          }
          //          printf("\n");
          auxS->addClause(lits);
          if (nLoad-- <= 0) break;
        }
      }
      //      sat = auxS->solve();
      //printf("SAT: %d\n", sat);
    }

  }

}




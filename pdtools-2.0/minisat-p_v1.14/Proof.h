/*****************************************************************************************[Proof.h]
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

#ifndef Proof_h
#define Proof_h

#include "SolverTypes.h"
#include "File.h"




//=================================================================================================
// A "listner" for the proof. Proof events will be passed onto (online mode) or replayed to
// (offline mode) this class.  Each call to 'root()' or 'chain()' produces a new clause. The first
// clause has ID 0, the next 1 and so on. These are the IDs passed to 'chain()'s 'cs' parameter.
//
struct ProofTraverser {
  
   
  //PDT HD DFS visit
  enum {MEM_MODE,HD_MODE,P_MODE} mode; 
  vec<int64> refNodeVector; // contains pointers to the file nodes  
  cchar* fileName;
  int hdmode;
  //----------------
  virtual void root   (const vec<Lit>& c) {}
  virtual void root1   (const int id) {}
  virtual void chain  (const vec<ClauseId>& cs, const vec<Lit>& xs) {}
  virtual void chain1  (const int id) {}
  virtual void core  () {}
  virtual void genitp  () {}
  virtual void deleted(ClauseId c) {}
  virtual void done   () {}
  virtual ~ProofTraverser() {}
};


class TempFiles {
  vec<char*> files;      // For clean-up purposed on abnormal exit.

public:
  ~TempFiles()
  {
    for (int i = 0; i < files.size(); i++) {
      remove(files[i]);
      xfree(files[i]);
    }
    //printf("Didn't delete:\n  %s\n", files[i]);
  }
  
  void deleteTemps() {
    for (int i = 0; i < files.size(); i++) {
      remove(files[i]);
      xfree(files[i]);
    }
    files.clear();
  }
  
  char *getLocalTempName(char *prefix);

    // Returns a read-only string with the filename of the temporary
    // file. 
    // The pointer can be used to 
    // remove the file (which is otherwise done automatically 
    // upon program exit).
    //

  char* open(File& fp)
  {
    char* name = NULL;
    
    name = getLocalTempName(NULL);
    //    printf("TMPNAME: %s\n", name);
    assert(name != NULL);
    fp.open(name, "wx+");
    if (fp.null()) {
      xfree(name);
    } else {
      files.push(name);
    }
    
    return name;
  }
  
};


class Proof {
    File            fp;
    cchar*          fp_name;
    ClauseId        id_counter;
    ProofTraverser* trav;
    TempFiles       temp_files;       // (should be singleton)
  
    vec<Lit>        clause;
    vec<ClauseId>   chain_id;
    vec<Lit>        chain_var;

public:
    Proof();                        // Offline mode -- proof stored to a file, which can be saved, compressed, and/or traversed.
    Proof(ProofTraverser& t);       // Online mode -- proof will not be stored.

    ClauseId addRoot   (vec<Lit>& clause);
    void     beginChain(ClauseId start);
    void     resolve   (ClauseId next, Lit l);
    ClauseId endChain  ();
    void     deleted   (ClauseId gone);
    ClauseId last      () { assert(id_counter != ClauseId_NULL); return id_counter - 1; }

    void     compress  (Proof& dst, ClauseId goal = ClauseId_NULL);     // 'dst' should be a newly constructed, empty proof.
    bool     save      (cchar* filename);
    void     traverse  (ProofTraverser& trav, ClauseId goal = ClauseId_NULL) ;
    void     deleteTemps();
};


//=================================================================================================
#endif

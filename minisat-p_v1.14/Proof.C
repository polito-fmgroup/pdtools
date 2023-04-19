/*****************************************************************************************[Proof.C]
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

#include "Proof.h"
#include "Sort.h"
#include <pthread.h>

//=================================================================================================

// Temporary files handling:

  char *TempFiles::getLocalTempName(char *prefix) {
    char myfile[255+1];
    char *tmpName = NULL;
    int found = 0;
    char location[] = "/tmp";

    while (!found) {
      struct stat buf;
      // while loop, indeed!
      for (unsigned int i=0; !found ; i++) {
	if (prefix) {
	  snprintf(myfile, 255, "%s/%s_%d_%02u.prf", location, 
		   prefix, (int) getpid(), i);
	} else {
	  //	  snprintf(myfile, 255, "%s/%s_%d_%02u.prf", 
	  //       location, "itpTmp", (int) getpid(), i);
	  snprintf(myfile, 255, "%s/%s_%d_%d_%02u.prf", 
		   location, "itpTmp", (int) getpid(), (int)pthread_self(), i);
	}
	if (stat(myfile, &buf) == -1) {
	  tmpName = strdup(myfile);
	  found = 1;
	}
      }
    }  

    return tmpName;
  }

//=================================================================================================
// Proof logging:


Proof::Proof()
{
    fp_name    = temp_files.open(fp);
    id_counter = 0;
    trav       = NULL;
}


Proof::Proof(ProofTraverser& t)
{
    id_counter = 0;
    trav       = &t;
}


ClauseId Proof::addRoot(vec<Lit>& cl)
{
    cl.copyTo(clause);
    sortUnique(clause);

    if (trav != NULL)
        trav->root(clause);
    if (!fp.null()){
        putUInt(fp, index(clause[0]) << 1);
        for (int i = 1; i < clause.size(); i++)
            putUInt(fp, index(clause[i]) - index(clause[i-1]));
        putUInt(fp, 0);     // (0 is safe terminator since we removed duplicates)
    }

    return id_counter++;
}


void Proof::beginChain(ClauseId start)
{
    assert(start != ClauseId_NULL);
    chain_id .clear();
    chain_var.clear();
    chain_id.push(start);
}


void Proof::resolve(ClauseId next, Lit l)
{
    assert(next != ClauseId_NULL);
    chain_id .push(next);
    chain_var.push(l);
}


ClauseId Proof::endChain()
{
    assert(chain_id.size() == chain_var.size() + 1);
    if (chain_id.size() == 1)
        return chain_id[0];
    else{
        if (trav != NULL)
            trav->chain(chain_id, chain_var);
        if (!fp.null()){
            putUInt(fp, ((id_counter - chain_id[0]) << 1) | 1);
            for (int i = 0; i < chain_var.size(); i++)
              putUInt(fp, index(chain_var[i]) + 1),
                putUInt(fp, id_counter - chain_id[i+1]);
            putUInt(fp, 0);
        }

        return id_counter++;
    }
}


void Proof::deleted(ClauseId gone)
{
    if (trav != NULL)
        trav->deleted(gone);
    if (!fp.null()){
        putUInt(fp, ((id_counter - gone) << 1) | 1);
        putUInt(fp, 0);
    }
}


//=================================================================================================
// Read-back methods:


void Proof::compress(Proof& dst, ClauseId goal)
{
    assert(!fp.null());
    assert(false);  // Not yet!    
}


bool Proof::save(cchar* filename)
{
    assert(!fp.null());

    // Switch to read mode:
    fp.setMode(READ);
    fp.seek(0);

    // Copy file:
    File    out(filename, "wox");
    if (out.null())
        return false;

    while (!fp.eof())
        out.putChar(fp.getChar());

    // Restore write (proof-logging) mode:
    fp.seek(0, SEEK_END);
    fp.setMode(WRITE);
    return true;
}


void Proof::traverse(ProofTraverser& trav, ClauseId goal)
{
  Lit l;
  int totSize = 0;
  //  trav.hdmode=trav.MEM_MODE;
  assert(!fp.null());
  //PDT HD DFS visit -> attribute fileName added to traverser   
  trav.fileName=fp_name;
  // Switch to read mode:
  fp.setMode(READ);
  fp.seek(0);

  // Traverse proof:
  if (goal == ClauseId_NULL)
    goal = last();

  uint64  tmp;
  int     idx;

  if (trav.hdmode) {     
    trav.refNodeVector.growTo(goal+1);
  }

  for(ClauseId id = 0; id <= goal; id++){

    if (trav.hdmode) {     
      trav.refNodeVector[id]=fp.tell();
    }
        
    tmp = getUInt(fp);
        
    if ((tmp & 1) == 0){
      // Root clause:
      clause.clear();
      idx = tmp >> 1;
      clause.push(l.toLit(idx));
      //	    printf("Clause: %d", idx);
      for(;;){
	tmp = getUInt(fp);
	if (tmp == 0) break;
	idx += tmp;
	clause.push(l.toLit(idx));
	//   	        printf(" %d", idx);
      }
      trav.root(clause);
      totSize++;
    }else{
      // Derivation or Deletion:
      chain_id .clear();
      chain_var.clear();
      chain_id.push(id - (tmp >> 1));
      //	    printf("Id: %d", id - (tmp >> 1));
      for(;;){
	tmp = getUInt(fp);
	if (tmp == 0) break;
	chain_var.push(l.toLit(tmp - 1));
	//   	        printf(" Var: %d", tmp-1);
	tmp = getUInt(fp);
	chain_id.push(id - tmp);
	//   	        printf(" Id: %d", id - tmp);
	totSize++;
      }

      if (chain_var.size() == 0)
	id--,   // (no new clause introduced)
	  //		printf(" deleted"),
	  trav.deleted(chain_id[0]);
      else
	trav.chain(chain_id, chain_var);
    }
    //   	    printf("\n");
  }
  //  printf("TOT PROOF SIZE: %d\n",totSize);
  trav.done();

  // Restore write (proof-logging) mode:
  fp.seek(0, SEEK_END);
  fp.setMode(WRITE);
}

void Proof::deleteTemps() {
  fp.close();
  temp_files.deleteTemps();
}

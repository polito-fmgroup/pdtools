/***************************************************************************
Copyright (c) 2006-2011, Armin Biere, Johannes Kepler University.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
***************************************************************************/

#define USAGE \
"usage: aigdd [-h][-v][-r] <src> <dst> <run> [ <run-options> ...]\n" \
"\n" \
"This 'delta debugger' for AIGs has the following options:\n" \
"\n" \
"  -h     prints this command line option summary\n" \
"  -v     increases verbose level (default 0, max 3)\n" \
"  -r     reencode and remove holes even if <dst> is in ASCII format\n" \
"  <src>  source file in AIGER format\n" \
"  <dst>  destination file in AIGER format\n" \
"  <run>  executable\n" \
"\n" \
"The idea is that '<run> <src>' produces a fault, e.g. you know that\n" \
"there is a big AIG saved in '<src>' which produces a wrong behaviour\n" \
"when given as argument to the program '<run>'.\n" \
"\n" \
"You can now use 'aigdd' to produce a copy '<dst>' of '<src>' in which\n" \
"as many literals as possible are removed while still producing\n" \
"the same exit code when running '<run> <dst>'.  Literals are actually\n" \
"removed by assigning them to a constant.  This in effect removes inputs,\n" \
"latches and ANDs.  The number of outputs is currently not changed, but\n" \
"individual outputs are set to constants.\n" \
"\n" \
"If '<dst>' is an AIG in ASCII format, by specifying a '.aag' extension,\n" \
"then the 'holes' left by removed literals are not squeezed out, while\n" \
"in the binary this is enforced.\n" \
"\n" \
"As a typical example consider that you have a new structural SAT solver\n" \
"'solve' that reads AIGs.  On one AIG it fails with an assertion\n" \
"failure.  You save this AIG in '/tmp/fail.aig'.  To shrink\n" \
"this file while still producing a failure you could just use\n" \
"\n" \
"  aigdd /tmp/fail.aig /tmp/shrunken.aig solve\n" \
"\n" \
"Unless your solver produces the same exit code for a correct run, this\n" \
"should give a compact easier to analyze AIG in '/tmp/shrunken.aig'.\n"

#include "aiger.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>           /* for WEXITSTATUS */

static aiger *src;
static const char *dst_name;
static unsigned *stable;
static unsigned *unstable;
static char * fixed;
static char * outputs;
static char * bad;
static char * constraints;
static char * justice;
static char * fairness;
static int verbose;
static int reencode;
static int runs;

static void
msg (int level, const char *fmt, ...)
{
  va_list ap;
  if (verbose < level)
    return;
  fputs ("[aigdd] ", stderr);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
}

static void
die (const char *fmt, ...)
{
  va_list ap;
  fputs ("*** [aigdd] ", stderr);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
  exit (1);
}

static unsigned
deref (unsigned lit)
{
  unsigned idx = lit / 2, sign = lit & 1, tmp, res, tmp0, tmp1;
  aiger_and * and;

  tmp = unstable[idx];

  if (tmp == 2 * idx)
    {
      if (!fixed [idx])
	{
	  and = aiger_is_and (src, 2 * idx);

	  if (and)
	    {
	      tmp0 = deref (and->rhs0);
	      tmp1 = deref (and->rhs1);

	      if (!tmp0 || !tmp1)
		tmp = 0;
	      else if (tmp0 == 1)
		tmp = tmp1;
	      else if (tmp1 == 1)
		tmp = tmp0;
	    }
	}
    }
  else
    tmp = deref (tmp);

  unstable[idx] = tmp;
  fixed[idx] = 1;
  res = tmp ^ sign;

  return res;
}

static void
write_unstable_to_dst (void)
{
  aiger_symbol *symbol;
  unsigned i, j, lit;
  aiger_and *and;
  aiger *dst;

  memset (fixed, 0, src->maxvar + 1);

  dst = aiger_init ();

  for (i = 0; i < src->num_inputs; i++)
    {
      symbol = src->inputs + i;
      lit = symbol->lit;
      if (deref (lit) != lit) continue;
      aiger_add_input (dst, lit, symbol->name);
      if (symbol->reset)				// TODO delta debug
        aiger_add_reset (dst, lit, symbol->reset);
    }

  for (i = 0; i < src->num_latches; i++)
    {
      symbol = src->latches + i;
      lit = symbol->lit;
      if (deref (lit) != lit) continue;
      aiger_add_latch (dst, lit, deref (symbol->next), symbol->name);
      if (symbol->reset <= 1)
	aiger_add_reset (dst, lit, symbol->reset);
      else
	{
	  assert (symbol->reset == lit);
	aiger_add_reset (dst, lit, lit);
	}
    }

  for (i = 0; i < src->num_ands; i++)
    {
      and = src->ands + i;
      if (deref (and->lhs) == and->lhs)
	aiger_add_and (dst, and->lhs, deref (and->rhs0), deref (and->rhs1));
    }

  for (i = 0; i < src->num_outputs; i++)
    {
      if (!outputs[i]) continue;
      symbol = src->outputs + i;
      aiger_add_output (dst, deref (symbol->lit), symbol->name);
    }

  for (i = 0; i < src->num_bad; i++)
    {
      if (!bad[i]) continue;
      symbol = src->bad + i;
      aiger_add_bad (dst, deref (symbol->lit), symbol->name);
    }

  for (i = 0; i < src->num_constraints; i++)
    {
      if (!constraints[i]) continue;
      symbol = src->constraints + i;
      aiger_add_constraint (dst, deref (symbol->lit), symbol->name);
    }

  for (i = 0; i < src->num_justice; i++)
    {
      unsigned * lits;
      if (!justice[i]) continue;
      symbol = src->justice + i;
      lits = malloc (symbol->size * sizeof *lits);
      for (j = 0; j < symbol->size; j++)
	lits[j] = deref (symbol->lit);
      aiger_add_justice (dst, symbol->size, lits, symbol->name);
      free (lits);
    }

  for (i = 0; i < src->num_fairness; i++)
    {
      if (!fairness[i]) continue;
      symbol = src->fairness + i;
      aiger_add_fairness (dst, deref (symbol->lit), symbol->name);
    }


  assert (!aiger_check (dst));

  if (reencode)
    aiger_reencode (dst);

  unlink (dst_name);
  if (!aiger_open_and_write_to_file (dst, dst_name))
    die ("failed to write '%s'", dst_name);
  aiger_reset (dst);
}

static void
copy_stable_to_unstable (void)
{
  unsigned i;

  for (i = 0; i <= src->maxvar; i++)
    unstable[i] = stable[i];
}

// #define CMDPREFIX "exec " // did not work?
#define CMDPREFIX ""
#define CMDSUFFIX " 1>/dev/null 2>/dev/null"

static int
min (int a, int b)
{
  return a < b ? a : b;
}

static int
run (const char * cmd)
{
  int res;
  runs++;
  res = system (cmd);
  return WEXITSTATUS (res);
}

int
main (int argc, char **argv)
{
  int i, changed, delta, j, expected, res, last, outof;
  const char *src_name, *run_name, *err;
  int optslen, optsi, cmdlen;
  char *cmd;

  src_name = dst_name = run_name = 0;
  optsi = argc;
  optslen = 0;

  for (i = 1; i < argc; i++)
    {
      if (!src_name && !strcmp (argv[i], "-h"))
	{
	  fprintf (stderr, USAGE);
	  exit (0);
	}
      else if (!src_name && !strcmp (argv[i], "-v"))
	verbose++;
      else if (!src_name && !strcmp (argv[i], "-r"))
	reencode = 1;
      else if (src_name && dst_name && run_name)
	{
	  optslen += strlen (argv[i]) + 1;
	  if (optsi == argc)
	    optsi = i;
	}
      else if (dst_name)
	run_name = argv[i];
      else if (src_name)
	dst_name = argv[i];
      else
	src_name = argv[i];
    }

  if (!src_name || !dst_name)
    die ("expected exactly two files");

  if (!run_name)
    die ("name of executable missing");

  cmdlen = strlen (CMDPREFIX);
  cmdlen += strlen (run_name);
  cmdlen += optslen;
  cmdlen += 1;
  if (strlen (src_name) > strlen (dst_name))
    cmdlen += strlen (src_name);
  else
    cmdlen += strlen (dst_name);
  cmdlen += strlen (CMDSUFFIX);
  cmdlen += 1;

  cmd = malloc (cmdlen);
  strcpy (cmd, CMDPREFIX);
  strcat (cmd, run_name);
  for (i = optsi; i < argc; i++)
    sprintf (cmd + strlen (cmd), " %s", argv[i]);
  strcat (cmd, " ");
  strcat (cmd, src_name);
  strcat (cmd, CMDSUFFIX);
  expected = run (cmd);
  msg (1, "'%s' returns %d", cmd, expected);
  free (cmd);

  cmd = malloc (cmdlen);
  strcpy (cmd, CMDPREFIX);
  strcat (cmd, run_name);
  for (i = optsi; i < argc; i++)
    sprintf (cmd + strlen (cmd), " %s", argv[i]);
  strcat (cmd, " ");
  strcat (cmd, dst_name);
  strcat (cmd, CMDSUFFIX);

  src = aiger_init ();
  if ((err = aiger_open_and_read_from_file (src, src_name)))
    die ("%s: %s", src_name, err);

  stable = malloc (sizeof (stable[0]) * (src->maxvar + 1));
  unstable = malloc (sizeof (unstable[0]) * (src->maxvar + 1));
  fixed = malloc (src->maxvar + 1);

  outputs = malloc (src->num_outputs + sizeof *outputs);
  bad = malloc (src->num_bad + sizeof *bad);
  constraints = malloc (src->num_constraints + sizeof *constraints);
  justice = malloc (src->num_justice + sizeof *justice);
  fairness = malloc (src->num_fairness + sizeof *fairness);

  for (i = 0; i <= src->maxvar; i++)
    stable[i] = 2 * i;
  for (i = 0; i < src->num_outputs; i++)
    outputs[i] = 1;
  for (i = 0; i < src->num_bad; i++)
    bad[i] = 1;
  for (i = 0; i < src->num_constraints; i++)
    constraints[i] = 1;
  for (i = 0; i < src->num_justice; i++)
    justice[i] = 1;
  for (i = 0; i < src->num_fairness; i++)
    fairness[i] = 1;

  copy_stable_to_unstable ();
  write_unstable_to_dst ();

  res = run (cmd);
  if (res != expected)
    die ("return value of copy differs (%d instead of %d)", res, expected);

  for (delta = src->maxvar; delta; delta = (delta == 1) ? 0 : (delta + 1) / 2)
    {
      i = 1;

      do
	{
	  for (j = 1; j < i; j++)
	    unstable[j] = stable[j];

	  changed = 0;
	  last = min (i + delta - 1, src->maxvar);
	  outof = last - i + 1;
	  for (j = i; j <= last; j++)
	    {
	      if (stable[j])	/* replace '1' by '0' as well */
		{
		  unstable[j] = 0;
		  changed++;
		}
	      else
		unstable[j] = 0;	/* always favor 'zero' */
	    }

	  if (changed)
	    {
	      for (j = i + delta; j <= src->maxvar; j++)
		unstable[j] = stable[j];

	      write_unstable_to_dst ();
	      res = run (cmd);
	      if (res == expected)
		{
		  msg (1, "[%d,%d] set to 0 (%d out of %d)",
		       i, last, changed, outof);

		  for (j = i; j <= last; j++)
		    stable[j] = unstable[j];
		}
	      else		/* try setting to 'one' */
		{
		  msg (2, "[%d,%d] can not be set to 0 (%d out of %d)",
		       i, last, changed, outof);

		  for (j = 1; j < i; j++)
		    unstable[j] = stable[j];

		  changed = 0;
		  for (j = i; j <= last; j++)
		    {
		      if (stable[j])
			{
			  if (stable[j] > 1)
			    {
			      unstable[j] = 1;
			      changed++;
			    }
			  else
			    unstable[j] = 1;
			}
		      else
			unstable[j] = 0;	/* always favor '0' */
		    }

		  if (changed)
		    {
		      for (j = i + delta; j <= src->maxvar; j++)
			unstable[j] = stable[j];

		      write_unstable_to_dst ();
		      res = run (cmd);
		      if (res == expected)
			{
			  msg (1, "[%d,%d] set to 1 (%d out of %d)",
			       i, last, changed, outof);

			  for (j = i; j < i + delta && j <= src->maxvar; j++)
			    stable[j] = unstable[j];
			}
		      else
			msg (2,
			     "[%d,%d] can neither be set to 1 (%d out of %d)",
			     i, last, changed, outof);
		    }
		}
	    }
	  else
	    msg (3, "[%d,%d] stabilized to 0", i, last);

	  i += delta;
	}
      while (i <= src->maxvar);
    }

  copy_stable_to_unstable ();
  write_unstable_to_dst ();

  if (src->num_outputs)
    {
      changed = 0;
      for (i = 0; i < src->num_outputs; i++)
	{
	  assert (outputs[i]);
	  outputs[i] = 0;
	  write_unstable_to_dst ();
	  res = run (cmd);
	  if (res == expected)
	    {
	      msg (1, "removed output %d", i);
	      changed++;
	    }
	  else
	    {
	      msg (2, "can not remove output %d", i);
	      outputs[i] = 1;
	    }
	}
      msg (1, "removed %d outputs", changed);

      copy_stable_to_unstable ();
      write_unstable_to_dst ();
    }

  if (src->num_bad)
    {
      changed = 0;
      for (i = 0; i < src->num_bad; i++)
	{
	  assert (bad[i]);
	  bad[i] = 0;
	  write_unstable_to_dst ();
	  res = run (cmd);
	  if (res == expected)
	    {
	      msg (1, "removed bad state property %d", i);
	      changed++;
	    }
	  else
	    {
	      msg (2, "can not remove bad state property %d", i);
	      bad[i] = 1;
	    }
	}
      msg (1, "removed %d bad state properties", changed);

      copy_stable_to_unstable ();
      write_unstable_to_dst ();
    }

  if (src->num_constraints)
    {
      changed = 0;
      for (i = 0; i < src->num_constraints; i++)
	{
	  assert (constraints[i]);
	  constraints[i] = 0;
	  write_unstable_to_dst ();
	  res = run (cmd);
	  if (res == expected)
	    {
	      msg (1, "removed environment constraint %d", i);
	      changed++;
	    }
	  else
	    {
	      msg (2, "can not remove environment constraint %d", i);
	      constraints[i] = 1;
	    }
	}
      msg (1, "removed %d environment constraints", changed);

      copy_stable_to_unstable ();
      write_unstable_to_dst ();
    }

  if (src->num_justice)
    {
      changed = 0;
      for (i = 0; i < src->num_justice; i++)
	{
	  assert (justice[i]);
	  justice[i] = 0;
	  write_unstable_to_dst ();
	  res = run (cmd);
	  if (res == expected)
	    {
	      msg (1, "removed justice property %d", i);
	      changed++;
	    }
	  else
	    {
	      msg (2, "can not remove justice property %d", i);
	      justice[i] = 1;
	    }
	}
      msg (1, "removed %d justice property", changed);

      copy_stable_to_unstable ();
      write_unstable_to_dst ();
    }

  if (src->num_fairness)
    {
      changed = 0;
      for (i = 0; i < src->num_fairness; i++)
	{
	  assert (fairness[i]);
	  fairness[i] = 0;
	  write_unstable_to_dst ();
	  res = run (cmd);
	  if (res == expected)
	    {
	      msg (1, "removed fairness constraint %d", i);
	      changed++;
	    }
	  else
	    {
	      msg (2, "can not remove fairness constraint %d", i);
	      fairness[i] = 1;
	    }
	}
      msg (1, "removed %d fairness constraint", changed);

      copy_stable_to_unstable ();
      write_unstable_to_dst ();
    }

  changed = 0;
  for (i = 1; i <= src->maxvar; i++)
    if (stable[i] <= 1)
      changed++;

  msg (1, "%.1f%% literals removed (%d out of %d)",
       src->maxvar ? changed * 100.0 / src->maxvar : 0, changed, src->maxvar);

  free (fairness);
  free (justice);
  free (constraints);
  free (bad);
  free (stable);
  free (unstable);
  free (fixed);
  free (cmd);
  aiger_reset (src);

  return 0;
}

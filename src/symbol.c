#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#include "symbol.h"
#include "debug.h"
#include "memory.h"

/*
   Symbol processor.

   Stores symbols for the lexical scanner. Can later print them.
   Implementation uses a hashtable, the size of which is defined in
   symbols.h.
*/

/* accessible for externals */

int globalError;		//!< If >0, stdout output goes to stderr (for e.g. terms)
char *globalStream;		//!< Defaults to stdout

/* external declarations */

extern int yylineno;

/* global declarations */

//! Symbol hash table.
Symbol symbtab[HASHSIZE];
//! List of available (freed) symbol blocks.
Symbol symb_list;
//! List of all allocated symbol blocks.
Symbol symb_alloc;

/* main code */

//! Open symbols code.
void
symbolsInit (void)
{
  int i;

  for (i = 0; i < HASHSIZE; i++)
    symbtab[i] = NULL;
  symb_list = NULL;
  symb_alloc = NULL;
  globalError = 0;
  globalStream = (char *) stdout;
}

//! Close symbols code.
void
symbolsDone (void)
{
  Symbol s;

  while (symb_alloc != NULL)
    {
      s = symb_alloc;
      symb_alloc = s->allocnext;
      memFree (s, sizeof (struct symbol));
    }
}

//! Create a memory block for a symbol.
/**
 * Internal memory management is used.
 *@return A pointer to a memory block of size struct.
 */
Symbol
get_symb (void)
{
  Symbol t;
  if (symb_list != NULL)
    {
      t = symb_list;
      symb_list = symb_list->next;
    }
  else
    {
      t = (Symbol) memAlloc (sizeof (struct symbol));
      t->allocnext = symb_alloc;
      symb_alloc = t;
    }
  t->keylevel = INT_MAX;
  return t;
}

//! Declare a symbol to be freed.
void
free_symb (const Symbol s)
{
  if (s == NULL)
    return;
  s->next = symb_list;
  symb_list = s;
}

//! Return the index in the hash table for the string.
int
hash (const char *s)
{
  int hv = 0;
  int i;

  for (i = 0; s[i] != EOS; i++)
    {
      int v = (hv >> 28) ^ (s[i] & 0xf);
      hv = (hv << 4) | v;
    }
  hv = hv & 0x7fffffff;
  return hv % HASHSIZE;
}

//! Insert a string into the hash table.
void
insert (const Symbol s)
{
  int hv;

  if (s == NULL)
    return;			/* illegal insertion of empty stuff */

  hv = hash (s->text);
  s->next = symbtab[hv];
  symbtab[hv] = s;
}

//! Find a string in the hash table.
Symbol
lookup (const char *s)
{
  int hv;
  Symbol t;

  if (s == NULL)
    return NULL;

  hv = hash (s);
  t = symbtab[hv];

  while (t != NULL)
    {
      if (strcmp (t->text, s) == 0)
	break;
      else
	t = t->next;
    }
  return t;
}

//! Print a symbol.
void
symbolPrint (const Symbol s)
{
  if (s == NULL)
    return;

  /* TODO maybe action depending on type? */
  eprintf ("%s", s->text);
}

//! Print all symbols
void
symbolPrintAll (void)
{
  int i, count;

  eprintf ("List of all symbols\n");
  count = 0;
  for (i = 0; i < HASHSIZE; i++)
    {
      Symbol sym;

      sym = symbtab[i];
      if (sym != NULL)
	{
	  eprintf ("H%i:\t", i);
	  while (sym != NULL)
	    {
	      count++;
	      eprintf ("[%s]\t", sym->text);
	      sym = sym->next;
	    }
	  eprintf ("\n");
	}
    }
  eprintf ("Total:\t%i\n", count);
}

//! Insert a string into the symbol table, if it wasn't there yet.
/**
 * Also sets line numbers and type.
 *\sa T_SYSCONST
 */
Symbol
symbolSysConst (const char *str)
{
  Symbol symb;

  symb = lookup (str);
  if (symb == NULL)
    {
      symb = get_symb ();
      symb->lineno = yylineno;
      symb->type = T_SYSCONST;
      symb->text = str;
      insert (symb);
    }
  return symb;
}

//! Generate the first fresh free number symbol, prefixed by a certain symbol's string.
/**
 * Note that there is an upper limit to this, to avoid some problems with buffer overflows etc.
 */
Symbol
symbolNextFree (Symbol prefixsymbol)
{
  char *prefixstr;
  int n;
  int len;

  if (prefixsymbol != NULL)
    {
      prefixstr = (char *) prefixsymbol->text;
    }
  else
    {
      prefixstr = "";
    }

  len = strlen (prefixstr);
  n = 1;
  while (n <= 9999)
    {
      char buffer[len + 5];	// thus we must enforce a maximum of 9.999 (allowing for storage of \0 )
      Symbol symb;
      int slen;

      slen = sprintf (buffer, "%s%i", prefixstr, n);
      symb = lookup (buffer);
      if (symb == NULL)
	{
	  char *newstring;
	  // Copy the buffer to something that will survive
	  /**
	   * Memory leak: although this routine should not be called recursively, it will never de-allocate this memory.
	   * Thus, some precaution is necessary.
	   * [x][CC]
	   */
	  newstring = (char *) memAlloc (slen + 1);
	  memcpy (newstring, buffer, slen + 1);

	  /* This persistent string can be used to return a fresh symbol */

	  return symbolSysConst (newstring);
	}

      // Try next one
      n++;
    }
  error ("We ran out of numbers (%i) when trying to generate a fresh symbol.",
	 n);
}

//! Fix all the unset keylevels
void
symbol_fix_keylevels (void)
{
  int i;

  for (i = 0; i < HASHSIZE; i++)
    {
      Symbol sym;

      sym = symbtab[i];
      while (sym != NULL)
	{
#ifdef DEBUG
	  if (DEBUGL (5))
	    {
	      eprintf ("Symbol ");
	      symbolPrint (sym);
	    }
#endif
	  if (sym->keylevel == INT_MAX)
	    {
	      // Nothing currently, this simply does not originate on a strand.
#ifdef DEBUG
	      if (DEBUGL (5))
		{
		  eprintf (" doesn't have a keylevel yet.\n");
		}
#endif
	    }
#ifdef DEBUG
	  else
	    {
	      if (DEBUGL (5))
		{
		  eprintf (" has keylevel %i\n", sym->keylevel);
		}
	    }
#endif
	  sym = sym->next;
	}
    }
}

//! Get output stream pointer
FILE *
getOutputStream (void)
{
  if (globalError == 0)
    return (FILE *) globalStream;
  else
    return stderr;
}

//! Print out according to globalError
/**
 * Input is comparable to printf, only depends on globalError. This should be
 * used by any function trying to do output.
 *
 * Furthermore, if globalError == 0, it can still be overriden by
 * globalStream, which can be another stream pointer. If it is null, stdout
 * is assumed.
 *
 *\sa globalError
 */
void
eprintf (char *fmt, ...)
{
  va_list args;

  va_start (args, fmt);
  vfprintf (getOutputStream (), fmt, args);
  va_end (args);
}

// Variable list variant
void
veprintf (const char *fmt, va_list args)
{
  vfprintf (getOutputStream (), fmt, args);
}

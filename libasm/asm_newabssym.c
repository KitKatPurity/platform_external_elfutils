/* Create new ABS symbol.
   Copyright (C) 2002 Red Hat, Inc.
   This file is part of Red Hat elfutils.
   Written by Ulrich Drepper <drepper@redhat.com>, 2002.

   Red Hat elfutils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 2 of the License.

   Red Hat elfutils is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with Red Hat elfutils; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301 USA.

   Red Hat elfutils is an included package of the Open Invention Network.
   An included package of the Open Invention Network is a package for which
   Open Invention Network licensees cross-license their patents.  No patent
   license is granted, either expressly or impliedly, by designation as an
   included package.  Should you wish to participate in the Open Invention
   Network licensing program, please visit www.openinventionnetwork.com
   <http://www.openinventionnetwork.com>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libasmP.h>
#include <system.h>


/* Object for special COMMON section.  */
static const AsmScn_t __libasm_abs_scn =
  {
    .data = {
      .main = {
	.scn = ASM_ABS_SCN
      }
    }
  };


AsmSym_t *
asm_newabssym (ctx, name, size, value, type, binding)
     AsmCtx_t *ctx;
     const char *name;
     GElf_Xword size;
     GElf_Addr value;
     int type;
     int binding;
{
  AsmSym_t *result;

  if (ctx == NULL)
    /* Something went wrong before.  */
    return NULL;

  /* Common symbols are public.  Therefore the user must provide a
     name.  */
  if (name == NULL)
    {
      __libasm_seterrno (ASM_E_INVALID);
      return NULL;
    }

  rwlock_wrlock (ctx->lock);

  result = (AsmSym_t *) malloc (sizeof (AsmSym_t));
  if (result == NULL)
    return NULL;

  result->scn = (AsmScn_t *) &__libasm_abs_scn;
  result->size = size;
  result->type = type;
  result->binding = binding;
  result->symidx = 0;
  result->strent = ebl_strtabadd (ctx->symbol_strtab, name, 0);

  /* The value of an ABS symbol must not be modified.  Since there are
     no subsection and the initial offset of the section is 0 we can
     get the alignment recorded by storing it into the offset
     field.  */
  result->offset = value;

  if (unlikely (ctx->textp))
    {
      /* An absolute symbol can be defined by giving a symbol a
	 specific value.  */
      if (binding == STB_GLOBAL)
	fprintf (ctx->out.file, "\t.globl %s\n", name);
      else if (binding == STB_WEAK)
	fprintf (ctx->out.file, "\t.weak %s\n", name);

      if (type == STT_OBJECT)
	fprintf (ctx->out.file, "\t.type %s,@object\n", name);
      else if (type == STT_FUNC)
	fprintf (ctx->out.file, "\t.type %s,@function\n", name);

      fprintf (ctx->out.file, "%s = %llu\n",
	       name, (unsigned long long int) value);

      if (size != 0)
	fprintf (ctx->out.file, "\t.size %s, %llu\n",
		 name, (unsigned long long int) size);
    }
  else
    {
      /* Put the symbol in the hash table so that we can later find it.  */
      if (asm_symbol_tab_insert (&ctx->symbol_tab, elf_hash (name), result)
	  != 0)
	{
	  /* The symbol already exists.  */
	  __libasm_seterrno (ASM_E_DUPLSYM);
	  free (result);
	  result = NULL;
	}
      else if (name != NULL && asm_emit_symbol_p (name))
	/* Only count non-private symbols.  */
	++ctx->nsymbol_tab;
    }

  rwlock_unlock (ctx->lock);

  return result;
}

/*
 * Debugging object format (used to debug object format module interface)
 *
 *  Copyright (C) 2001  Peter Johnson
 *
 *  This file is part of YASM.
 *
 *  YASM is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  YASM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "util.h"
/*@unused@*/ RCSID("$IdPath$");

#include "globals.h"
#include "errwarn.h"
#include "expr.h"
#include "symrec.h"

#include "section.h"
#include "objfmt.h"


/* Note that the functions here write to debug_file.  This is NOT legal for
 * other object formats to do--only the output() function can write to a file,
 * and only the file it's passed in its f parameter!
 */

static void
dbg_objfmt_initialize(const char *in_filename, const char *obj_filename)
{
    assert(debug_file != NULL);
    fprintf(debug_file, "%*sinitialize(\"%s\", \"%s\")\n", indent_level, "",
	    in_filename, obj_filename);
}

static void
dbg_objfmt_output(FILE *f, sectionhead *sections)
{
    fprintf(f, "%*soutput(f, sections->\n", indent_level, "");
    indent_level++;
    sections_print(f, sections);
    indent_level--;
    fprintf(f, "%*s)\n", indent_level, "");
    indent_level++;
    fprintf(f, "%*sSymbol Table:\n", indent_level, "");
    symrec_print_all(f);
    indent_level--;
}

static void
dbg_objfmt_cleanup(void)
{
    assert(debug_file != NULL);
    fprintf(debug_file, "%*scleanup()\n", indent_level, "");
}

static /*@observer@*/ /*@null@*/ section *
dbg_objfmt_sections_switch(sectionhead *headp, valparamhead *valparams,
			   /*@unused@*/ /*@null@*/
			   valparamhead *objext_valparams)
{
    valparam *vp;
    section *retval;
    int isnew;

    assert(debug_file != NULL);

    fprintf(debug_file, "%*ssections_switch(headp, ", indent_level, "");
    vps_print(debug_file, valparams);
    fprintf(debug_file, ", ");
    vps_print(debug_file, objext_valparams);
    fprintf(debug_file, "), returning ");

    if ((vp = vps_first(valparams)) && !vp->param && vp->val != NULL) {
	retval = sections_switch_general(headp, vp->val, 200, 0, &isnew);
	if (isnew) {
	    fprintf(debug_file, "(new) ");
	    symrec_define_label(vp->val, retval, (bytecode *)NULL, 1);
	}
	fprintf(debug_file, "\"%s\" section\n", vp->val);
	return retval;
    } else {
	fprintf(debug_file, "NULL\n");
	return NULL;
    }
}

static void
dbg_objfmt_section_data_delete(/*@only@*/ void *data)
{
    assert(debug_file != NULL);
    fprintf(debug_file, "%*ssection_data_delete(%p)\n", indent_level, "", data);
    xfree(data);
}

static void
dbg_objfmt_section_data_print(FILE *f, /*@null@*/ void *data)
{
    if (data)
	fprintf(f, "%*s%p\n", indent_level, "", data);
    else
	fprintf(f, "%*s(none)\n", indent_level, "");
}

static void
dbg_objfmt_extern_declare(symrec *sym, /*@unused@*/ /*@null@*/
			  valparamhead *objext_valparams)
{
    assert(debug_file != NULL);
    fprintf(debug_file, "%*sextern_declare(\"%s\", ", indent_level, "",
	    symrec_get_name(sym));
    vps_print(debug_file, objext_valparams);
    fprintf(debug_file, "), setting of_data=NULL\n");
    symrec_set_of_data(sym, NULL);
}

static void
dbg_objfmt_global_declare(symrec *sym, /*@unused@*/ /*@null@*/
			  valparamhead *objext_valparams)
{
    assert(debug_file != NULL);
    fprintf(debug_file, "%*sglobal_declare(\"%s\", ", indent_level, "",
	    symrec_get_name(sym));
    vps_print(debug_file, objext_valparams);
    fprintf(debug_file, "), setting of_data=NULL\n");
    symrec_set_of_data(sym, NULL);
}

static void
dbg_objfmt_common_declare(symrec *sym, /*@only@*/ expr *size, /*@unused@*/
			  /*@null@*/ valparamhead *objext_valparams)
{
    assert(debug_file != NULL);
    fprintf(debug_file, "%*scommon_declare(\"%s\", ", indent_level, "",
	    symrec_get_name(sym));
    expr_print(debug_file, size);
    fprintf(debug_file, ", ");
    vps_print(debug_file, objext_valparams);
    fprintf(debug_file, "), setting of_data=");
    expr_print(debug_file, size);
    symrec_set_of_data(sym, size);
    fprintf(debug_file, "\n");
}

static void
dbg_objfmt_symrec_data_delete(/*@only@*/ void *data)
{
    assert(debug_file != NULL);
    fprintf(debug_file, "%*ssymrec_data_delete(", indent_level, "");
    if (data) {
	expr_print(debug_file, data);
	expr_delete(data);
    }
    fprintf(debug_file, ")\n");
}

static void
dbg_objfmt_symrec_data_print(FILE *f, /*@null@*/ void *data)
{
    if (data) {
	fprintf(f, "%*sSize=", indent_level, "");
	expr_print(f, data);
	fprintf(f, "\n");
    } else {
	fprintf(f, "%*s(none)\n", indent_level, "");
    }
}

static int
dbg_objfmt_directive(const char *name, valparamhead *valparams,
		     /*@null@*/ valparamhead *objext_valparams,
		     /*@unused@*/ sectionhead *headp)
{
    assert(debug_file != NULL);
    fprintf(debug_file, "%*sdirective(\"%s\", ", indent_level, "", name);
    vps_print(debug_file, valparams);
    fprintf(debug_file, ", ");
    vps_print(debug_file, objext_valparams);
    fprintf(debug_file, "), returning 0 (recognized)\n");
    return 0;	    /* dbg format "recognizes" all directives */
}

static void
dbg_objfmt_bc_objfmt_data_delete(unsigned int type, /*@only@*/ void *data)
{
    assert(debug_file != NULL);
    fprintf(debug_file, "%*ssymrec_data_delete(%u, %p)\n", indent_level, "",
	    type, data);
    xfree(data);
}

static void
dbg_objfmt_bc_objfmt_data_print(FILE *f, unsigned int type, const void *data)
{
    fprintf(f, "%*sType=%u\n", indent_level, "", type);
    fprintf(f, "%*sData=%p\n", indent_level, "", data);
}

/* Define objfmt structure -- see objfmt.h for details */
objfmt yasm_dbg_LTX_objfmt = {
    "Trace of all info passed to object format module",
    "dbg",
    "dbg",
    ".text",
    32,
    dbg_objfmt_initialize,
    dbg_objfmt_output,
    dbg_objfmt_cleanup,
    dbg_objfmt_sections_switch,
    dbg_objfmt_section_data_delete,
    dbg_objfmt_section_data_print,
    dbg_objfmt_extern_declare,
    dbg_objfmt_global_declare,
    dbg_objfmt_common_declare,
    dbg_objfmt_symrec_data_delete,
    dbg_objfmt_symrec_data_print,
    dbg_objfmt_directive,
    dbg_objfmt_bc_objfmt_data_delete,
    dbg_objfmt_bc_objfmt_data_print
};

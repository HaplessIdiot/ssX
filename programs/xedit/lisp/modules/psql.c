/*
 * Copyright (c) 2001 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo César Pereira de Andrade
 */

/* $XFree86: xc/programs/xedit/lisp/modules/psql.c,v 1.3 2001/10/15 15:36:51 paulo Exp $ */

#include <stdlib.h>
#include <libpq-fe.h>
#undef USE_SSL		/* cannot get it to compile... */
#include <postgres.h>
#include <utils/geo_decls.h>
#include "internal.h"

/*
 * Prototypes
 */
int psqlLoadModule(LispMac *mac);

LispObj *Lisp_PQbackendPID(LispMac*, LispBuiltin*);
LispObj *Lisp_PQclear(LispMac*, LispBuiltin*);
LispObj *Lisp_PQconsumeInput(LispMac*, LispBuiltin*);
LispObj *Lisp_PQdb(LispMac*, LispBuiltin*);
LispObj *Lisp_PQerrorMessage(LispMac*, LispBuiltin*);
LispObj *Lisp_PQexec(LispMac*, LispBuiltin*);
LispObj *Lisp_PQfinish(LispMac*, LispBuiltin*);
LispObj *Lisp_PQfname(LispMac*, LispBuiltin*);
LispObj *Lisp_PQfnumber(LispMac*, LispBuiltin*);
LispObj *Lisp_PQfsize(LispMac*, LispBuiltin*);
LispObj *Lisp_PQftype(LispMac*, LispBuiltin*);
LispObj *Lisp_PQgetlength(LispMac*, LispBuiltin*);
LispObj *Lisp_PQgetvalue(LispMac*, LispBuiltin*);
LispObj *Lisp_PQhost(LispMac*, LispBuiltin*);
LispObj *Lisp_PQnfields(LispMac*, LispBuiltin*);
LispObj *Lisp_PQnotifies(LispMac*, LispBuiltin*);
LispObj *Lisp_PQntuples(LispMac*, LispBuiltin*);
LispObj *Lisp_PQoptions(LispMac*, LispBuiltin*);
LispObj *Lisp_PQpass(LispMac*, LispBuiltin*);
LispObj *Lisp_PQport(LispMac*, LispBuiltin*);
LispObj *Lisp_PQresultStatus(LispMac*, LispBuiltin*);
LispObj *Lisp_PQsetdb(LispMac*, LispBuiltin*);
LispObj *Lisp_PQsetdbLogin(LispMac*, LispBuiltin*);
LispObj *Lisp_PQsocket(LispMac*, LispBuiltin*);
LispObj *Lisp_PQstatus(LispMac*, LispBuiltin*);
LispObj *Lisp_PQtty(LispMac*, LispBuiltin*);
LispObj *Lisp_PQuser(LispMac*, LispBuiltin*);

/*
 * Initialization
 */
static LispBuiltin lispbuiltins[] = {
    {LispFunction, Lisp_PQbackendPID, "pq-backend-pid connection"},
    {LispFunction, Lisp_PQclear, "pq-clear result"},
    {LispFunction, Lisp_PQconsumeInput, "pq-consume-input connection"},
    {LispFunction, Lisp_PQdb, "pq-db connection"},
    {LispFunction, Lisp_PQerrorMessage, "pq-error-message connection"},
    {LispFunction, Lisp_PQexec, "pq-exec connection query"},
    {LispFunction, Lisp_PQfinish, "pq-finish connection"},
    {LispFunction, Lisp_PQfname, "pq-fname result field-number"},
    {LispFunction, Lisp_PQfnumber, "pq-fnumber result field-name"},
    {LispFunction, Lisp_PQfsize, "pq-fsize result field-number"},
    {LispFunction, Lisp_PQftype, "pq-ftype result field-number"},
    {LispFunction, Lisp_PQgetlength, "pq-getlength result tupple field-number"},
    {LispFunction, Lisp_PQgetvalue, "pq-getvalue result tupple field-number &optional type"},
    {LispFunction, Lisp_PQhost, "pq-host connection"},
    {LispFunction, Lisp_PQnfields, "pq-nfields result"},
    {LispFunction, Lisp_PQnotifies, "pq-notifies connection"},
    {LispFunction, Lisp_PQntuples, "pq-ntuples result"},
    {LispFunction, Lisp_PQoptions, "pq-options connection"},
    {LispFunction, Lisp_PQpass, "pq-pass connection"},
    {LispFunction, Lisp_PQport, "pq-port connection"},
    {LispFunction, Lisp_PQresultStatus, "pq-result-status result"},
    {LispFunction, Lisp_PQsetdb, "pq-setdb host port options tty dbname"},
    {LispFunction, Lisp_PQsetdbLogin, "pq-setdb-login host port options tty dbname login password"},
    {LispFunction, Lisp_PQsocket, "pq-socket connection"},
    {LispFunction, Lisp_PQstatus, "pq-status connection"},
    {LispFunction, Lisp_PQtty, "pq-tty connection"},
    {LispFunction, Lisp_PQuser, "pq-user connection"},
};

LispModuleData psqlLispModuleData = {
    LISP_MODULE_VERSION,
    psqlLoadModule
};

static int PGconn_t, PGresult_t;

/*
 * Implementation
 */
int
psqlLoadModule(LispMac *mac)
{
    int i;
    char *fname = "PSQL-LOAD-MODULE";

    PGconn_t = LispRegisterOpaqueType(mac, "PGconn*");
    PGresult_t = LispRegisterOpaqueType(mac, "PGresult*");

    GCProtect();
    /* NOTE: Implemented just enough to make programming examples
     * (and my needs) work.
     * Completing this is an exercise to the reader, or may be implemented
     * when/if required.
     */
    LispExecute(mac,
		"(DEFSTRUCT PG-NOTIFY RELNAME BE-PID)\n"
		"(DEFSTRUCT PG-POINT X Y)\n"
		"(DEFSTRUCT PG-BOX HIGH LOW)\n"
		"(DEFSTRUCT PG-POLYGON SIZE NUM-POINTS BOUNDBOX POINTS)\n");

    /* enum ConnStatusType */
    (void)LispSetVariable(mac, ATOM2("PG-CONNECTION-OK"),
			  REAL(CONNECTION_OK), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PG-CONNECTION-BAD"),
			  REAL(CONNECTION_BAD), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PG-CONNECTION-STARTED"),
			  REAL(CONNECTION_STARTED), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PG-CONNECTION-MADE"),
			  REAL(CONNECTION_MADE), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PG-CONNECTION-AWAITING-RESPONSE"),
			  REAL(CONNECTION_AWAITING_RESPONSE), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PG-CONNECTION-AUTH-OK"),
			  REAL(CONNECTION_AUTH_OK), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PG-CONNECTION-SETENV"),
			  REAL(CONNECTION_SETENV), fname, 0);


    /* enum ExecStatusType */
    (void)LispSetVariable(mac, ATOM2("PGRES-EMPTY-QUERY"),
			  REAL(PGRES_EMPTY_QUERY), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PGRES-COMMAND-OK"),
			  REAL(PGRES_COMMAND_OK), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PGRES-TUPLES-OK"),
			  REAL(PGRES_TUPLES_OK), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PGRES-COPY-OUT"),
			  REAL(PGRES_COPY_OUT), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PGRES-COPY-IN"),
			  REAL(PGRES_COPY_IN), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PGRES-BAD-RESPONSE"),
			  REAL(PGRES_BAD_RESPONSE), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PGRES-NONFATAL-ERROR"),
			  REAL(PGRES_NONFATAL_ERROR), fname, 0);
    (void)LispSetVariable(mac, ATOM2("PGRES-FATAL-ERROR"),
			  REAL(PGRES_FATAL_ERROR), fname, 0);
    GCUProtect();

    for (i = 0; i < sizeof(lispbuiltins) / sizeof(lispbuiltins[0]); i++)
	LispAddBuiltinFunction(mac, &lispbuiltins[i]);

    return (1);
}

LispObj *
Lisp_PQbackendPID(LispMac *mac, LispBuiltin *builtin)
/*
 pq-backend-pid connection
 */
{
    int pid;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    pid = PQbackendPID(conn);

    return (INTEGER(pid));
}

LispObj *
Lisp_PQclear(LispMac *mac, LispBuiltin *builtin)
/*
 pq-clear result
 */
{
    PGresult *res;

    LispObj *result;

    result = ARGUMENT(0);

    if (!CHECKO(result, PGresult_t))
	LispDestroy(mac, "%s: cannot convert %s to PGresult*",
		    STRFUN(builtin), STROBJ(result));
    res = (PGresult*)(result->data.opaque.data);

    PQclear(res);

    return (NIL);
}

LispObj *
Lisp_PQconsumeInput(LispMac *mac, LispBuiltin *builtin)
/*
 pq-consume-input connection
 */
{
    int result;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    result = PQconsumeInput(conn);

    return (INTEGER(result));
}

LispObj *
Lisp_PQdb(LispMac *mac, LispBuiltin *builtin)
/*
 pq-db connection
 */
{
    char *string;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

     string = PQdb(conn);

    return (string ? STRING(string) : NIL);
}

LispObj *
Lisp_PQerrorMessage(LispMac *mac, LispBuiltin *builtin)
{
    char *string;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    string = PQerrorMessage(conn);

    return (string ? STRING(string) : NIL);
}

LispObj *
Lisp_PQexec(LispMac *mac, LispBuiltin *builtin)
/*
 pq-exec connection query
 */
{
    PGconn *conn;
    PGresult *res;

    LispObj *connection, *query;

    query = ARGUMENT(1);
    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    if (!STRING_P(query))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(query));

    res = PQexec(conn, STRPTR(query));

    return (res ? OPAQUE(res, PGresult_t) : NIL);
}

LispObj *
Lisp_PQfinish(LispMac *mac, LispBuiltin *builtin)
/*
 pq-finish connection
 */
{
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    PQfinish(conn);

    return (NIL);
}

LispObj *
Lisp_PQfname(LispMac *mac, LispBuiltin *builtin)
/*
 pq-fname result field-number
 */
{
    char *string;
    int field;
    PGresult *res;

    LispObj *result, *field_number;

    field_number = ARGUMENT(1);
    result = ARGUMENT(0);

    if (!CHECKO(result, PGresult_t))
	LispDestroy(mac, "%s: cannot convert %s to PGresult*",
		    STRFUN(builtin), STROBJ(result));
    res = (PGresult*)(result->data.opaque.data);

    if (!INDEX_P(field_number))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(field_number));
    field = field_number->data.integer;

    string = PQfname(res, field);

    return (string ? STRING(string) : NIL);
}

LispObj *
Lisp_PQfnumber(LispMac *mac, LispBuiltin *builtin)
/*
 pq-fnumber result field-name
 */
{
    int number;
    int field;
    PGresult *res;

    LispObj *result, *field_name;

    field_name = ARGUMENT(1);
    result = ARGUMENT(0);

    if (!CHECKO(result, PGresult_t))
	LispDestroy(mac, "%s: cannot convert %s to PGresult*",
		    STRFUN(builtin), STROBJ(result));
    res = (PGresult*)(result->data.opaque.data);

    if (!STRING_P(field_name))
	LispDestroy(mac, "%s: %s is not a string",
		    STRFUN(builtin), STROBJ(field_name));

    number = PQfnumber(res, STRPTR(field_name));

    return (INTEGER(number));
}

LispObj *
Lisp_PQfsize(LispMac *mac, LispBuiltin *builtin)
/*
 pq-fsize result field-number
 */
{
    int size, field;
    PGresult *res;

    LispObj *result, *field_number;

    field_number = ARGUMENT(1);
    result = ARGUMENT(0);

    if (!CHECKO(result, PGresult_t))
	LispDestroy(mac, "%s: cannot convert %s to PGresult*",
		    STRFUN(builtin), STROBJ(result));
    res = (PGresult*)(result->data.opaque.data);

    if (!INDEX_P(field_number))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(field_number));
    field = field_number->data.integer;

    size = PQfsize(res, field);

    return (INTEGER(size));
}

LispObj *
Lisp_PQftype(LispMac *mac, LispBuiltin *builtin)
{
    Oid oid;
    int field;
    PGresult *res;

    LispObj *result, *field_number;

    field_number = ARGUMENT(1);
    result = ARGUMENT(0);

    if (!CHECKO(result, PGresult_t))
	LispDestroy(mac, "%s: cannot convert %s to PGresult*",
		    STRFUN(builtin), STROBJ(result));
    res = (PGresult*)(result->data.opaque.data);

    if (!INDEX_P(field_number))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(field_number));
    field = field_number->data.integer;

    oid = PQftype(res, field);

    return (INTEGER(oid));
}

LispObj *
Lisp_PQgetlength(LispMac *mac, LispBuiltin *builtin)
/*
 pq-getlength result tupple field-number
 */
{
    PGresult *res;
    int tuple, field, length;

    LispObj *result, *otupple, *field_number;

    field_number = ARGUMENT(2);
    otupple = ARGUMENT(1);
    result = ARGUMENT(0);

    if (!CHECKO(result, PGresult_t))
	LispDestroy(mac, "%s: cannot convert %s to PGresult*",
		    STRFUN(builtin), STROBJ(result));
    res = (PGresult*)(result->data.opaque.data);

    if (!INDEX_P(otupple))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(field_number));
    tuple = otupple->data.integer;

    if (!INDEX_P(field_number))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(field_number));
    field = field_number->data.integer;

    length = PQgetlength(res, tuple, field);

    return (INTEGER(length));
}

LispObj *
Lisp_PQgetvalue(LispMac *mac, LispBuiltin *builtin)
/*
 pq-getvalue result tuple field &optional type-specifier
 */
{
    char *string;
    double real = 0.0;
    PGresult *res;
    int tuple, field, isint = 0, isreal = 0, integer;

    LispObj *result, *otupple, *field_number, *type;

    type = ARGUMENT(3);
    field_number = ARGUMENT(2);
    otupple = ARGUMENT(1);
    result = ARGUMENT(0);

    if (!CHECKO(result, PGresult_t))
	LispDestroy(mac, "%s: cannot convert %s to PGresult*",
		    STRFUN(builtin), STROBJ(result));
    res = (PGresult*)(result->data.opaque.data);

    if (!INDEX_P(otupple))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(field_number));
    tuple = otupple->data.integer;

    if (!INDEX_P(field_number))
	LispDestroy(mac, "%s: %s is not a positive integer",
		    STRFUN(builtin), STROBJ(field_number));
    field = field_number->data.integer;

    string = PQgetvalue(res, tuple, field);

    if (type != NIL) {
	char *typestring;

	if (!SYMBOL_P(type))
	    LispDestroy(mac, "%s: %s is not a symbol",
			STRFUN(builtin), STROBJ(type));

	typestring = STRPTR(type);

	if (strcmp(typestring, "INT16") == 0) {
	    integer = *(short*)string;
	    isint = 1;
	    goto simple_type;
	}
	else if (strcmp(typestring, "INT32") == 0) {
	    integer = *(int*)string;
	    isint = 1;
	    goto simple_type;
	}
	else if (strcmp(typestring, "FLOAT") == 0) {
	    real = *(float*)string;
	    isreal = 1;
	    goto simple_type;
	}
	else if (strcmp(typestring, "REAL") == 0) {
	    real = *(double*)string;
	    isreal = 1;
	    goto simple_type;
	}
	else if (strcmp(typestring, "PG-POLYGON") == 0)
	    goto polygon_type;
	else if (strcmp(typestring, "STRING") != 0)
	    LispDestroy(mac, "%s: unknown type %s",
			STRFUN(builtin), typestring);
    }

simple_type:
    return (isint ? INTEGER(integer) : isreal ? REAL(real) :
	    (string ? STRING(string) : NIL));

polygon_type:
  {
    LispObj *poly, *box, *p = NIL, *cdr, *obj;
    POLYGON *polygon;
    int i, size;

    size = PQgetlength(res, tuple, field);
    polygon = (POLYGON*)(string - sizeof(int));

    GCProtect();
    /* get polygon->boundbox */
    cdr = EVAL(CONS(ATOM("MAKE-PG-POINT"),
		    CONS(KEYWORD(ATOM("X")),
			 CONS(REAL(polygon->boundbox.high.x),
			      CONS(KEYWORD(ATOM("Y")),
				   CONS(REAL(polygon->boundbox.high.y), NIL))))));
    obj = EVAL(CONS(ATOM("MAKE-PG-POINT"),
		    CONS(KEYWORD(ATOM("X")),
			 CONS(REAL(polygon->boundbox.low.x),
			      CONS(KEYWORD(ATOM("Y")),
				   CONS(REAL(polygon->boundbox.low.y), NIL))))));
    box = EVAL(CONS(ATOM("MAKE-PG-BOX"),
		    CONS(KEYWORD(ATOM("HIGH")),
			 CONS(cdr,
			      CONS(KEYWORD(ATOM("LOW")),
				   CONS(obj, NIL))))));
    /* get polygon->p values */
    for (i = 0; i < polygon->npts; i++) {
	obj = EVAL(CONS(ATOM("MAKE-PG-POINT"),
			CONS(KEYWORD(ATOM("X")),
			     CONS(REAL(polygon->p[i].x),
			      CONS(KEYWORD(ATOM("Y")),
				   CONS(REAL(polygon->p[i].y), NIL))))));
	if (i == 0)
	    p = cdr = CONS(obj, NIL);
	else {
	    CDR(cdr) = CONS(obj, NIL);
	    cdr = CDR(cdr);
	}
    }

    /* make result */
    poly = EVAL(CONS(ATOM("MAKE-PG-POLYGON"),
		     CONS(KEYWORD(ATOM("SIZE")),
			  CONS(REAL(size),
			       CONS(KEYWORD(ATOM("NUM-POINTS")),
				    CONS(REAL(polygon->npts),
					 CONS(KEYWORD(ATOM("BOUNDBOX")),
					      CONS(box,
						   CONS(KEYWORD(ATOM("POINTS")),
							CONS(QUOTE(p), NIL))))))))));
    GCUProtect();

    return (poly);
  }
}

LispObj *
Lisp_PQhost(LispMac *mac, LispBuiltin *builtin)
/*
 pq-host connection
 */
{
    char *string;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    string = PQhost(conn);

    return (string ? STRING(string) : NIL);
}

LispObj *
Lisp_PQnfields(LispMac *mac, LispBuiltin *builtin)
/*
 pq-nfields result
 */
{
    int nfields;
    PGresult *res;

    LispObj *result;

    result = ARGUMENT(0);

    if (!CHECKO(result, PGresult_t))
	LispDestroy(mac, "%s: cannot convert %s to PGresult*",
		    STRFUN(builtin), STROBJ(result));
    res = (PGresult*)(result->data.opaque.data);

    nfields = PQnfields(res);

    return (INTEGER(nfields));
}

LispObj *
Lisp_PQnotifies(LispMac *mac, LispBuiltin *builtin)
/*
 pq-notifies connection
 */
{
    LispObj *result, *code, *frm = FRM;
    PGconn *conn;
    PGnotify *notifies;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    if ((notifies = PQnotifies(conn)) == NULL)
	return (NIL);

    GCProtect();
    code = CONS(ATOM("MAKE-PG-NOTIFY"),
		  CONS(KEYWORD(ATOM("RELNAME")),
		       CONS(STRING(notifies->relname),
			    CONS(KEYWORD(ATOM("BE-PID")),
				 CONS(REAL(notifies->be_pid), NIL)))));
    FRM = CONS(code, FRM);
    GCUProtect();
    result = EVAL(code);
    FRM = frm;

    free(notifies);

    return (result);
}

LispObj *
Lisp_PQntuples(LispMac *mac, LispBuiltin *builtin)
/*
 pq-ntuples result
 */
{
    int ntuples;
    PGresult *res;

    LispObj *result;

    result = ARGUMENT(0);

    if (!CHECKO(result, PGresult_t))
	LispDestroy(mac, "%s: cannot convert %s to PGresult*",
		    STRFUN(builtin), STROBJ(result));
    res = (PGresult*)(result->data.opaque.data);

    ntuples = PQntuples(res);

    return (INTEGER(ntuples));
}

LispObj *
Lisp_PQoptions(LispMac *mac, LispBuiltin *builtin)
/*
 pq-options connection
 */
{
    char *string;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    string = PQoptions(conn);

    return (string ? STRING(string) : NIL);
}

LispObj *
Lisp_PQpass(LispMac *mac, LispBuiltin *builtin)
/*
 pq-pass connection
 */
{
    char *string;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    string = PQpass(conn);

    return (string ? STRING(string) : NIL);
}

LispObj *
Lisp_PQport(LispMac *mac, LispBuiltin *builtin)
/*
 pq-port connection
 */
{
    char *string;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    string = PQport(conn);

    return (string ? STRING(string) : NIL);
}

LispObj *
Lisp_PQresultStatus(LispMac *mac, LispBuiltin *builtin)
/*
 pq-result-status result
 */
{
    int status;
    PGresult *res;

    LispObj *result;

    result = ARGUMENT(0);

    if (!CHECKO(result, PGresult_t))
	LispDestroy(mac, "%s: cannot convert %s to PGresult*",
		    STRFUN(builtin), STROBJ(result));
    res = (PGresult*)(result->data.opaque.data);

    status = PQresultStatus(res);

    return (INTEGER(status));
}

LispObj *
LispPQsetdb(LispMac *mac, LispBuiltin *builtin, int loginp)
/*
 pq-setdb host port options tty dbname
 pq-setdb-login host port options tty dbname login password
 */
{
    PGconn *conn;
    char *host, *port, *options, *tty, *dbname, *login, *password;

    LispObj *ohost, *oport, *ooptions, *otty, *odbname, *ologin, *opassword;

    if (loginp) {
	opassword = ARGUMENT(6);
	ologin = ARGUMENT(5);
    }
    else
	opassword = ologin = NIL;
    odbname = ARGUMENT(4);
    otty = ARGUMENT(3);
    ooptions = ARGUMENT(2);
    oport = ARGUMENT(1);
    ohost = ARGUMENT(0);

    if (ohost != NIL) {
	if (!STRING_P(ohost))
	    LispDestroy(mac, "%s: %s is not a string",
			STRFUN(builtin), STROBJ(ohost));
	host = STRPTR(ohost);
    }
    else
	host = NULL;

    if (oport != NIL) {
	if (!STRING_P(oport))
	    LispDestroy(mac, "%s: %s is not a string",
			STRFUN(builtin), STROBJ(oport));
	port = STRPTR(oport);
    }
    else
	port = NULL;

    if (ooptions != NIL) {
	if (!STRING_P(ooptions))
	    LispDestroy(mac, "%s: %s is not a string",
			STRFUN(builtin), STROBJ(ooptions));
	options = STRPTR(ooptions);
    }
    else
	options = NULL;

    if (otty != NIL) {
	if (!STRING_P(otty))
	    LispDestroy(mac, "%s: %s is not a string",
			STRFUN(builtin), STROBJ(otty));
	tty = STRPTR(otty);
    }
    else
	tty = NULL;

    if (odbname != NIL) {
	if (!STRING_P(odbname))
	    LispDestroy(mac, "%s: %s is not a string",
			STRFUN(builtin), STROBJ(odbname));
	dbname = STRPTR(odbname);
    }
    else
	dbname = NULL;

    if (ologin != NIL) {
	if (!STRING_P(ologin))
	    LispDestroy(mac, "%s: %s is not a string",
			STRFUN(builtin), STROBJ(ologin));
	login = STRPTR(ologin);
    }
    else
	login = NULL;

    if (opassword != NIL) {
	if (!STRING_P(opassword))
	    LispDestroy(mac, "%s: %s is not a string",
			STRFUN(builtin), STROBJ(opassword));
	password = STRPTR(opassword);
    }
    else
	password = NULL;

    conn = PQsetdbLogin(host, port, options, tty, dbname, login, password);

    return (conn ? OPAQUE(conn, PGconn_t) : NIL);
}

LispObj *
Lisp_PQsetdb(LispMac *mac, LispBuiltin *builtin)
/*
 pq-setdb host port options tty dbname
 */
{
    return (LispPQsetdb(mac, builtin, 0));
}

LispObj *
Lisp_PQsetdbLogin(LispMac *mac, LispBuiltin *builtin)
/*
 pq-setdb-login host port options tty dbname login password
 */
{
    return (LispPQsetdb(mac, builtin, 1));
}

LispObj *
Lisp_PQsocket(LispMac *mac, LispBuiltin *builtin)
/*
 pq-socket connection
 */
{
    int sock;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    sock = PQsocket(conn);

    return (INTEGER(sock));
}

LispObj *
Lisp_PQstatus(LispMac *mac, LispBuiltin *builtin)
/*
 pq-status connection
 */
{
    int status;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    status = PQstatus(conn);

    return (INTEGER(status));
}

LispObj *
Lisp_PQtty(LispMac *mac, LispBuiltin *builtin)
/*
 pq-tty connection
 */
{
    char *string;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    string = PQtty(conn);

    return (string ? STRING(string) : NIL);
}

LispObj *
Lisp_PQuser(LispMac *mac, LispBuiltin *builtin)
/*
 pq-user connection
 */
{
    char *string;
    PGconn *conn;

    LispObj *connection;

    connection = ARGUMENT(0);

    if (!CHECKO(connection, PGconn_t))
	LispDestroy(mac, "%s: cannot convert %s to PGconn*",
		    STRFUN(builtin), STROBJ(connection));
    conn = (PGconn*)(connection->data.opaque.data);

    string = PQuser(conn);

    return (string ? STRING(string) : NIL);
}

;;
;; Copyright (c) 2002 by The XFree86 Project, Inc.
;;
;; Permission is hereby granted, free of charge, to any person obtaining a
;; copy of this software and associated documentation files (the "Software"),
;; to deal in the Software without restriction, including without limitation
;; the rights to use, copy, modify, merge, publish, distribute, sublicense,
;; and/or sell copies of the Software, and to permit persons to whom the
;; Software is furnished to do so, subject to the following conditions:
;;
;; The above copyright notice and this permission notice shall be included in
;; all copies or substantial portions of the Software.
;;
;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
;; THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
;; WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
;; OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;; SOFTWARE.
;;
;; Except as contained in this notice, the name of the XFree86 Project shall
;; not be used in advertising or otherwise to promote the sale, use or other
;; dealings in this Software without prior written authorization from the
;; XFree86 Project.
;;
;; Author: Paulo César Pereira de Andrade
;;
;;
;; $XFree86: xc/programs/xedit/lisp/modules/progmodes/lisp.lsp,v 1.5 2002/11/17 07:51:29 paulo Exp $
;;

(require "syntax")
(require "indent")
(in-package "XEDIT")

(defsynprop *prop-special*
    "special"
    :font	"*courier-bold-r*12*"
    :foreground	"NavyBlue"
)

(defsynprop *prop-quote*
    "quote"
    :font	"*courier-bold-r*12*"
    :foreground	"Red3"
)

(defsynprop *prop-package*
    "package"
    :font	"*lucidatypewriter-medium-r*12*"
    :foreground	"Gold4"
)

(defsynprop *prop-unreadable*
    "unreadable"
    :font	"*courier-medium-r*12*"
    :foreground	"Gray25"
    :underline	t
)


	;;  This almost understands all the lisp syntax, but may be
	;; very slow as will always reparse entire function/macro bodies
	;;   (but it is good to find unmatched parenthesis, etc...)
	;; and there are some things to fix in Xaw to avoid too much
	;; redrawing, since all properties are removed when restarting
	;; the reparse of a region, Xaw will automatically mark some
	;; regions to be updated when the new entities are added.
#+notdef
(defsyntax *lisp-mode* :main nil nil nil
    (syntoken "(" :nospec t :property *prop-punctuation* :begin :cons)
    (syntoken ";" :nospec t :contained t :begin :simple-comment)
    (syntoken "\"" :nospec t :begin :string :contained t)
    (syntoken "#|" :nospec t :begin :comment :contained t)
    ;; number
    (syntoken "(\\<|\\s[+-]?)(\\d+\\.?(\\d*([SsFfDdLlEe][+-]?\\d+)?)?)"
	:property *prop-number*)
    ;; keyword
    (syntoken ":[][a-zA-Z0-9_?!+/<=>*%@${}&^~-]+" :property *prop-constant*)
    ;; special symbol
    (syntoken "\\*[][a-zA-Z0-9_?!+/<=>%@${}&^~-]+\\*" :property *prop-special*)
    ;; package name
    (syntoken "[][a-zA-Z0-9_?!+/<=>*%@${}&^~-]+::?" :property *prop-package*)
    ;; character constant
    (syntoken "#\\\\(\\W|\\w+(-\\w+)?)" :property *prop-constant*)
    (syntoken "|" :nospec t :begin :unreadable :contained t)

    (syntoken "." :nospec t :property *prop-quote*)
    (syntoken "'" :nospec t :property *prop-quote* :begin :quote)
    (syntoken "`" :nospec t :property *prop-quote* :begin :backquote)

    ;; Read time conditionals, evaluations and functions.
    (syntoken "#\\s*([+'cCsS-]|\\d+[aA])?" :begin :preprocessor :contained t)

    (syntable :cons nil nil
	;; Highlight car
	(syntoken "[][a-zA-Z0-9_?!+/<=>*%@${}&^~-]+"
	    :property *prop-keyword*
	    :begin :cdr)
	;; Empty list
	(syntoken ")" :nospec t :property *prop-punctuation* :switch -1)
	(syntoken "[^(#\"',;|:`]|$" :begin :cdr)

	(syntable :cdr nil nil
	    (syntoken ")" :nospec t :property *prop-punctuation* :switch -2)
	    (synaugment :main)
	)

	(synaugment :main)
    )

    (syntable :simple-comment *prop-comment* nil
	(syntoken "$" :switch -1)
	(syntoken "XXX|FIXME|TODO" :property *prop-annotation*)
    )

    (syntable :string *prop-string* nil
	(syntoken "\\\\.")
	(syntoken "\"" :nospec t :switch -1)
    )

    (syntable :comment *prop-comment* nil
	(syntoken "#|" :nospec t :begin :comment)
	(syntoken "|#" :nospec t :switch -1)
	(syntoken "XXX|FIXME|TODO" :property *prop-annotation*)
    )

    (syntable :quote nil nil
	(syntoken "(" :nospec t :property *prop-quote* :begin :quote-expression)
	(syntoken "[][a-zA-Z0-9_?!+/<=>*:%@${}&^~-]+" :switch -1)

	(syntable :quote-expression nil nil
	    (syntoken "(" :nospec t :property *prop-quote* :begin :quote-recursive)
	    (syntoken ")" :nospec t :property *prop-quote* :switch -2)
	    (syntable :quote-recursive nil nil
		(syntoken "(" :nospec t :property *prop-quote* :begin :quote-recursive)
		(syntoken ")" :nospec t :property *prop-quote* :switch -1)
		(synaugment :main)
	    )
	    (synaugment :main)
	)
	(synaugment :main)
    )

    (syntable :backquote nil nil
	(syntoken "(" :nospec t :property *prop-quote* :begin :backquote-expression)
	(syntoken "'|,@?" :property *prop-quote*)
	(syntoken "[][a-zA-Z0-9_?!+/<=>*:%@${}&^~-]+" :switch -1)

	(syntable :backquote-expression nil nil
	    (syntoken ",@?" :property *prop-quote*)
	    (syntoken "(" :nospec t :property *prop-quote* :begin :backquote-recursive)
	    (syntoken ")" :nospec t :property *prop-quote* :switch -2)
	    (syntable :backquote-recursive nil nil
		(syntoken "(" :nospec t :property *prop-quote* :begin :backquote-recursive)
		(syntoken "'|,@?" :property *prop-quote*)
		(syntoken ")" :nospec t :property *prop-quote* :switch -1)
		(synaugment :main)
	    )
	    (synaugment :main)
	)
	(synaugment :main)
    )

    (syntable :preprocessor *prop-preprocessor* nil
	(syntoken ";" :nospec t :begin :simple-comment :contained t)
	(syntoken "#|" :nospec t :begin :comment :contained t)
	(syntoken "[][a-zA-Z0-9_?!+/<=>*:%@${}&^~-]+" :switch -1)
	(syntoken "(" :nospec t :contained t :begin :preprocessor-expression)

	(syntable :preprocessor-expression *prop-preprocessor* nil
	    (syntoken "(" :nospec t :contained t :begin :preprocessor-recursive)
	    (syntoken ")" :nospec t :switch -2)

	    (syntable :preprocessor-recursive *prop-preprocessor* nil
		(syntoken "(" :nospec t :contained t :begin :preprocessor-recursive)
		(syntoken ")" :nospec t :switch -1)
	    )
	)
    )

    (syntable :unreadable *prop-unreadable* nil
	(syntoken "\\\\.")
	(syntoken "|" :nospec t :switch -1)
    )

    (syntoken "\\c" :property *prop-control*)
    ;; unbalanced
    (syntoken ")" :nospec t :property *prop-error*)
)


	;; This version isn't also optimal, but it's performance is
	;; acceptable.
(defsyntax *lisp-mode* :main nil #'default-indent nil
    ;; highlight car and parenthesis
    (syntoken "\\(+\\s*[][{}A-Za-z_0-9!$%&/<=>?^~*:+-]*\\)*"
	:property *prop-keyword*)
    (syntoken "\\)+" :property *prop-keyword*)

    ;; nil and t
    (syntoken "\\<(nil|t)\\>" :icase t :property *prop-special*)

    (syntoken "|" :nospec t :begin :unreadable :contained t)

    ;; keywords
    (syntoken ":[][{}A-Za-z_0-9!$%&/<=>^~+-]+" :property *prop-constant*)

    ;; special symbol.
    (syntoken "\\*[][{}A-Za-z_0-9!$%&7=?^~+-]+\\*"
	:property *prop-special*)

    ;; special identifiers
    (syntoken "&(aux|key|optional|rest)\\>" :icase t :property *prop-constant*)

    ;; numbers
    (syntoken
	;; since lisp is very liberal in what can be a symbol, this pattern
	;; will not always work as expected, since \< and \> will not properly
	;; work for all characters that may be in a symbol name
	(string-concat
	    "(\\<|[+-])\\d+("
		;; integers
		"(\\>|\\.(\\s|$))|"
		;; ratios
		"/\\d+\\>|"
		;;floats
		"\\.?\\d*([SsFfDdLlEe][+-]?\\d+)?\\>"
	    ")"
	)
	:property *prop-number*)

    ;; characters
    (syntoken "#\\\\(\\W|\\w+(-\\w+)?)" :property *prop-constant*)

    ;; quotes
    (syntoken "[`'.]|,@?" :property *prop-quote*)

    ;; package names
    (syntoken "[A-Za-z_0-9%-]+::?" :property *prop-package*)

    ;; read time evaluation
    (syntoken "#\\d+#" :property *prop-preprocessor*)
    (syntoken "#([+'cCsS-]|\\d+[aA=])?" :begin :preprocessor :contained t)

    (syntoken "\\c" :property *prop-control*)

    ;; symbols, do nothing, just resolve conflicting matches
    (syntoken "[][{}A-Za-z_0-9!$%&/<=>^~:*+-]+")

    (syntable :simple-comment *prop-comment* nil
	(syntoken "$" :switch -1)
	(syntoken "XXX|FIXME|TODO" :property *prop-annotation*)
    )

    (syntable :comment *prop-comment* nil
	;; comments can nest
	(syntoken "#|" :nospec t :begin :comment)
	;;  return to previous state
	(syntoken "|#" :nospec t :switch -1)
	(syntoken "XXX|FIXME|TODO" :property *prop-annotation*)
    )

    (syntable :unreadable *prop-unreadable* nil
	;; ignore escaped characters
	(syntoken "\\\\.")
	(syntoken "|" :nospec t :switch -1)
    )

    (syntable :string *prop-string* nil
	;; ignore escaped characters
	(syntoken "\\\\.")
	(syntoken "\"" :nospec t :switch -1)
    )

    (syntable :preprocessor *prop-preprocessor* nil
	;; a symbol
	(syntoken "[][{}A-Za-z_0-9!$%&/<=>^~:*+-]+" :switch -1)

	;; conditional expression
	(syntoken "(" :nospec t :begin :preprocessor-expression :contained t)

	(syntable :preprocessor-expression *prop-preprocessor* nil
	    ;; recursive
	    (syntoken "(" :nospec t :begin :preprocessor-recursive :contained t)
	    (syntoken ")" :nospec t :switch -2)

	    (syntable :preprocessor-recursive *prop-preprocessor* nil
		(syntoken "(" :nospec t
		    :begin :preprocessor-recursive
		    :contained t)
		(syntoken ")" :nospec t :switch -1)

		(synaugment :comments-and-strings)
	    )

	    (synaugment :comments-and-strings)
	)

	(synaugment :comments-and-strings)
    )

    (syntable :comments-and-strings nil nil
	(syntoken "\"" :nospec t :begin :string :contained t)
	(syntoken "#|" :nospec t :begin :comment :contained t)
	(syntoken ";" :begin :simple-comment :contained t)
    )

    (synaugment :comments-and-strings)
)

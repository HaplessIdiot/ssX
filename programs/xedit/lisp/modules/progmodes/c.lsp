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
;; $XFree86: xc/programs/xedit/lisp/modules/progmodes/c.lsp,v 1.6 2002/09/15 21:32:35 paulo Exp $
;;

;;  Uncomment this to run the module in the stand alone command line
;; interpreter. After uncommenting, in the shell run something like:
;;	$ ./lsp c.lsp < somefile.c
;;
;;	(setq *features* (append *features* '(:debug :debug-verbose)))


(require "syntax")
(in-package "XEDIT")

(defsynprop *prop-format*
    "format"
    :font	"*lucidatypewriter-medium-r*12*"
    :foreground	"RoyalBlue2"
    :underline	t
)

(defsyntax *c* "C/C++" :main nil

    ;;  All recognized C keywords.
    (syntoken
	(string-concat
	    "\\<("
	    "asm|auto|break|case|catch|char|class|const|continue|default|"
	    "delete|do|double|else|enum|extern|float|for|friend|goto|if|"
	    "inline|int|long|new|operator|private|protected|public|register|"
	    "return|short|signed|sizeof|static|struct|switch|template|this|"
	    "throw|try|typedef|union|unsigned|virtual|void|volatile|while"
	    ")\\>"
	)
	:property *prop-keyword*)

    ;;  Integers.
    (syntoken "\\<(\\d+|0x\\x+)(u|ul|ull|l|ll|lu|llu)?\\>"
	:icase t
	:property *prop-number*)

    ;;  Floating point numbers.
    (syntoken "\\<(\\d+\\.?\\d*|\\d*\\.\\d+)(e[+-]?\\d+)?[lf]?\\>"
	:icase t
	:property *prop-number*)


    ;;  Parentheses start rule.
#+debug-complex
    (syntoken "("
	:nospec t
	:property *prop-punctuation*
	:begin :parentheses)

    ;;  Braces start rule.
#+debug-complex
    (syntoken "{"
	:nospec t
	:property *prop-punctuation*
	:begin :braces)

    ;;  String start rule.
    (syntoken "\""
	:nospec t
	:begin :string
	:contained t)

    ;;  Character start rule.
    (syntoken "'"
	:nospec t
	:begin :character
	:contained t)

    ;;  Brackets start rule.
#+debug-complex
    (syntoken "["
	:nospec t
	:property *prop-punctuation*
	:begin :brackets)

    ;;  Preprocessor start rule.
    (syntoken "^\\s*#\\s*\\w+"
	:begin :preprocessor
	:contained t)

    ;;  Comment start rule.
    (syntoken "/*"
	:nospec t
	:begin :comment
	:contained t)

    ;;  C++ style comments.
    (syntoken "//.*"
	:property *prop-comment*)

    ;;  Punctuation, match two at the same time if possible, but no more to
    ;; avoid matching things like /** or ///.
#+debug-complex
    (syntoken "[/*+:;=<>,&.!%|^~?-][*+:;=<>,&.!%|^~?-]?"
	:property *prop-punctuation*)
#-debug-complex
    (syntoken "[][(){}/*+:;=<>,&.!%|^~?-][][(){}*+:;=<>,&.!%|^~?-]?"
	:property *prop-punctuation*)


    ;;  Rules for comments.
    (syntable :comment *prop-comment*

	;;  Match nested comments as an error.
	(syntoken "/*"
	    :nospec t
	    :property *prop-error*)

	(syntoken "XXX|TODO|FIXME"
	    :property *prop-annotation*)

	;;  Rule to finish a comment.
	(syntoken "*/"
	    :nospec t
	    :switch -1)
    )

    ;;  Rules for strings.
    (syntable :string *prop-string*

	;;  Ignore escaped characters, this includes \".
	(syntoken "\\\\.")

	;;  Match, most, printf arguments.
	(syntoken "%%|%([+-]?\\d+)?(l?[deEfgiouxX]|[cdeEfgiopsuxX])"
	    :property *prop-format*)

	;;  Ignore continuation in the next line.
	(syntoken "\\\\$")

	;;  Rule to finish a string.
	(syntoken "\""
	    :nospec t
	    :switch -1)

	;;  Don't allow strings continuing in the next line.
	(syntoken ".?$"
	    :begin :error)
    )

    ;;  Rules for characters.
    (syntable :character *prop-constant*

	;;  Ignore escaped characters, this includes \'.
	(syntoken "\\\\.")

	;;  Ignore continuation in the next line.
	(syntoken "\\\\$")

	;;  Rule to finish a character constant.
	(syntoken "'"
	    :nospec t
	    :switch -1)

	;;  Don't allow constants continuing in the next line.
	(syntoken ".?$"
	    :begin :error)
    )

    ;;  Rules for preprocessor.
    (syntable :preprocessor *prop-preprocessor*

	;;  Preprocessor includes comments.
	(syntoken "/*"
	    :nospec t
	    :begin :comment
	    :contained t)

	;;  Ignore lines finishing with a backslash.
	(syntoken "\\\\$")

	;;  Return to previous state if end of line found.
	(syntoken ".?$"
	    :switch -1)
    )

    ;;  Rules for parenthesis.
#+debug-complex
    (syntable :parentheses nil
	(syntoken ")"
	    :nospec t
	    :property *prop-punctuation*
	    :switch -1)

	;;  Unbalanced.
	(syntoken "[]}]"
	    :property *prop-error*)

	;;  Expressions in parentheses can include everything.
	(synaugment :main)
    )

    ;;  Rules for brackets.
#+debug-complex
    (syntable :brackets nil
	(syntoken "]"
	    :nospec t
	    :property *prop-punctuation*
	    :switch -1)

	;;  Unbalanced.
	(syntoken "[)}]"
	    :property *prop-error*)

	;;  Expressions in brackets can include everything.
	(synaugment :main)
    )

    ;;  Rules for braces.
#+debug-complex
    (syntable :braces nil
	(syntoken "}"
	    :nospec t
	    :property *prop-punctuation*
	    :switch -1)

	;;  Unbalanced.
	(syntoken "[])]"
	    :property *prop-error*)

	;;  Expressions in braces can include everything.
	(synaugment :main)
    )

    (syntable :error *prop-error*
	(syntoken "^.*$"
	    :switch -2)
    )

    ;;  This is an error when parsing the entire file, but normal
    ;; when interactively parsing small portions of the file.
#+debug-complex
    (syntoken "[])}]"
	:property *prop-punctuation*)

    (syntoken "\\c"
	:property *prop-control*)
)

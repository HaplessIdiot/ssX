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
;; $XFree86: xc/programs/xedit/lisp/modules/progmodes/c.lsp,v 1.4 2002/08/25 02:48:32 paulo Exp $
;;

;;  Uncomment this to run the module in the stand alone command line
;; interpreter. After uncommenting, in the shell run something like:
;;	$ ./lsp c.lsp < somefile.c
;;
;;	(setq *FEATURES* (append *FEATURES* '(:DEBUG :DEBUG-VERBOSE)))


(require "syntax")


;;  Uncomment this to run the command line version with other lisp.
;;  It will not do everything, but these stubs will allow it running
;; under clisp.
	#|
#-xedit
(defun xedit::RE-COMP (pattern &key nospec icase nosub newline)
    pattern
)
#-xedit
(defun xedit::RE-EXEC (regex string &key count start end notbol noteol)
    (list (cons 0 (length string)))
)
#-xedit
(defmacro xedit::WHILE (test &rest body)
    `(loop
	(unless (,@test) (return))
	,@body
    )
)
#-xedit
(defmacro xedit::UNTIL (test &rest body)
    `(loop
	(when (,@test) (return))
	,@body
    )
)
	|#

(defsynprop *PROP-FORMAT*
    "format"
    :FONT	"*lucidatypewriter-medium-r*12*"
    :FOREGROUND	"RoyalBlue2"
    :UNDERLINE	T
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Sample definition for the C language.
;;  This definition is basically what will (or is expected to) be
;; the contents of a syntax highlight rules definition file. Note that
;; only one variable (named *C*) is created.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defsyntax *C* "C/C++" :MAIN NIL

    ;;  All recognized C keywords.
    (syntoken
	;;  For this sample put all keywords in a single regex, but to
	;; be safe, a regex should not have more than 256 bytes.
	(string-concat
	    "\\<("
	    "asm|auto|break|case|catch|char|class|const|continue|default|"
	    "delete|do|double|else|enum|extern|float|for|friend|goto|if|"
	    "inline|int|long|new|operator|private|protected|public|register|"
	    "return|short|signed|sizeof|static|struct|switch|template|this|"
	    "throw|try|typedef|union|unsigned|virtual|void|volatile|while"
	    ")\\>"
	)
	:PROPERTY *PROP-KEYWORD*)

    ;;  Integers.
    (syntoken "\\<(\\d+|0x\\x+)(u|ul|ull|l|ll|lu|llu)?\\>"
	:ICASE T
	:PROPERTY *PROP-NUMBER*)

    ;;  Floating point numbers.
    (syntoken "\\<(\\d+\\.?\\d*|\\d*\\.?\\d+)(e[+-]?\\d+)?[lLfF]?\\>"
	:ICASE T
	:PROPERTY *PROP-NUMBER*)

    ;;  Parentheses start rule.
    (syntoken "("
	:NOSPEC T
	:PROPERTY *PROP-PUNCTUATION*
	:BEGIN :PARENTHESES)

    ;;  Braces start rule.
    (syntoken "{"
	:NOSPEC T
	:PROPERTY *PROP-PUNCTUATION*
	:BEGIN :BRACES)

    ;;  String start rule.
    (syntoken "\""
	:BEGIN :STRING
	:CONTAINED T)

    ;;  Character start rule.
    (syntoken "'"
	:BEGIN :CHARACTER
	:CONTAINED T)

    ;;  Brackets start rule.
    (syntoken "["
	:NOSPEC T
	:PROPERTY *PROP-PUNCTUATION*
	:BEGIN :BRACKETS)

    ;;  Preprocessor start rule.
    (syntoken "^\\s*#\\s*[a-z]+"
	:BEGIN :PREPROCESSOR
	:CONTAINED T)

    ;;  Comment start rule.
    (syntoken "/*"
	:NOSPEC T
	:BEGIN :COMMENT
	:CONTAINED T)


    ;;  C++ style comments.
    (syntoken "//.*$"
	:PROPERTY *PROP-COMMENT*)

    ;;  Punctuation, match two at the same time if possible, but no more to
    ;; avoid matching things like /** or ///.
    (syntoken "[/*+:;=<>,&.!%|^~?-][/*+:;=<>,&.!%|^~?-]?"
	:PROPERTY *PROP-PUNCTUATION*)


    ;;  Rules for comments.
    (syntable :COMMENT *PROP-COMMENT*

	;;  Match nested comments as an error.
	(syntoken "/*"
	    :NOSPEC T
	    :PROPERTY *PROP-ERROR*)

	(syntoken "XXX|TODO|FIXME"
	    :PROPERTY *PROP-ANNOTATION*)

	;;  Rule to finish a comment.
	(syntoken "*/"
	    :NOSPEC T
	    :SWITCH -1)
    )

    ;;  Rules for strings.
    (syntable :STRING *PROP-STRING*

	;;  Ignore escaped characters, this includes \".
	(syntoken "\\\\.")

	;;  Match, most, printf arguments.
	(syntoken "%%|%([+-]?\\d+)?(l?[deEfgiouxX]|[cdeEfgiopsuxX])"
	    :PROPERTY *PROP-FORMAT*)

	;;  Rule to finish a string.
	(syntoken "\""
	    :SWITCH -1)
    )

    ;;  Rules for characters.
    (syntable :CHARACTER *PROP-CONSTANT*

	;;  Ignore escaped characters, this includes \'.
	(syntoken "\\\\.")

	;;  Rule to finish a character constant.
	(syntoken "'"
	    :SWITCH -1)
    )

    ;;  Rules for preprocessor.
    (syntable :PREPROCESSOR *PROP-PREPROCESSOR*

	;;  Preprocessor includes comments.
	(syntoken "/*"
	    :NOSPEC T
	    :BEGIN :COMMENT
	    :CONTAINED T)

	;;  Ignore lines finishing with a backslash.
	(syntoken "\\\\$")

	;;  Return to previous state if end of line found.
	(syntoken "$"
	    :SWITCH -1)
    )

    ;;  Rules for parenthesis.
    (syntable :PARENTHESES NIL
	(syntoken ")"
	    :NOSPEC T
	    :PROPERTY *PROP-PUNCTUATION*
	    :SWITCH -1)

	;;  Unbalanced.
	(syntoken "[]}]"
	    :PROPERTY *PROP-ERROR*)

	;;  Expressions in parentheses can include everything.
	(synaugment :MAIN)
    )

    ;;  Rules for brackets.
    (syntable :BRACKETS NIL
	(syntoken "]"
	    :NOSPEC T
	    :PROPERTY *PROP-PUNCTUATION*
	    :SWITCH -1)

	;;  Unbalanced.
	(syntoken "[)}]"
	    :PROPERTY *PROP-ERROR*)

	;;  Expressions in brackets can include everything.
	(synaugment :MAIN)
    )

    ;;  Rules for braces.
    (syntable :BRACES NIL
	(syntoken "}"
	    :NOSPEC T
	    :PROPERTY *PROP-PUNCTUATION*
	    :SWITCH -1)

	;;  Unbalanced.
	(syntoken "[])]"
	    :PROPERTY *PROP-ERROR*)

	;;  Expressions in keys can include everything.
	(synaugment :MAIN)
    )


    ;;  Unbalanced.
    ;;  This rule should be added only for this sample, or to
    ;; a very smart version of the parser, or used only when
    ;; parsing the entire file.
    (syntoken "[])}]"
	:PROPERTY *PROP-ERROR*)
)

(compile 'syntax-highlight)
#+debug (syntax-highlight *C*)

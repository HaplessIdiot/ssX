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
;; $XFree86: xc/programs/xedit/lisp/modules/progmodes/c.lsp,v 1.10 2002/10/20 05:58:55 paulo Exp $
;;

(require "syntax")
(in-package "XEDIT")

(defsynprop *prop-format*
    "format"
    :font	"*lucidatypewriter-medium-r*12*"
    :foreground	"RoyalBlue2"
    :underline	t
)

;; The default values are my preferred style. It also should match
;; a "java" style, with a base indentation of 4 spaces, and adding one
;; indent level to case/default labels.
(defsynoptions *c-DEFAULT-style*
    ;; Positive number. Basic indentation.
    (:indentation		.	4)

    ;; Boolean. Support for GNU style indentation.
    (:brace-indent		.	nil)

    ;; Boolean. Add one indentation level to case and default?
    (:case-indent		.	t)

    ;; Boolean. Remove one indentation level for labels?
    (:label-dedent		.	t)

    ;; Boolean. Add one indentation level to continuations?
    (:cont-indent		.	t)

    ;; Boolean. Move cursor to the indent column after pressing <Enter>?
    (:newline-indent		.	nil)

    ;; Boolean. Set to T if tabs shouldn't be used to fill indentation.
    (:emulate-tabs		.	nil)

    ;; Boolean. Force a newline before braces?
    (:newline-before-brace	.	nil)

    ;; Boolean. Force a newline after braces?
    (:newline-after-brace	.	nil)

    ;; Boolean. Force a newline after semicolons?
    (:newline-after-semi	.	nil)

    ;; Boolean. Only calculate indentation after pressing <Enter>?
    ;;		This may be useful if the parser does not always
    ;;		do what the user expects...
    (:only-newline-indent	.	nil)

    ;; Boolean. Remove extra spaces from previous line.
    ;;		This should default to T when newline-indent is not NIL.
    (:trim-blank-lines		.	nil)

    ;; Boolean. If this hash-table entry is set, no indentation is done.
    ;;		Useful to temporarily disable indentation.
    (:disable-indent		.	nil)
)

;; BSD like style
(defsynoptions *c-BSD-style*
    (:indentation		.	8)
    (:brace-indent		.	nil)
    (:case-indent		.	nil)
    (:label-dedent		.	t)
    (:cont-indent		.	t)
    (:newline-indent		.	t)
    (:emulate-tabs		.	nil)
    (:newline-before-brace	.	nil)
    (:newline-after-brace	.	t)
    (:newline-after-semi	.	t)
    (:trim-blank-lines		.	t)
)

;; GNU like style
(defsynoptions *c-GNU-style*
    (:indentation		.	2)
    (:brace-indent		.	t)
    (:case-indent		.	nil)
    (:label-dedent		.	t)
    (:cont-indent		.	t)
    (:newline-indent		.	nil)
    (:emulate-tabs		.	nil)
    (:newline-before-brace	.	t)
    (:newline-after-brace	.	t)
    (:newline-after-semi	.	t)
    (:trim-blank-lines		.	nil)
)

;; K&R like style
(defsynoptions *c-K&R-style*
    (:indentation		.	5)
    (:brace-indent		.	nil)
    (:case-indent		.	nil)
    (:label-dedent		.	t)
    (:cont-indent		.	t)
    (:newline-indent		.	t)
    (:emulate-tabs		.	t)
    (:newline-before-brace	.	t)
    (:newline-after-brace	.	t)
    (:newline-after-semi	.	t)
    (:trim-blank-lines		.	t)
)

(defvar *c-styles* '(
    ("xedit"	.	*c-DEFAULT-style*)
    ("BSD"	.	*c-BSD-style*)
    ("GNU"	.	*c-GNU-style*)
    ("K&R"	.	*c-K&R-style*)
))

(defvar *c-mode-options* *c-DEFAULT-style*)
; (setq *c-mode-options* *c-bsd-style*)

;;(defsyntax *c-mode* :main nil #+debug #'c-indent #-debug nil *c-mode-options*
(defsyntax *c-mode* :main nil #'c-indent *c-mode-options*
    ;;  All recognized C keywords.
    (syntoken
	(string-concat
	    "\\<("
	    "asm|auto|break|case|catch|char|class|const|continue|default|"
	    "delete|do|double|else|enum|extern|float|for|friend|goto|if|"
	    "inline|int|long|new|operator|private|protected|public|register|"
	    "return|short|signed|sizeof|static|struct|switch|template|this|"
	    "throw|try|typedef|union|unsigned|virtual|void|volatile|while"
	    ")\\>")
	:property *prop-keyword*)

    ;; Numbers, this is optional, comment this rule if xedit is
    ;; too slow to load c files.
    (syntoken
	(string-concat
	    "\\<("
	    ;; Integers
	    "(\\d+|0x\\x+)(u|ul|ull|l|ll|lu|llu)?|"
	    ;; Floats
	    "\\d+\\.?\\d*(e[+-]?\\d+)?[lf]?"
	    ")\\>")
	:icase t
	:property *prop-number*
    )

    ;; String start rule.
    (syntoken "\"" :nospec t :begin :string :contained t)

    ;; Character start rule.
    (syntoken "'" :nospec t :begin :character :contained t)

    ;; Preprocessor start rule.
    (syntoken "^\\s*#\\s*\\w+" :begin :preprocessor :contained t)

    ;; Comment start rule.
    (syntoken "/*" :nospec t :begin :comment :contained t)

    ;; C++ style comments.
    (syntoken "//.*" :property *prop-comment*)

    ;; Punctuation, this is also optional, comment this rule if xedit is
    ;; too slow to load c files.
    (syntoken "[][(){}/*+:;=<>,&.!%|^~?-][][(){}*+:;=<>,&.!%|^~?-]?"
	:property *prop-punctuation*)


    ;; Rules for comments.
    (syntable :comment *prop-comment* nil
	;; Match nested comments as an error.
	(syntoken "/*" :nospec t :property *prop-error*)

	(syntoken "XXX|TODO|FIXME" :property *prop-annotation*)

	;;  Rule to finish a comment.
	(syntoken "*/" :nospec t :switch -1)
    )

    ;; Rules for strings.
    (syntable :string *prop-string* nil
	;; Ignore escaped characters, this includes \".
	(syntoken "\\\\.")

	;; Match, most, printf arguments.
	(syntoken "%%|%([+-]?\\d+)?(l?[deEfgiouxX]|[cdeEfgiopsuxX])"
	    :property *prop-format*)

	;; Ignore continuation in the next line.
	(syntoken "\\\\$")

	;; Rule to finish a string.
	(syntoken "\"" :nospec t :switch -1)

	;; Don't allow strings continuing in the next line.
	(syntoken ".?$" :begin :error)
    )

    ;; Rules for characters.
    (syntable :character *prop-constant* nil
	;; Ignore escaped characters, this includes \'.
	(syntoken "\\\\.")

	;; Ignore continuation in the next line.
	(syntoken "\\\\$")

	;; Rule to finish a character constant.
	(syntoken "'" :nospec t :switch -1)

	;; Don't allow constants continuing in the next line.
	(syntoken ".?$" :begin :error)
    )

    ;;  Rules for preprocessor.
    (syntable :preprocessor *prop-preprocessor* nil
	;;  Preprocessor includes comments.
	(syntoken "/*" :nospec t :begin :comment :contained t)

	;;  Ignore lines finishing with a backslash.
	(syntoken "\\\\$")

	;; Return to previous state if end of line found.
	(syntoken ".?$" :switch -1)
    )

    (syntable :error *prop-error* nil
	(syntoken "^.*$" :switch -2)
    )

    ;;  You may also want to comment this rule if the parsing is
    ;; noticeably slow.
    (syntoken "\\c" :property *prop-control*)
)


(defconstant c-spaces '(#\Space #\Page #\Return #\Vt))
(defconstant c-tab+spaces (cons #\Tab c-spaces))

;; Assumes the argument `from' is an offset inside (or at the beginning)
;; of the string or character and returns the offset of the start of the
;; matching character. Example:
;;   "some string"
;;   ^		^
;;   |		from
;;   offset
(defun c-char-match (from char &aux (offset from) (pattern (string char)))
    (while
	(and
	    (setq offset (search-backward pattern offset))
	    (> offset (point-min))
	    ;; keep looping if there is a backslash before the character
	    ;; XXX this check isn't completely safe
	    (char= #\\ (char-before offset))
	)
	(decf offset 2)
    )
    offset
)

;; Calculate the indentation to align with character at given offset
(defun c-offset-align (offset &aux
		       (start (scan offset :eol :left))
		       (line (read-text start (- offset start)))
		       (indent 0))
    (dotimes (i (length line) indent)
	(if (char= (char line i) #\Tab)
	    (incf indent (- 8 (rem indent 8)))
	    (incf indent)
	)
    )
)

;; Calculate indentation starting at `start'
;; Assumes start is the offset of the first character in a line
;; Returns 3 values:
;;	o indentation
;;	o relative offset of first non-blank character
;;	o first non-blank character, newline if empty line, or nil if at eof
(defun c-offset-indent (start &aux (index 0) (indent 0) char)
    (loop
	(setq char (char-after (+ start index)))
	(cond
	    ;; eof reached
	    ((null char)
		(return)
	    )
	    ;; a blank character
	    ((member char c-spaces)
		(incf indent)
	    )
	    ;; adjust tabulation
	    ((char= char #\Tab)
		(incf indent (- 8 (rem indent 8)))
	    )
	    ;; newline or a non blank character
	    (t (return))
	)
	(incf index)
    )
    (values indent index char)
)

;; The `char' argument must be either ), ], }, or >. The code doesn't
;; try to flag errors if something is unbalanced, just skips over comments,
;; strings, preprocessor lines and character. If a match is not found,
;; returns nil.
;; Assumes that the `from' is not inside a comment, preprocessor, string,
;; or character.
(defun c-exp-match (from match
		    &aux char count offset line index open start adjust)
    (setq
	count	1
	offset	(scan from :eol :left)
	;; add -1 to read the matching character when the expression is empty
	line	(read-text offset (- from offset -1))
	index	(1- (length line))
	;; < and > aren't currently used, but should at some time,
	;; to properly parse c++ templates.
	open	(case match (#\) #\() (#\] #\[) (#\} #\{) (#\> #\<))
    )

    (loop
	;; check if need to read a new line of text
	(while (minusp index)
	    (if adjust
		(setq offset (scan from :eol :left) adjust nil)
		(progn
		    (setq start offset)
		    (multiple-value-setq
			(offset from)
			(c-top-match (scan from :eol :left :count 2))
		    )
		    ;; if cannot find a safe point to restart parsing
		    (and (= start offset) (return (setq from nil)))
		)
	    )
	    ;; rebuild loop state
	    (setq
		line   (read-text offset (- from offset))
		index  (1- (length line))
	    )
	)

	(setq char (char line index))
	(cond
	    ;; found the matching (, [, {, or <
	    ((char= char open)
		(if (zerop (decf count))
		    ;; success
		    (return (setq from (+ offset index)))
		)
	    )
	    ;; found another ), ], }, or <
	    ((char= char match)
		(incf count)
	    )
	    ;; found a comment?
	    ((and (char= char #\/)
		  (plusp index)
		  (char= (char line (1- index)) #\*))
		(setq from (search-backward "/*" (+ offset (- index 2))))
		;; should never be true, cannot find start of comment
		(unless (integerp from)
		    (return)
		)
		(setq  adjust t)
	    )
	    ;; found a string or character
	    ((member char '(#\" #\'))
		(setq from (c-char-match (+ offset (1- index)) char))
		;; should never be true, cannot find start of string/character
		(unless (integerp from)
		    (return)
		)
		(setq adjust t)
	    )
	)

	(if adjust
	    (if (>= from offset)
		(setq adjust nil index (- from offset 1))
		(setq index -1)
	    )
	    (decf index)
	)
    )
    from
)

;; This function is used when parsing backwards the file.
;; The text line starting at `from' may:
;;	o end in a c++ style comment
;;	o be a preprocessor line
;;	o be the last line of a multiline preprocessor directive
;; Variables `left' and `right' are used as a temporary value for
;; the result, because there may be backslashes ending lines that
;; aren't preprocessor directives, or comments/strings/characters
;; that aren't part of a preprocessor directive.
(defun c-top-match (from &aux char to line index left right adjust)
    (setq
	to	(scan from :eol :right)
	left	from
	right	to
	line	(read-text left (- right left))
	index	(1- (length line))
    )

    (loop
	(when (minusp index)
	    (and
		(<=
		    (setq left (scan right :eol :left :count (if adjust 1 2)))
		    (point-min)
		)
		;; if start of file found return from loop
		(return)
	    )
	    (or adjust (setq right (scan left :eol :right)))
	    (if (char= (char-before right) #\\)
		;; if it is a continuation, maybe a multiline preprocessor
		(progn
		    (setq
			line	(read-text left (- right left))
			index	(1- (length line))
		    )
		    ;; empty line before continuation
		    (and (minusp index) (return))
		)
		;; else a safe point was found
		;; XXX maybe should check for spaces after a backslash
		(return)
	    )
	    (setq adjust nil)
	)

	(case (setq char (char line index))
	    ;; maybe a comment
	    (#\/
		(when (plusp index)
		    (case (char line (decf index))
			;; c++ comment
			(#\/	(setq to (+ left (1- index)) right to adjust t))
			;; normal comment
			(#\*
			    (setq
				right (search-backward "/*" (+ left (1- index)))
				adjust t
			    )
			)
		    )
		)
	    )
	    ;; preprocessor directive; XXX don't flag an error if this
	    ;; is not the first non blank character in the line?
	    (#\#
		(setq
		    to (if (plusp index) (+ left (1- index)) left)
		    right to
		    adjust t
		)
	    )
	    ;; string or character
	    ((#\" #\')
		(setq right (c-char-match (+ left (1- index)) char) adjust t)
	    )
	)

	(or right (return))

	(if adjust
	    (if (>= right left)
		(setq adjust nil index (- right left 1))
		(setq index -1)
	    )
	    (decf index)
	)
    )

    ;;  if `to' is smaller than `from', a c++ comment or preprocessor
    ;; directive, possibly multiline, was found
    (and (< to from) (setq from left))

    (values from to)
)

;; Helper function. If line starts in a toplevel state, return it's
;; indentation, nil otherwise.
(defun c-indentation (offset &aux indent index char)
    (when (= offset (scan offset :eol :left))
	(multiple-value-setq (indent index char) (c-offset-indent offset))
	(when (and (characterp char) (char/= char #\Newline))
	    (return-from c-indentation (values indent char))
	)
    )
)

;; This function is called in the toplevel syntax-table, so it knows
;; that the cursor is not inside a comment, preprocessor, string, or
;; character.
(defun c-indent (syntax syntable)
    (prog*
	(
	;; insert position in the file
	(point (point))

	;; start offset of current line
	(start (scan point :eol :left))

	;; start parsing this region
	(left start)
	(right point)

	;; hash table of options
	(options (syntax-options syntax))

	;; information for text before cursor
	(line (read-text left (- right left)))
	(index (1- (length line)))

	(patterns '(
	    (#.(re-comp "case|default")	. :case)
	    (#.(re-comp "if")		. :if)
	    ;; XXX same handling for now
	    (#.(re-comp "do|else")	. :else)
	    (#.(re-comp "for")		. :for)
	    (#.(re-comp "switch")	. :switch)
	    (#.(re-comp "while")	. :while)
	))

	;; current character read
	char

	;; temporary
	offset

	;; flag to readjust input text
	adjust

	;; temporary
	result

	;; indentation of current line
	current-indent

	;; offset of first non blank character in the current line
	current-offset

	;; parsed expressions, while building indentation information
	info

	;; flag indicating if current line starts in a toplevel state
	toplevel

	;; indentation to be used
	(indent 0)

	;; temporary flags
	test
	bols
	block
	flow
	switch
	cont

	;; parameters from options hash-table
	*indentation*
	*brace-indent*
	*case-indent*
	*cont-indent*
	*emulate-tabs*
	*newline-indent*
	*only-newline-indent*
	)

	(and
	    (or
		(not (hash-table-p options))
		(gethash :disable-indent options)
	    )
	    (return-from c-indent)
	)

	(setq
	    *newline-indent*	  (gethash :newline-indent options nil)
	    *only-newline-indent* (gethash :only-newline-indent options nil)
	)

	(or
	    (if (or *newline-indent* *only-newline-indent*)
		;; if at bol
		(= point start)
		;; or after first char typed
		(= (1- point) start)
	    )
	    ;; or one of these was typed
	    (member (char-before point) '(#\{ #\} #\; #\: #\] #\)))
	    ;; else save cpu time...
	    (return-from c-indent)
	)

	(setq
	    *indentation*	(gethash :indentation options 4)
	    *brace-indent*	(gethash :brace-indent options nil)
	    *case-indent*	(gethash :case-indent options t)
	    *cont-indent*	(gethash :cont-indent options t)
	    *emulate-tabs*	(gethash :emulate-tabs options nil)
	)

	;; parse until start of line is found
	(loop
	    ;; start of line found, line may be empty
	    (and (minusp index) (return))

	    ;; read a character
	    (setq char (char line index))

	    ;; parse the character
	    (cond
		;; check for the end of a comment
		((and (plusp index)
		      (char= char #\/)
		      (char= (char line (1- index)) #\*))
		    (setq right (search-backward "/*" (+ start (- index 2))))
		    (if (and (integerp right) (>= right start))
			;; skip the comment
			(setq index (- right start))
			;; failed to find start or is multiline
			(return-from c-indent)
		    )
		)
		;; check for end of string or character
		((member char '(#\" #\'))
		    ;; find start
		    (setq right (c-char-match (+ start (1- index)) char))
		    (if (and (integerp right) (>= right start))
			;; skip the string or character
			(setq index (- right start))
			;; failed to find start or is multiline
			(return-from c-indent)
		    )
		)
	    )
	    (decf index)
	)

	;; line starts in a toplevel state, continue parsing

	;; calculate current indentation and find first non blank character
	(multiple-value-setq
	    (current-indent index char)
	    (c-offset-indent start)
	)
	;; if should only indent after first char typed
	(if
	    (and
		(zerop index)
		(or (null char) (char= char #\Newline))
	    )
	    ;; line is empty
	    (if (and (not *newline-indent*) (not *only-newline-indent*))
		(return-from c-indent)
	    )
	    ;; line is not empty
	    (and *only-newline-indent* (return-from c-indent))
	)
	(setq current-offset (+ start index))

	;; if line starts with one of these characters, align with match
	(when (member char '(#\) #\]))
	    (setq right (c-exp-match (1- current-offset) char))
	    (if (integerp right)
		(progn
		    (setq indent (c-offset-align right))
		    (go :indent)
		)
		;; unbalanced
		(return-from c-indent)
	    )
	)

	;; build information to calculate indentation
	(setq index -1)
	(loop
	    (while (minusp index)
		(setq
		    offset left
		    left   (scan left :eol :left :count (if adjust 1 2))
		)

		;; failed to backup a line, start of file reached?
		(and (not adjust) (= offset left) (return))

		;; make sure input text is in a toplevel region
		(setq offset right)
		(multiple-value-setq (left right) (c-top-match left))
		;; don't reparse an skiped expression
		(when adjust
		    (and (> right offset) (setq right offset))
		    (setq adjust nil)
		)
		(when (>= right left)
		    (setq
			line	 (read-text left (- right left))
			index	 (1- (length line))
			toplevel (c-indentation left)
		    )
		)
	    )

	    (setq char (char line index))

	    (cond
		;; identifiers and numbers
		((or (alphanumericp char) (char= char #\_))
		    ;; offset of first non alphanumeric char
		    (setq offset (1+ index))

		    ;; skip alphanumeric characters
		    (while
			(and
			    (plusp index)
			    (setq char (char line (decf index)))
			    (or (alphanumericp char) (char= char #\_))
			)
		    )
		    ;; index points to the first alphanumeric char
		    (if (plusp index)
			(incf index)
			(unless (or (alphanumericp char) (char= char #\_))
			    (incf index)
			)
		    )

		    ;; check regex patterns
		    (dolist (pat patterns)
			(setq
			    result
			    (re-exec (car pat) line :start index :end offset)
			)
			;; if matched
			(when (and (consp result) (= (caar result) index))
			    ;; update state information
			    (push (cons (cdr pat) (+ left index)) info)
			    (return)
			)
		    )
		)

		;; string or character
		((member char '(#\" #\'))
		    (setq right (c-char-match (1- (+ left index)) char))
		    (if (integerp right)
			(setq adjust t)
			;; failed to find start
			(return-from c-indent)
		    )
		)

		;; maybe comment
		((char= char #\/)
		    (when
			(and (plusp index) (char= (char line (1- index)) #\*))
			(setq right (search-backward "/*" (+ left (- index 1))))
			(if (integerp right)
			    (setq adjust t)
			    ;; failed to find start of comment
			    (return-from c-indent)
			)
		    )
		)

		;; skip expression
		;; XXX if this becomes too slow, may need to not check for
		;; comments, strings, preprocessor, etc...
		((member char '(#\) #\] #\}))
		    (setq right (c-exp-match (1- (+ left index)) char))
		    (if (integerp right)
			(progn
			    (setq adjust t)
			    (push
				(cons
				    (cdr
					(assoc char '(
					    (#\) . :parentheses)
					    (#\] . :brackets)
					    (#\} . :braces))
					)
				    )
				    (+ left index)
				)
				info
			    )
			)
			;; failed to find start
			(return-from c-indent)
		    )
		)

		;; inside an expression, align with it
		((member char '(#\( #\[))
		    (setq indent (1+ (c-offset-align (+ left index))))
		    (go :indent)
		)

		;; skip spaces
		((member char c-tab+spaces)
		    (setq offset (+ left index 1))
		    (while
			(and
			    (plusp index)
			    (setq char (char line (decf index)))
			    (member char c-tab+spaces)
			)
		    )
		    ;; index points to the first non space character
		    (if (plusp index)
			(incf index)
			(unless (member char c-tab+spaces) (incf index))
		    )

		    ;; if:
		    ;;	o in a toplevel state
		    ;;	o at the start of non blank line
		    ;;	o looks like a continuation or start of expression
		    ;;	o not a comment line
		    ;; XXX won't properly handle code after comment
		    ;; in the same line.
		    (and
			toplevel
			(zerop index)
			(or (not info) (/= (cdar info) offset))
			(characterp (setq char (char-after offset)))
			(char/= char #\Newline)
			(or
			    ;; if first char is not /
			    (char/= char #\/)
			    ;; or if eof after /
			    (null (setq char (char-after (1+ offset))))
			    (and
				;; or * does not follow /
				(char/= char #\*)
				;; or / does not follow /
				(char/= char #\/)
			    )
			)
			(push (cons :bol offset) info)
		    )
		)

		;; block start
		((char= char #\{)
		    (push (cons :block (+ left index)) info)
		)

		;; if not ::
		;; XXX needs better handling to know if is a label or a bitfield
		((char= char #\:)
		    (and (or (zerop index) (char/= (char line (1- index)) char))
			(push (cons :collon (+ left index)) info)
		    )
		)

		;; characters that may also be used to resolve indentation
		((member char '(#\; #\, #\=))
		    (push
			(cons
			    (cdr
				(assoc char '(
				    (#\; . :expression)
				    (#\, . :comma)
				    (#\= . :assign)
				    )
				)
			    )
			    (+ left index)
			)
			info
		    )
		)
	    )

	    ;; check if a safe-point was found
	    ;;	A "safe-point" is an offset from where it is expected to
	    ;; be able to calculate the correct indentation for the cursor
	    ;; line based on the indentation of the "safe-point" and the
	    ;; information collected while looping.
	    (when (and toplevel (zerop index))
		;; if nothing matched, remember start of line offset
		(when
		    (and
			info
			;; not a continuation/start of expression
			(not (eq (caar info) :bol))
			(not (member (char line 0) c-tab+spaces))
			;; nothing recorded at bol offset
			(/= (cdar info) left)
		    )
		    (push (cons :bol left) info)
		)

		(setq result info test (caar result))
		(cond
		    ;; easy cases when line starts with any of these
		    ((member test '(:block :case :else) :test #'eq)
			(go :process)
		    )

		    ;; if line starts with any of these
		    ((member test '(:if :for :while :switch) :test #'eq)
			(setq result (cdr result))
			;; if expression inside parentheses does not follow
			(unless (eq (caar result) :parentheses)
			    (return-from c-indent)
			)
			(setq result (cdr result))
			;; expression is inside a block
			(and (eq (caar result) :block)
			    (go :process)
			)
		    )

		    ;; line starts at the middle or start of an expression
		    ((eq test :bol)
			(setq
			    bols 1 result (cdr result) test (caar result)
			)
			(loop
			    (cond
				;; "simple" code block
				((eq test :block)
				    (go :process)
				)

				((and
				    (eq test :bol)
				    (or
					(and
					    (null *case-indent*)
					    (null *cont-indent*)
					)
					(<=
					    (c-offset-align (cdar result))
					    *indentation*
					)
				    ))

				    (when (> (incf bols) 2)
					(pop info)
					(until (eq (caar info) :bol)
					    (pop info)
					)
					(go :process)
				    )
				)

				((member
				    test
				    '(:comma
				      :assign
				      :collon
				      :parenthesis
				      :brackets
				      :expression
				    )
				    :test #'eq)
				    ;; do nothing
				)

				;; anything else or end of list
				(t
				    (return)
				)
			    )
			    (setq result (cdr result) test (caar result))
			)
		    )
		)
	    )

	    (if adjust
		(if (>= (setq index (- right left 1)) 0)
		    ;; if didn't skip more than one line
		    (setq adjust nil)
		    (setq left right)
		)
		;; no adjusting required, just check previous character
		(decf index)
	    )
	)

;------------------------------------------------------------------------
; calculate indentation based on the collected information
:process
	;; at the start of the file?
	(and (null info) (setq indent 0) (go :indent))

	;; convert (:else ...) (:if ...) to (:else-if), for simplicity
	(setq result info)
	(while result
	    (and (eq (caar result) :else) (eq (caadr result) :if)
		(rplaca (car result) :else-if)
		(rplacd result (cddr result))
	    )
	    (setq result (cdr result))
	)

	;; initialize indent to the indentation of first item in info
	(setq indent (c-offset-align (cdar info)) flow 0 block 0 bols 0 switch 0)

	;; calculate indentation
	(dolist (item info)
	    (case (car item)
		(:case
		    (setq bols 0)
		    (unless (> switch 0)
			(setq switch 1 indent (+ indent *indentation*))
		    )
		)

		(:switch
		    (setq
			bols   0
			flow   (1+ flow)
			switch (+ switch flow)
			indent (+ indent *indentation*)
		    )
		    (and *case-indent* (incf indent *indentation*))
		)

		((:if :else-if :else :for :while)
		    (setq
			bols   0
			flow   (1+ flow)
			indent (+ indent *indentation*)
		    )
		)

		;; no change in indentation
		((:parentheses :brackets)
		    (setq bols 0)
		)

		(:braces
		    (when (> flow 0)
			(setq
			    offset nil
			    flow   (1- flow)
			    indent (- indent *indentation*)
			)
			(and (> switch 0) (>= switch flow) *case-indent*
			    (while (> switch 0)
				(setq
				    indent (- indent *indentation*)
				    switch (1- switch)
				)
			    )
			)
		    )
		    (setq bols 0)
		)

		;; open code block
		(:block
		    ;; if indentation not yet added
		    (if (= flow 0)
			;; increment one indentation level
			(setq indent (+ indent *indentation*))
			(progn
			    (setq flow (1- flow))
			    (and *brace-indent* (= switch 0)
				(incf indent *indentation*)
			    )
			)
		    )
		    (setq block 1 bols 0)
		)

		(:expression
		    (while (> flow 0)
			(setq
			    indent (- indent *indentation*)
			    flow   (1- flow)
			)
		    )
		    (setq bols 0)
		)

		(:bol
		    (incf bols)
		)

		((:comma :collon)
		    (setq bols 0)
		)

		(:assign
		)
	    )
	)

	(and *cont-indent* (> bols 0) (setq cont t))

	;; if line starts with { or }
	(when (characterp (setq char (char-after current-offset)))
	    (cond
		((char= char #\{)
		    (and (> flow 0) (decf indent *indentation*))
		    (and *brace-indent* (incf indent *indentation*))
		    (and (> switch 0) *brace-indent* *case-indent*
			(decf indent *indentation*)
		    )
		    (go :indent)
		)
		((char= char #\})
		    (decf indent *indentation*)
		    (and (> switch 0) *case-indent*
			(decf indent *indentation*)
		    )
		    (go :indent)
		)
	    )
	)

	;; check if need to remove one indentation level
	(let (lline lcase)
	    (setq
		right (scan current-offset :eol :right)
		line  (read-text current-offset (- right current-offset))
		lline (length line)
	    )

	    (setq test nil)
	    (dolist (case '("case" "default"))
		(setq lcase (length case))
		(and
		    (>= lline lcase)
		    (string= line case :end1 lcase)
		    (or
			(= lline lcase)
			(not
			    (or
				(char= (setq char (char line lcase)) #\_)
				(alphanumericp char)
			    )
			)
		    )
		    (decf indent *indentation*)
		    (return (setq test t))
		)
	    )

	    ;; indentation not decremented, check for a label
	    (when
		(and
		    (not test)
		    (or (> switch 0) (gethash :label-dedent options))
		)
		;; remove spaces
		(setq
		    line (string-trim c-tab+spaces line)
		    index (1- (length line))
		)
		;; check if looks like a label
		(when (and (plusp index) (char= (char line index) #\:))
		    (setq
			line
			(string-right-trim
			    c-tab+spaces
			    (subseq line 0 index)
			)

			index
			(1- (length line))
		    )
		    (when (plusp index)
			(while
			    (and
				(>= index 0)
				(setq char (char line index))
				(or
				    (alphanumericp char)
				    (char= char #\_)
				)
			    )
			    (decf index)
			)
			;; was a label
			(and (minusp index) (decf indent *indentation*))
		    )
		)
	    )
	)
	(and cont (> indent 0) (incf indent *indentation*))


;------------------------------------------------------------------------
; indent the current line
:indent
	(and (minusp indent) (setq indent 0))
	(when (/= indent current-indent)
	    (let
		(tabs spaces string)
		(if *emulate-tabs*
		    (setq
			index  indent
			offset (+ start index)
			string (make-string index :initial-element #\Space)
		    )
		    (progn
			(multiple-value-setq (tabs spaces) (floor indent 8))
			(setq
			    index  (+ tabs spaces)
			    offset (+ start index)
			    string (make-string index :initial-element #\Tab)
			)
			(fill string #\Space :start tabs)
		    )
		)
		(replace-text start current-offset string)
		(if (> current-offset point)
		    (goto-char offset)
		    (goto-char (- point (- current-offset start index)))
		)
	    )
	)

	;; last character typed
	(setq point (point) char (char-before point) left point)
	;; check if some extra work should be done
	(cond
	    ((and
		(member char '(#\{ #\}))
		(gethash :newline-before-brace options)
		(< (c-indentation start) (1- (c-offset-align point))))

		(while
		    (and
			(> (decf left) start)
			(member (char-before left) c-tab+spaces)
		    )
		    ;; skip blanks
		)
		(replace-text left left (string #\Newline))
		(c-indent syntax syntable)
	    )

	    ((or
		(and
		    (member char '(#\{ #\}))
		    (gethash :newline-after-brace options)
		)
		(and
		    (char= char #\;)
		    (gethash :newline-after-semi options)
		))

		(incf left)
		(if (= left (scan left :eol :left) (scan left :eol :right))
		    (progn
			(goto-char left)
			(c-indent syntax syntable)
		    )
		    (progn
			(replace-text left left (string #\Newline))
			(goto-char left)
			(c-indent syntax syntable)
		    )
		)
	    )

	    (t
		(when (gethash :trim-blank-lines options)
		    (setq
			offset left
			left   (scan start :eol :left :count 2)
			right  (scan left :eol :right)
		    )
		    (when (and (< left offset) (/= left right))
			(setq
			    line  (read-text left (- right left))
			    index (1- (length line))
			)
			(while
			    (and
				(>= index 0)
				(member (char line index) c-tab+spaces)
			    )
			    (decf index)
			)
			(and (minusp index) (replace-text left right ""))
		    )
		)
	    )
	)
    )
)
(compile 'c-indent)

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
;; $XFree86: xc/programs/xedit/lisp/modules/progmodes/c.lsp,v 1.8 2002/09/29 02:55:01 paulo Exp $
;;

(require "syntax")
(in-package "XEDIT")

(defsynprop *prop-format*
    "format"
    :font	"*lucidatypewriter-medium-r*12*"
    :foreground	"RoyalBlue2"
    :underline	t
)

;; XXX This is not yet used
(defsynoptions *c-mode-options*
    ;; Positive number. This is the basic indentation used when there is
    ;;			no other option to override it. Set to 2 for
    ;;			GNU style indentation.
    (:indent		.	4)

    (:case-indent	.	4)

    ;; Positive number. Set to 2 for GNU style indentation.
    (:brace-indent	.	0)

    ;; Boolean. Move cursor to the indent column after pressing <Enter>?
    (:newline-indent	.	nil)
)

(defsyntax *c-mode* :main nil #+debug #'c-indent #-debug nil *c-mode-options*
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


;; XXX code below is not yet used

;; don't add tab or newline to this list
(defconstant c-space-characters '(#\Space #\Page #\Return #\Vt))

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
	    ((member char c-space-characters)
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
(defun c-exp-match (from match &aux char count offset line open start)
    (setq
	count	1
	offset	(scan from :eol :left)
	line	(read-text offset (- from offset))
	index	(1- (length line))
	;; < and > aren't currently used, but should at some time,
	;; to properly parse c++ templates.
	open	(case match (#\) #\() (#\] #\[) (#\} #\{) (#\> #\<))
    )

    (loop
	;; check if need to read a new line of text
	(while (minusp index)
	    (setq start offset)
	    (multiple-value-setq
		(offset from)
		(c-top-match (scan from :eol :left :count 2))
	    )
	    ;; if cannot find a safe point to restart parsing
	    (and (= start offset) (return))
	    ;; rebuild loop state
	    (setq
		line  (read-text offset (- from offset))
		index (1- (length line))
	    )
	)

	(setq char (char line index))
	(cond
	    ;; found the matching (, [, {, or <
	    ((char= char open)
		(if (zerop (decf count))
		    ;; success
		    (return (+ offset index))
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
		;; index will be smaller than zero if comment is multiline
		(setq index (- from offset))
	    )
	    ;; found a string or character
	    ((member char '(#\" #\'))
		(setq from (c-char-match (+ offset (1- index)) char))
		;; should never be true, cannot find start of string/character
		(unless (integerp from)
		    (return)
		)
		;; index will be smaller than zero if was multiline
		(setq index (- from offset))
	    )
	)

	(decf index)
    )
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
(defun c-top-match (from &aux char to line index left right prev)
    (setq
	to	(scan from :eol :right)
	left	from
	right	to
	prev	right
	line	(read-text left (- right left))
	index	(1- (length line))
    )

    (loop
	(when (minusp index)
	    (if (<= (setq left (scan right :eol :left :count 2)) (point-min))
		;; if start of file found return from loop
		(return)
	    )
	    (setq right (scan left :eol :right))
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
	)

	(case (setq char (char line index))
	    ;; maybe a comment
	    (#\/
		(when (plusp index)
		    (case (char line (decf index))
			;; c++ comment
			(#\/	(setq to (+ left (1- index)) right to))
			;; normal comment
			(#\*
			    (setq
				right (search-backward "/*" (+ left (1- index)))
				index (- right left)
			    )
			    ;; true if the end of the comment is at eol
			    (and (= to prev) (setq to right))
			)
		    )
		)
	    )
	    ;; preprocessor directive; XXX don't flag an error if this
	    ;; is not the first non blank character in the line?
	    (#\#
		(setq to (+ left (1- index)) right to)
	    )
	    ;; string or character
	    ((#\" #\')
		(setq right (c-char-match (+ left (1- index)) char))
		;; true if the end of the string or character is at eol
		(and (= to prev) (setq to right))
	    )
	)
	(if (/= right prev)
	    ;; if something was skiped
	    (progn
		;; if failed to skip
		(if (null right) (return))
		;; index will be smaller than zero if need to restart
		;; in the previous line
		(setq index (- right left) prev right)
	    )
	    ;; else just check the previous character
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
(defun c-continuation (offset &aux indent index char)
    (when (= offset (scan offset :eol :left))
	(multiple-value-setq (indent index char) (c-offset-indent offset))
	(and
	    (characterp char)
	    (char/= char #\Newline)
	    (return-from c-continuation (values indent char))
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

	;; hash table of options
	(options (syntax-options syntax))

	;; start parsing this region
	(left start)
	(right point)

	;; information for text before cursor
	(line (read-text left (- right left)))
	(index (1- (length line)))

	;; indentation that will be used
	(indent 0)

	;; may be used to calculate indentation, possibly relative
	line-indent

	;; current character read
	char

	;; first non-blank char, nil at eof, or newline at empty/blank line
	first-char

	;; first character of current line
	line-char

	;; offset of the first character
	first-offset

	;; flag to readjust backuped line as have skiped a multiline
	;; comment, string or character.
	line-adjust

	;; number of backed lines, will not be the exact number when
	;; skipping multiline comments, preprocessor, etc.
	(line-count 0)
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
	    (current-indent index first-char)
	    (c-offset-indent start)
	)
	;; if should only indent after first char typed
	(and
	    (zerop index)
	    (not (gethash :newline-indent options))
	    (or  (null first-char) (char= first-char #\Newline))
	    (return-from c-indent)
	)
	(setq first-offset (+ start index))

	;; if line starts with one of these characters, align with match
	(when (member first-char '(#\) #\]))
	    (setq right (c-exp-match (1- first-offset) first-char))
	    (if (integerp right)
		(progn
		    (setq indent (c-offset-align right))
		    (go :indent)
		)
		;; unbalanced
		(return-from c-indent)
	    )
	)

	;; get information of previous expression
	(setq index -1)
	(loop
	    ;; while reading an empty line
	    (while (minusp index)
		(setq
		    offset left
		    left   (scan left :eol :left :count 2)
		)
		;; failed to backup a line, start of file reached
		(and (= offset left) (return-from c-indent))

		(multiple-value-setq (left right) (c-top-match left))

		;; failed to find a safe place to restart parsing
		(and (= offset left) (return-from c-indent))

		;; read a chunk of text
		(setq
		    line  (read-text left (- right left))
		    index (1- (length line))
		)

		(if (multiple-value-setq
			(indent line-char) (c-continuation left)
		    )
		    (progn
			(if (= (incf line-count) 1)
			    (setq line-indent indent)
			    ;; continuation is more than 1 line
			    (when (> line-count 2)
				(setq indent line-indent)
				(go :indent)
			    )
			)
		    )
		    ;; line was empty, make the loop test true
		    (setq index -1)
		)
	    )

	    (case (setq char (char line index))
		;; inside an expression, align with it
		((#\( #\[)
		    (setq indent (1+ (c-offset-align (+ left index))))
		    (go :indent)
		)
		;; move to beginning of expression
		((#\) #\])
		    (setq right (c-exp-match (1- (+ left index)) char))
		    (if (integerp right)
			(setq line-adjust t)
			;; unbalanced
			(return-from c-indent)
		    )
		)
		;; move to beginning of string or character
		((#\" #\')
		    (setq right (c-char-match (1- (+ left index)) char))
		    (if (integerp right)
			(setq line-adjust t)
			;; failed to find start
			(return-from c-indent)
		    )
		)
		;; maybe a comment
		(#\/
		    (when
			(and (plusp index) (char= (char line (1- index)) #\*))
			(setq right (search-backward "/*" (+ left (- index 2))))
			(if (integerp right)
			    (setq line-adjust t)
			    ;; failed to find start of comment
			    (return-from c-indent)
			)
		    )
		)
		;; one full statement was skiped
		(#\;
		    (when (> line-count 1)
			;; if a continuation, add one indentation level
			(setq indent (+ line-indent (gethash :indent options)))
			(go :indent)
		    )
		    ;; exit loop
		    (return)
		)
		;; end of a block
		(#\}
		    (if (char/= line-char char)
			(decf indent (gethash :indent options))
			(decf indent (gethash :brace-indent options))
		    )
		    (go :indent)
		)
	    )

	    (if line-adjust
		(progn
		    (if (> right left)
			;; didn't skip more than one line
			(setq index (- right left 1))
			;; skiped more than one line, read new chunk of text
			(progn
			    (when
				(multiple-value-setq
				    (indent line-char)
				    (c-continuation left)
				)
				(if (= (incf line-count) 1)
				    (setq line-indent indent)
				    ;; continuation is more than 1 line
				    (when (> line-count 2)
					(setq indent line-indent)
					(go :indent)
				    )
				)
			    )
			    (setq
				left  (scan right :eol :left)
				text  (read-text left (- right left))
				index (1- (length line))
			    )
			)
		    )
		    (setq line-adjust nil)
		)
		;; no adjustment, check previous character
		(decf index)
	    )
	)

	(return)
;------------------------------------------------------------------------
; indent the current line
:indent
	(and (minusp indent) (setq indent 0))
	(when (/= indent current-indent)
	    (let
		(tabs spaces string)
		(multiple-value-setq (tabs spaces) (floor indent 8))
		(setq
		    index  (+ tabs spaces)
		    offset (+ start index)
		    string (make-string index :initial-element #\Tab)
		)
		(fill string #\Space :start tabs)
		(replace-text start first-offset string)
		(if (> first-offset point)
		    (goto-char offset)
		    (goto-char (- point (- first-offset start index)))
		)
	    )
	)
    )
)

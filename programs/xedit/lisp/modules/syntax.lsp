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
;; $XFree86$
;;

(provide "syntax")

(require "xedit")

(in-package "XEDIT")
(defvar *SYNTAX-SYMBOLS*
    '(
    syntax-highlight defsyntax syntax-p syntable syntoken synaugment

    *PROP-DEFAULT* *PROP-KEYWORD* *PROP-NUMBER* *PROP-STRING*
    *PROP-CONSTANT* *PROP-COMMENT* *PROP-PREPROCESSOR*
    *PROP-PUNCTUATION* *PROP-ERROR*
    )
)

(export *SYNTAX-SYMBOLS*)

(in-package "USER")
(dolist (symbol xedit::*SYNTAX-SYMBOLS*)
    (import symbol)
)

(in-package "XEDIT")



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Some annotations to later write documentation for the module...
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#|
    The current interface logic should be easy to understand for people
that have written lex scanners before. It has some extended semantics,
that could be translated to stacked BEGIN() statements in lex, but
currently does not have rules for matches in the format RE/TRAILING, as
well as code attached to rules (the biggest difference) and/or things
like REJECT and unput(). Also, at least currently, it is *really* quite
slower than lex.

	MATCHING RULES
	--------------
    When two tokens are matched at the same input offset, the longest
token is used, if the length is the same, the first definition is
used. For example:
	token1	=>	int
	token2	=>	[A-Za-z]
	input	=>	integer
    Token1 matches "int" and token2 matches "integer", but since token2 is
longer, it is used. But in the case:
	token1	=>	int
	token2	=>	[A-Za-z]
	input	=>	int
    Both, token1 and token2 match "int", since token1 is defined first, it
is used.

	RESERVED NAMES
	--------------
    :PREVIOUS
    Reserved to be used with a :SWITCH clause to pop a syntax table from
the parser stack, or if the stack is empty, just return/keep in the top level.
|#


;;  Initialize some default properties that may be shared in syntax
;; highlight definitions. Use of these default properties is encouraged,
;; so that "tokens" will be shown identically when editing program
;; sources in different programming languages.
(defsynprop *PROP-DEFAULT*
    "default"
    :FONT	"*courier-medium-r*12*"
    :FOREGROUND	"black"
)

(defsynprop *PROP-KEYWORD*
    "keyword"
    :FONT	"*courier-bold-r*12*"
    :FOREGROUND	"gray12"
)

(defsynprop *PROP-NUMBER*
    "number"
    :FONT	"*courier-bold-r*12*"
    :FOREGROUND	"OrangeRed3"
)

(defsynprop *PROP-STRING*
    "string"
    :FONT	"*lucidatypewriter-medium-r*12*"
    :FOREGROUND	"RoyalBlue2"
)

(defsynprop *PROP-CONSTANT*
    "constant"
    :FONT	"*lucidatypewriter-medium-r*12*"
    :FOREGROUND	"VioletRed3"
)

(defsynprop *PROP-COMMENT*
    "comment"
    :FONT	"*courier-medium-o*12*"
    :FOREGROUND	"SlateBlue3"
)

(defsynprop *PROP-PREPROCESSOR*
    "preprocessor"
    :FONT	"*courier-medium-r*12*"
    :FOREGROUND	"green4"
)

(defsynprop *PROP-PUNCTUATION*
    "punctuation"
    :FONT	"*courier-bold-r*12*"
    :foreground	"gray12"
)

(defsynprop *PROP-ERROR*
    "error"
    :FONT	"*new century schoolbook-bold*24*"
    :FOREGROUND	"yellow"
    :BACKGROUND	"red"
)



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  The "main" definition of the syntax highlight coding interface.
;;  Creates a "special" variable with the given name, associating to
;; it an already compiled syntax table.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defmacro DEFSYNTAX (variable name label property &rest lists)
    `(progn
	(proclaim '(special ,variable))
	(setq ,variable
	    (compile-syntax-table
		,name
		(syntable ,label ,property ,@lists)
	    )
	)
    )
)



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; These definitions should be "private".
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defstruct SYNTOKEN
    regex		;; A compiled regexp.
    property		;; NIL for default, or a synprop structure.
    contained		;; Only used when switch/begin is not NIL. Values:
			;;	NIL	  -> just switch to or begin new
			;;		     syntax table.
			;;	(not NIL) -> apply syntoken property
			;;		     (or default one) to matched
			;;		     text *after* switching to or
			;;		     beginning a new syntax table.
    switch		;; Values for switch are:
			;;	NIL	  -> do nothing
			;;	A keyword -> switch to the syntax table
			;;		     identified by the keyword.
			;;  NOTE: This is actually a jump, the stack is
			;; popped until the named syntax table is found,
			;; if the stack becomes empty, a new state is
			;; implicitly created.
    begin		;;  Same values as for switch, but instead of
			;; popping the stack, it pushes the current syntax
			;; table to the stack and sets a new current one.

    ;; Note: Either "switch" or "begin" may be specified, not both.
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Just a wrapper to make-syntoken.
;; XXX If regex was also a &key argument, it could be more clean
;;     to have a constructor for the structure.
;;     TODO: Add support for structure constructors.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun SYNTOKEN (regex &key icase property contained switch begin)
    (make-syntoken
	:REGEX		(regcomp regex :icase icase)
	:PROPERTY	property
	:CONTAINED	contained
	:SWITCH		switch
	:BEGIN		begin
    )
)


;;  This structure is defined only to do some type checking, it just
;; holds a list of keywords.
(defstruct SYNAUGMENT
    labels		;; List of keywords labeling syntax tables.
)


(defstruct SYNTABLE
    label		;; A keyword naming this syntax table.
    property		;; NIL or a default synprop structure.
    tokens		;; A list of syntoken structures.
    tables		;; A list of syntable structures.
    augments		;;  A list of synaugment structures, used only
			;; at "compile time", so that a table can be
			;; used before it's definition.
)


;;  The main structure, normally, only one should exist per syntax highlight
;; module.
(defstruct SYNTAX
    name		;;  A unique string to identify the syntax mode.
			;; Should be the name of the language/file type.
    table		;; The main syntable structure.
			;; NOTE: This is also the car of the labels field.

    ;; Field(s) defined at "compile time"
    labels		;;  Not exactly a list of labels, but all syntax
			;; tables for the module.
			;; XXX When hash tables be implemented in the
			;;     interpreter, this should be a hash table
			;;     to speed up a bit translating labels to
			;;     syntax tables.
    quark		;;  A XrmQuark associated with the XawTextPropertyList
			;; used by this syntax mode.

    ;; Field(s) used at "run time"
    syntax		;;  The current syntable, about to move to the
			;; top of the stack, if a rule requests nesting.
			;;  At initialization, or when in the top level,
			;; this is the same as the table field.
    property		;;  The current default text property.
			;;  NOTE: This value is also available from the
			;; syntax field as it is it's property element.
    stack		;; Stack of syntax tables for nested rules.
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Just call make-syntable, but sorts the elements by type, allowing
;; a cleaner code when defining the syntax highlight rules.
;; XXX Same comments as for syntoken about the use of a constructor for
;; structures. TODO: when/if clos is implemented in the interpreter.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun SYNTABLE (label default-property &rest definitions)

    ;; Check for possible errors in the arguments.
    (unless (keywordp label)
	(error "SYNTABLE: ~A is not a keyword" label)
    )
    (unless
	(or
	    (null default-property)
	    (synprop-p default-property)
	)
	(error "SYNTABLE: ~A is an invalid text property"
	    default-property
	)
    )

    ;; Don't allow unknown data in the definition list.
    ;; XXX typecase should be added to the interpreter, and since
    ;;     the code is traversing the entire list, it could build
    ;;     now the arguments to make-syntable.
    (dolist (item definitions)
	(unless
	    (or

		;;  Allow NIL in the definition list, so that one
		;; can put conditionals in the syntax definition,
		;; and if the conditional is false, fill the slot
		;; with a NIL value.
		(atom item)
		(syntoken-p item)
		(syntable-p item)
		(synaugment-p item)
	    )
	    (error "SYNTABLE: invalid syntax table argument ~A" item)
	)
    )

    ;; Build the syntax table.
    (make-syntable
	:LABEL		label
	:PROPERTY	default-property
	:TOKENS		(remove-if-not #'syntoken-p definitions)
	:TABLES		(remove-if-not #'syntable-p definitions)
	:AUGMENTS	(remove-if-not #'synaugment-p definitions)
    )
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Just to do a "preliminary" error checking, every element must be a
;; a keyword, and also check for reserved names.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun SYNAUGMENT (&rest keywords)
    (dolist (keyword keywords)
	(if
	    (or
		(eq keyword :PREVIOUS)
		(not (keywordp keyword))
	    )
	    (error "SYNAUGMENT: bad syntax table label ~A" keyword)
	)
    )
    (make-synaugment :LABELS keywords)
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Recursive compile utility function.
;; Returns a cons in the format:
;;	car	=>	List of all syntoken structures
;;			(including child tables).
;;	cdr	=>	List of all child syntable structures.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun LIST-SYNTABLE-ELEMENTS (table &aux result sub-result)
    (setq
	result
	(cons
	    (syntable-tokens table)
	    (syntable-tables table)
	)
    )

    ;; For every child syntax table.
    (dolist (child (syntable-tables table))

	;; Recursively call list-syntable-elements.
	(setq sub-result (list-syntable-elements child))

	(rplaca
	    result
	    (append (car result) (car sub-result))
	)
	(rplacd
	    result
	    (append (cdr result) (cdr sub-result))
	)
    )

    ;; Return the pair of nested tokens and tables.
    result
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Append tokens of the augment list to the tokens of the specified
;; syntax table.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun COMPILE-SYNTAX-AUGMENT-LIST (table table-list
				    &aux labels augment tokens)

    ;; Create a list of all augment tables.
    (dolist (augment (syntable-augments table))
	(setq labels (append labels (synaugment-labels augment)))
    )

    ;;  Remove duplicates and references to "itself",
    ;; without warnings?
    (setq
	labels
	(remove
	    (syntable-label table)
	    (remove-duplicates labels :FROM-END T)
	)
    )

    ;; Check if the specified syntax tables exists!
    (dolist (label labels)
	(unless
	    (setq
		augment
		(car (member label table-list :key #'syntable-label))
	    )
	    (error "COMPILE-SYNTAX-AUGMENT-LIST: Cannot augment ~A in ~A"
		label
		(syntable-label table)
	    )
	)

	;; Increase list of tokens.
	(setq tokens (append tokens (syntable-tokens augment)))
    )

    ;;  Store the tokens in the augment list. They will be added
    ;; to the syntax table in the second pass.
    (setf (syntable-augments table) tokens)

    ;;  Recurse on every child table.
    (dolist (child (syntable-tables table))
	(compile-syntax-augment-list child table-list)
    )
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Just add the augmented tokens to the token list, recursing on
;; every child syntax table.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun LINK-SYNTAX-AUGMENT-TABLE (table)
    (setf
	(syntable-tokens table)
	(append (syntable-tokens table) (syntable-augments table))
    )

    (dolist (child (syntable-tables table))
	(link-syntax-augment-table child)
    )
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; "Compile" the main structure of the syntax highlight code.
;; Variables "switches" and "begins" are used only for error checking.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun COMPILE-SYNTAX-TABLE (name main-table &aux syntax elements
			     switches begins tables properties)
    (unless (stringp name)
	(error "COMPILE-SYNTAX-TABLE: ~A is not a string" name)
    )

    (setq
	elements
	(list-syntable-elements main-table)

	switches
	(remove-if-not
	    #'keywordp
	    (car elements)
	    :KEY #'syntoken-switch
	)

	begins
	(remove-if-not
	    #'keywordp
	    (car elements)
	    :KEY #'syntoken-begin
	)

	;;  The "main-table" isn't in the list, because
	;; list-syntable-elements includes only the child tables;
	;; this is done to avoid the need of removing duplicates here.
	tables
	(cons main-table (cdr elements))
    )

    ;; Check for elements with switch and begin specified.
    (dolist (item switches)
	(if (syntoken-begin item)
	    (error "COMPILE-SYNTAX-TABLE: :SWITCH and :BEGIN specified")
	)
    )

    ;; Check for typos in the keywords, or for not defined syntax tables.
    (dolist (item switches)
	(unless
	    (or
		(eql (syntoken-switch item) :PREVIOUS)
		(member (syntoken-switch item) tables :key #'syntable-label)
	    )
	    (error "COMPILE-SYNTAX-TABLE: SWITCH ~A cannot be matched"
		(syntoken-switch item)
	    )
	)
    )
    (dolist (item begins)
	(unless (member (syntoken-begin item) tables :key #'syntable-label)
	    (error "COMPILE-SYNTAX-TABLE: BEGIN ~A cannot be matched"
		(syntoken-begin item)
	    )
	)
    )

    ;; Create a list of all properties used by the syntax.
    (setq
	properties
	(remove-duplicates

	    ;; Remove explicitly set to "default" properties.
	    (remove NIL

		(append

		    ;; List all properties in the syntoken list.
		    (mapcar
			#'syntoken-property
			(car elements)
		    )

		    ;; List all properties in the syntable list.
		    (mapcar
			#'syntable-property
			(cdr elements)
		    )
		)
	    )
	    :TEST #'string=
	    :KEY  #'synprop-name
	)
    )


    ;;  Provide a default property if none specified.
    (unless
	(member
	    "default"
	    properties
	    :TEST #'string=
	    :KEY #'synprop-name
	)
	(setq properties (append (list *PROP-DEFAULT*) properties))
    )


    ;;  Now that a list of all nested syntax tables is known, compile the
    ;; augment list. Note that even the main-table can be augmented to
    ;; include tokens of one of it's children.

    ;;  Adding the tokens of the augment tables must be done in
    ;; two passes, or it may cause surprises due to "inherited"
    ;; tokens, as the augment table was processed first, and
    ;; increased it's token list.
    (compile-syntax-augment-list main-table tables)

    ;;  Now just append the augmented tokens to the table's token list.
    (link-syntax-augment-table main-table)

    (setq syntax
	(make-syntax
	    :NAME	name
	    :TABLE	main-table
	    :LABELS	tables
	    :QUARK
		(compile-syntax-property-list
		    name
		    properties
		)

	    ;; Setup initial state.
	    :SYNTAX	main-table
	    :PROPERTY	(syntable-property main-table)

	    ;; Stack field initialization defaults to NIL...
	)
    )

    ;; Ready to run!
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Read a line of text from xedit.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun SYNTAX-READ-LINE (offset
			 &aux
			 text
			 result
			 (end (scan offset :EOL :RIGHT)))
#+debug
    (setq result (read-line *STANDARD-INPUT* NIL NIL))
#-debug
    (if (>= offset end)
	""
	(progn
	    (setq
		result
		(read-text offset (- end offset))
		offset
		(+ offset (length result))
	    )
	    (while (< offset end)
		(setq
		    text	(read-text offset (- end offset))
		    result	(string-concat result text)
		    offset	(+ offset (length text))
		)
	    )
	)
    )
    result
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Loop applying the specifed syntax table to the text.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun SYNTAX-HIGHLIGHT (*SYNTAX*
			 &optional
			 (*FROM* (point-min))
			 (*TO* (point-max))
			 &aux

			 (start
#+debug			     0
#-debug			     (- *FROM* (scan *FROM* :EOL :LEFT))
			 )
			 (*OFFSET* *FROM*)
			 *RESULT*
			 #+debug (*LINE-NUMBER* 0))

    ;; Remove any existing properties from the text.
    (clear-entities *FROM* *TO*)

    ;; Make sure the property list is in use.
    (set-text-property-list (syntax-quark *SYNTAX*))

    (loop
#+debug	(format t "~%[~D]> " (incf *LINE-NUMBER*))

	(let*
	    (
	    match
	    left
	    right

	    ;; Newline not included in the input line.
	    (line (syntax-read-line *FROM*))
	    (length (length line))

	    result
	    regex

	    begin
	    switch
	    contained
	    property
	    change
	    )


	    ;; If input has finished, return.
#+debug	    (unless line (return))
#-debug	    (if (>= *FROM* *TO*) (return))


	    ;;	Actually, should only loop more than once if the syntax
	    ;; table changes, as the "cached" matches are lost.
	    (loop

		;; For every available regex token.
		(dolist (token (syntable-tokens (syntax-table *SYNTAX*)))

		    ;; Find all matches for this token.
		    (setq
			left start
			regex (syntoken-regex token)
		    )

		    ;; Loop with while as there is non local exit in the loop.
		    (while
			(and

			    ;; There is input available.
			    (< left length)

			    ;; And a match to the regex was found.
			    (consp
				(setq match
				    (regexec regex line
					:START left
					:NOTBOL (> left 0)
				    )
				)
			    )
			)

			;;  regexec returns a list of conses, but only the
			;; first pair will be used.
			(setq match (car match) left (cdr match))

			;; Check for null length matches.
			(if (eql left (car match))
			    (incf left)
			    ;;	XXX Null length match is in the middle of
			    ;; a string should be triggered as an error...
			)

			;;  Add match to list of matches. Everything is added
			;; instead of choosing the matches with the smaller
			;; offset, because it is cheaper in computer time to
			;; use xedit lisp code to sort and check for successive
			;; matches than forget it and call regexec again and
			;; again to find the same offsets later.
			(setq result (nconc result (list (cons match token))))
		    )
		)

		;; If result is NIL, nothing was matched in the entire input.
		(when (null result)
		    (setq
			*RESULT*
			(list (list* start length (syntax-property *SYNTAX*)))
		    )

		    ;; Process *RESULT* and go read more input.
		    (return)
		)



		;; If more than one match found.
		(when (> (length result) 1)
		    (let
			(
			index
			offsets
			matches
			)

			;; Put matches with smaller offset first.
			(stable-sort result #'< :KEY #'caar)

			;; List start offset of all matches.
			(setq
			    offsets
			    (remove-duplicates (mapcar #'caar result))
			)

			;; Prepare list for splited matches.
			(setq matches (make-list (length offsets)))

			;; For every matching offset.
			(dotimes (index (length offsets))
			    (setf
				(nth index matches)

				;; Only first/best match will be checked.
				(car
				    (stable-sort

					;; Same start offset.
					(remove
					    (nth index offsets)
					    result
					    :TEST-NOT #'=
					    :KEY #'caar
					)

					;; Select longest match.
					#'> :KEY #'cdar
				    )
				)
			    )
			)

			;;  Now the longest matches at given offsets
			;; are known, but it is still required to remove
			;; overlaps in the matches. For example:
			;;
			;;  input   =>	    aint
			;;  token1  =>	    int
			;;  token2  =>	    [a-z]+
			;;
			;;  token1 will match at (1 . 4)
			;;  token2 will match at (0 . 4)
			;;
			;; token1 must be removed.

			;; Loop removing overlaps.
			(setq index 0)
			(loop

			    ;; If finished removing overlaps.
			    (if (null (setq match (nth index matches)))
				(return)
			    )

			    (setq
				right (cdar match)
				index (1+ index)
			    )

			    (setq matches
				(remove right matches
				    :START  index
				    :TEST

					;;  Check also matches for the
					;; null string, normally when
					;; matching the end of a line.
					;;  Example is matching end of
					;; line, and continuation in
					;; the next line when a newline
					;; is preceded by a backslash:
					;; "$"	  matches end of line.
					;; "\\$"  matches continuation
					;;	  in the next line.
					;;
					;; "$" starts where "\\$" ends,
					;; but since it is a zero length
					;; match, it is also an "overlap".
					#'(lambda (left right)
					    (or
						(= (car right) (cdr right))
						(> left (car right))
					    )
					)
				    :KEY #'car
				)
			    )
			)

			;;  After "filtering" the result, at least a
			;; single match must remain.
			(setq result matches)
		    )
		)

		;;  Process the matched input.
		(dolist (match result)
		    (setq   left	    (caar match)
			    right	    (cdar match)

			    ;; Change match value to the syntoken.
			    match	    (cdr match)
			    begin	    (syntoken-begin match)
			    switch	    (syntoken-switch match)
			    contained	    (syntoken-contained match)
			    change	    (or begin switch)
			    property	    (syntax-property *SYNTAX*)
		    )

		    ;; Check for unmatched text.
		    (if (> left start)
			(setq
			    *RESULT*
			    (nconc
				*RESULT*
				(list (list* start left property))
			    )
			)
		    )

		    ;;	If the syntax table is not changed,
		    ;; or if the new table requires that the
		    ;; current default property be used.
		    (when
			(or
			    (not change)
			    (not contained)
			)
			(if (syntoken-property match)
			    (setq property (syntoken-property match))
			)

			;; Add matched text.
			(setq
			    *RESULT*
			    (nconc
				*RESULT*
				(list (list* left right property))
			    )
			)
		    )



		    ;; Update start offset in the input now!
		    (setq start right)



		    ;;	If the syntax table was changed invalidate
		    ;; the "cache" of matched text.
		    ;;	Even if it is switching to the same syntax
		    ;; table this is required because we don't know
		    ;; if this will or not be tagged as an error.
		    ;; Typical example is nested C comments, where
		    ;; a rule can be created to show it as an error.
		    (when change
			(if switch
			    (progn

				;; Returning to previous state?
				(if (eq :PREVIOUS switch)
				    (setq
					change
					(pop (syntax-stack *SYNTAX*))
				    )

				    ;;	Not returning to the previous
				    ;; state, but returning to a named
				    ;; syntax table, search for it in
				    ;; the stack.
				    (loop
					(setq
					    change
					    (pop (syntax-stack *SYNTAX*))
					)
					(if
					    (or

						;; Stack empty.
						(null change)

						;; Match found.
						(eq switch
						    (syntable-label switch)
						)
					    )
					    (return)
					)
				    )
				)

				;;  If no match found while popping
				;; the stack.
				(if (null change)

				    ;;	Return to the topmost
				    ;; syntax table.
				    (setq
					change
					(syntax-table *SYNTAX*)
				    )
				)
			    )

			    ;; Else it is a begin.
			    (progn
				(setq change
				    (car
					(member
					    begin
					    (syntax-labels *SYNTAX*)
					    :KEY #'syntable-label
					)
				    )
				)

				;;  Save state for a possible
				;; (:SWITCH :PREVIOUS) later.
				(push
				    (syntax-table *SYNTAX*)
				    (syntax-stack *SYNTAX*)
				)
			    )
			)

			(setf
			    (syntax-table *SYNTAX*)
			    change

			    ;;	If "change" is not a syntable structure,
			    ;; it is (not?) too late to give a formatted
			    ;; error output. Anyway, if this happens there
			    ;; is either a bug in the compile code, or in
			    ;;the interpreter itself...
			    (syntax-property *SYNTAX*)
			    (syntable-property change)
			)


			;; If processing of text was deferred.
			(when contained

			    (if (syntoken-property match)
				(setq
				    property
				    (syntoken-property match)
				)
				(setq
				    property
				    (syntax-property *SYNTAX*)
				)
			    )

			    ;; Add matched text with the updated property.
			    (setq
				*RESULT*
				(nconc
				    *RESULT*
				    (list (list* left right property))
				)
			    )
			)


			(return)
		    )	    ;; End of "(when change"
		)	    ;; End of "(dolist (match result)"

		;;  If the entire input was checked, return to the read loop.
		(if (>= start length)
		    (return)

		    ;;  If there was no change in the syntax table,
		    ;; no text was matched up to the end of line.
		    (unless change
			(setq
			    *RESULT*
			    (nconc
				*RESULT*
				(list (list* start length property))
			    )
			)
			(return)
		    )
		)

		;; Prepare to loop again.
		(setq result NIL)
	    )

	    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	    ;; Process *RESULT*
	    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	    ;;	*RESULT* is now a list (not NIL terminated) in which
	    ;; every item has the format:
	    ;;	    item1   =>	    left offset
	    ;;	    item2   =>	    right offset
	    ;;	    item3   =>	    text property

	    ;;	String "default" has a special meaning. If this is the
	    ;; label of the property, set the property to NIL. Could
	    ;; also check for *PROP-DEFAULT*, but it is allowable to
	    ;; create the "default" property binded to other symbol,
	    ;; the real unique identifier is the string.
#+debug	    (dolist (item *RESULT*)
		(if
		    (and
			(synprop-p (cddr item))
			(string= "default" (synprop-name (cddr item)))
		    )
		    (rplacd (cdr item) NIL)
		)
	    )


	    ;;	Join all sequential matches that use the same property.
	    ;; Gaps do not exist. When text was not matched, the loop
	    ;; above filled the gap with the current default property.
	    ;; Overlaps don't exist either.
	    (setq
		left	    (caar *RESULT*)
		property    (cddar *RESULT*)
	    )
	    (dolist (item (cdr *RESULT*))
		(if (eq (cddr item) property)

		    ;;	Two consecutive matches with the same property.
		    ;; Adjust the beginning offset.
		    (rplaca item left)

		    ;; Else, update variables.
		    (setq
			left	    (car item)
			property    (cddr item)
		    )
		)
	    )

	    ;;	Loop in the result. If there are consecutive matches
	    ;; with the same property, now *RESULT* will have the same
	    ;; offset value in the CAR of it's elements.
	    (dolist (item (remove-duplicates *RESULT* :TEST #'= :KEY #'car))
		(setq
		    left	    (car item)
		    right	    (cadr item)
		    property	    (cddr item)
		)

		;; Use the information.
#+debug		(format T "~A: ~S~%"
		    (if (synprop-p property)
			(synprop-name property)
			"default"
		    )
		    (subseq line left right)
		)

		(if
		    (and
			;; XXX
			;; XXX
			;; IF THE CHECK OF RIGHT LARGER THAN LEFT IS NOT
			;; DONE (i.e. right != left) XAW WILL CRASH
			;; ALLOCATING ALL AVAILABLE MEMORY.
			;; XXX FIXME
			;; XXX
			;; XXX
			(> right left)
			(synprop-p property)
		    )
		    (add-entity
			(+ *FROM* left)
			(- right left)
			(synprop-quark property)
		    )
		)
	    )

	    ;; Prepare to new matches.
	    (setq
		*RESULT*	NIL
		start		0
	    )

	    ;; Update offset to read text. Add 1 for the skipped newline.
	    (incf *FROM* (1+ length))
	)
    )

#+debug (terpri)
)

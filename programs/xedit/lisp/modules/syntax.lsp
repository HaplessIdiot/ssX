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
;; $XFree86: xc/programs/xedit/lisp/modules/syntax.lsp,v 1.1 2002/07/22 07:26:29 paulo Exp $
;;

(provide "syntax")

(require "xedit")

(in-package "XEDIT")
(defvar *SYNTAX-SYMBOLS*
    '(
    syntax-highlight defsyntax defsynprop synprop-p syntax-p
    syntable syntoken synaugment

    *PROP-DEFAULT* *PROP-KEYWORD* *PROP-NUMBER* *PROP-STRING*
    *PROP-CONSTANT* *PROP-COMMENT* *PROP-PREPROCESSOR*
    *PROP-PUNCTUATION* *PROP-ERROR* *PROP-ANNOTATION*
    )
)

(export *SYNTAX-SYMBOLS*)

(in-package "USER")
(dolist (symbol xedit::*SYNTAX-SYMBOLS*)
    (import symbol)
)

(in-package "XEDIT")
(makunbound '*SYNTAX-SYMBOLS*)


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
	token2	=>	[A-Za-z]+
	input	=>	integer
    Token1 matches "int" and token2 matches "integer", but since token2 is
longer, it is used. But in the case:
	token1	=>	int
	token2	=>	[A-Za-z]+
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
    :FOREGROUND	"gray12"
)

(defsynprop *PROP-ERROR*
    "error"
    :FONT	"*new century schoolbook-bold*24*"
    :FOREGROUND	"yellow"
    :BACKGROUND	"red"
)

(defsynprop *PROP-ANNOTATION*
    "annotation"
    :FONT	"*courier-medium-r*12*"
    :FOREGROUND	"black"
    :BACKGROUND	"PaleGreen"
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
;; XXX If pattern were also a &key argument, it could be more clean
;;     to have a constructor for the structure.
;;     TODO: Add support for structure constructors.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun SYNTOKEN (pattern &key icase property contained switch begin
		 &aux (regex (regcomp pattern :ICASE icase)) check)

    ;;  Don't allow a regex that matches the null string enter the
    ;; syntax table list.
    (if (consp (setq check (regexec regex "" :NOTEOL :NOTBOL)))
#+xedit	(error "SYNTOKEN: regex matches empty string ~S" regex)
#-xedit	()
    )

    (make-syntoken
	:REGEX		regex
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
;;  Loop applying the specifed syntax table to the text.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun SYNTAX-HIGHLIGHT (*SYNTAX*
			 &optional
			 (*FROM* (point-min))
			 (*TO* (point-max))
			 &aux
#+debug			 (*LINE-NUMBER* 0)
			 stream
			 start
			)

#|
    ;;  Make sure arguments have a sane value,
#-debug
    (if (integerp *FROM*)
	(setq *FROM* (max *FROM* (point-min)))
    )
#-debug
    (if (integerp *TO*)
	(setq *TO* (min *TO* (point-max)))
    )
|#
#+debug
    (setq *FROM* 0 *TO* 0)

    ;;  Remove any existing properties from the text.
    (clear-entities *FROM* *TO*)

    ;;  Make sure the property list is in use.
    (set-text-property-list (syntax-quark *SYNTAX*))


    (setq
	start
#+debug	0
#-debug	(- *FROM* (scan *FROM* :EOL :LEFT))
    )


#-debug
    (let
	(
	text
	list
	(offset *FROM*)
	)

	;;  XXX May enter an inifinite loop if *TO* is invalid.
	(while (< offset *TO*)
	    (setq
		text	(read-text offset (- *TO* offset))
		list	(nconc list (list text))
		offset	(+ offset (length text))
	    )
	)

	;;  XXX When syntax-highlithing the entire file, all the
	;;	file will be in the string, and also, will be
	;;	duplicated, one splited copy in "list", one from
	;;	string-concat, and one stored in the resulting
	;;	string stream.
	(setq
	    stream
	    (make-string-input-stream (apply #'string-concat list))
	)
    )
#+debug
    (setq stream *STANDARD-INPUT*)

    (prog*
	(
	;;  Input line does not end in a newline?
	noteol

	;;  Offset larger than 0 in the line?
	notbol

	;;  Matches for the current list of tokens.
	matches

	;;  Line of text.
	line

	;;  Length of the text line.
	length

	;;  A inverse cache, don't call regexec when the regex is
	;; already known to not match.
	nomatch

	;;  Use cache as a list of matches to avoid repetitive
	;; unnecessary calls to regexec.
	;;  cache is a list in which every element has the format:
	;;	(token . (start . end))
	;;  Line of text.
	cache

	match

	right
	result
	left
	)

;-----------------------------------------------------------------------
:READ
#+debug-verbose
	(format T "** Entering :READ stack length is ~D~%"
	    (length (syntax-stack *SYNTAX*))
	)
#+debug	(format T "~%[~D]> " (incf *LINE-NUMBER*))

	(multiple-value-bind
	    (text eos)
	    (read-line stream NIL NIL)

	    (setq
		line	text
		length	(length text)
		noteol	eos
		nomatch	()
	    )
	)


	;;  If input has finished, return.
	(unless line
	    ;;  XXX FIXME: This will not free the string until the garbage
	    ;;	    collector be called on stream.
#-debug	    (close stream)
	    (return)
	)

:LOOP
#+debug-verbose
	(format T "** Entering :LOOP at offset ~D in table ~A, cache has ~D items~%"
	    start
	    (syntable-label (syntax-table *SYNTAX*))
	    (length cache)
	)

	(setq notbol (> start 0))

	;;  For every regex token in the current syntax table.
	(dolist (token (syntable-tokens (syntax-table *SYNTAX*)))

	    ;;  If token is already known to not be in the input line.
	    (unless (position token nomatch)

		;;  Try to fetch match from cache.
		(if (setq match (member token cache :key #'car))
		    ;;	Match is in the cache.

		    (when
			(member
			    (caar match)
			    (syntable-tokens (syntax-table *SYNTAX*))
			)

			;;  Match must be moved to the beginning of the
			;; matches list, as a match from another syntax
			;; table may be also in the cache, but before
			;; the match for the current token.
			(setq
			    matches
			    (nconc
				matches
				(list (car match))
			    )
			)

			;;  Remove the match from the cache.
			(case (length match)
			    (1
				(if (eq match cache)
				    (setq cache NIL)
				    (rplacd (last cache 2) NIL)
				)
			    )
			    (T
				(setf
				    (car match) (cadr match)
				    (cdr match) (cddr match)
				)
			    )
			)
		    )


		    ;;	Not in the cache, call regexec.
		    (if
			(consp
			    (setq
				match
				(regexec (syntoken-regex token) line
				    :START	start
				    :NOTBOL	notbol
				    :NOTEOL	noteol
				)
			    )
			)

			;;  Match found.
			(progn
#+debug-verbose		    (format T "Adding to cache: {~A:~S} ~A~%"
				(car match)
				(subseq line (caar match) (cdar match))
				(syntoken-regex token)
			    )
			    (setq
				;; Only the first pair is used.
				match
				(car match)

				matches
				(nconc matches (list (cons token match)))
			    )

			    ;;	Exit loop if the all the remaining
			    ;; input was matched.
			    (when
				(and
				    (= start (car match))
				    (= length (cdr match))
				)
#+debug-verbose			(format T "Rest of line match~%")
				(return)
			    )
			)

			;;  Match not found.
#+debug-verbose		(progn
			    (format T "Adding to nomatch: ~A~%"
				(syntoken-regex token)
			    )
			    (setq nomatch (nconc nomatch (list token)))
			)
#-debug-verbose		(setq nomatch (nconc nomatch (list token)))
		    )
		)
	    )
	)

	;;  Add matches to the beginning of the cache list.
	;;  There may be matches from another syntable table in the cache.
	(setq
	    cache	(nconc matches cache)

	    ;;  Make sure that when the match loop is reentered, this
	    ;; variable is NIL.
	    matches	()
	)

	;;  Put matches with smaller offset first.
	(stable-sort cache #'< :KEY #'cadr)

	;;  While the first entry in the is not from the current table.
	(until
	    (or
		(null cache)
		(member
		    (caar cache)
		    (syntable-tokens (syntax-table *SYNTAX*))
		)
	    )
#+debug-verbose
	    (format T "Not in the current table, removing {~A:~S} ~A~%"
		(cdar cache)
		(subseq line (cadar cache) (cddar cache))
		(syntoken-regex (caar cache))
	    )
	    (setq cache (cdr cache))
	)


	;;  If nothing was matched in the entire/remaining line.
	(when (null cache)
	    (setq
		result
		(nconc
		    result
		    (list
			(list* start length (syntax-property *SYNTAX*))
		    )
		)

		;;  Let the code know the input line is finished.
		start
		length
	    )
#+debug-verbose
	    (format T "No match until end of line~%")

	    ;;  Result already known, and there is no syntax table
	    ;; change, bypass :PARSE.
	    (go :PROCESS)
	)

	;;  If there is only one match in the entire input line.
	;;  Can't bypass :PARSE because the single match may repeat
	;; at a longer offset.
	(when (= (length cache) 1)
	    (setq
		match	(car cache)
		left	(cadr match)
		right	(cddr match)

		;;  Reset cache now.
		cache	NIL
	    )

	    (go :PARSE)
	)


        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;;  If got here, length of cache is larger than one.
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	;;  Prepare to choose best match.
	(setq
	    match	(car cache)
	    left	(cadr match)
	    right	(cddr match)

	    ;;  First element can be safely removed now.
	    cache	(cdr cache)
	)

	;;  Remove elements of cache that must be discarded.
	(loop
	    (let*
		(
		(item	(car cache))
		(from	(cadr item))
		(to	(cddr item))
		)

		(if
		    (or

			;;  If everything removed from the cache.
			(null item)

			;;  Or next item is at a longer offset than the
			;; end of current match.
			(> from right)
		    )
		    (return)
		)

		(if (= left from)

		    ;;	If another match at the same offset.
		    (if (> to right)

			;;  If this match is longer than the current one.
			(setq
			    match   item
			    right   to
			)
		    )

		    ;;	Else, if at the offset of the end, but
		    ;; not a zero length match, keep it by returning.
		    ;;	A zero length match *is* an overlap, and
		    ;; must be removed.
		    (when
			(and
			    (= from right)
			    (> to from)
			)
			(return)
		    )
		)

#+debug-verbose	(format T "Removing from cache {~A:~S} ~A~%"
		    (cdar cache)
		    (subseq line from to)
		    (cdr cache)
		)
		(setq cache (cdr cache))
	    )
	)


;-----------------------------------------------------------------------
:PARSE
#+debug-verbose
	(format T "** Entering :PARSE~%")

	(setq

	    ;;  Change match value to the syntoken.
	    match	(car match)
	)

	(let*
	    (
	    (begin	(syntoken-begin match))
	    (switch	(syntoken-switch match))
	    (contained	(syntoken-contained match))
	    (change	(or begin switch))
	    (property	(syntax-property *SYNTAX*))
	    )

	    ;;  Check for unmatched leading text.
	    (when (> left start)
#+debug-verbose	(format T "No match in {(~D . ~D):~S}~%"
		    start
		    left
		    (subseq line start left)
		)
		(setq
		    result
		    (nconc
			result
			(list (list* start left property))
		    )
		)
	    )

	    ;;  If the syntax table is not changed,
	    ;; or if the new table requires that the
	    ;; current default property be used.
	    (when
		(or
		    (not change)
		    (not contained)
		)

		;;  If token specifies the property.
		(if (syntoken-property match)
		    (setq property (syntoken-property match))
		)

		;;  Add matched text.
		(setq
		    result
		    (nconc
			result
			(list (list* left right property))
		    )
		)
#+debug-verbose	(format T "(0)Match found for {(~D . ~D):~S}~%"
		    left
		    right
		    (subseq line left right)
		)
	    )


	    ;;	Update start offset in the input now!
	    (setq start right)

	    ;;	When changing the current syntax table.
	    (when change
		(if switch
		    (progn
#+debug-verbose		(format T "switching to ")
			(if (eq :PREVIOUS switch)

			    ;;	If returning to the previous state.
			    (setq change (pop (syntax-stack *SYNTAX*)))

			    ;;	Else, not to the previous state, but
			    ;; returning to a named syntax table,
			    ;; search for it in the stack.
			    (while
				(and
				    (setq
					change
					(pop (syntax-stack *SYNTAX*))
				    )
				    (not (eq switch change))
				)
				;;  Empty loop.
			    )
			)

			;;  If no match found while popping
			;; the stack.
			(if (null change)

			    ;;	Return to the topmost syntax table.
			    (setq
				change
				(syntax-table *SYNTAX*)
			    )
			)
		    )

		    ;;	Else, it is a begin.
		    (progn
#+debug-verbose		(format T "begining ")
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
			;; :SWITCH later.
			(push
			    (syntax-table *SYNTAX*)
			    (syntax-stack *SYNTAX*)
			)
		    )
		)

#+debug-verbose	(format T "~A offset: ~D~%"
		    (syntable-label change)
		    start
		)

		;;  Change current syntax table.
		(setf
		    (syntax-table *SYNTAX*)
		    change

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
			result
			(nconc
			    result
			    (list (list* left right property))
			)
		    )

#+debug-verbose	    (format T "(1)Match found for {(~D . ~D):~S}~%"
			left
			right
			(subseq line left right)
		    )
		)

		;;  This is to find matches to end of line.
		;;  Probably it would be better to know if such matches
		;; exists in the current syntax table.
		(if (= start length) (go :LOOP))
	    )
	)


;-----------------------------------------------------------------------
:PROCESS
#+debug-verbose
	(format T "** Entering :PROCESS~%")

	;;  Wait for the end of the line to process, so that 
	;; it is possible to join sequential matches with the
	;; same text property.
	(if (< start length)
	    (go :LOOP)
	)


	(let
	    (
	    property
	    )

	    ;;	Join all sequential matches that use the same property.
	    ;; Gaps do not exist. When text was not matched, the loop
	    ;; above filled the gap with the current default property.
	    ;; Overlaps don't exist either.
	    (setq
		left	    (caar result)
		property    (cddar result)
	    )
	    (dolist (item (cdr result))
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
	    ;; with the same property, now result will have the same
	    ;; offset value in the CAR of it's elements.
	    (dolist (item (remove-duplicates result :TEST #'= :KEY #'car))
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
	)

	;; Prepare for new matches.
	(setq
	    result    NIL
	    start    0
	)


	;;  Update offset to read text.
	;;  Add 1 for the skipped newline.
	(incf *FROM* (1+ length))

	(go :READ)
    )

#+debug (terpri)
)

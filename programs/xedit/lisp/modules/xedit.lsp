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
;; $XFree86: xc/programs/xedit/lisp/modules/xedit.lsp,v 1.1 2002/07/22 07:26:30 paulo Exp $
;;

(provide "xedit")

(make-package "XEDIT" :use '("LISP" "EXT"))
(in-package "XEDIT")


(export '(
    background foreground font point point-min point-max
    insert read-text replace-text scan search-forward search-backward
    wrap-mode auto-fill justification left-column right-column
    vertical-scrollbar horizontal-scrollbar
    create-buffer remove-buffer
    buffer-name buffer-filename
    current-buffer other-buffer
))


;; Character that identifies xedit protocol commands.
(defconstant *ESCAPE* #-debug #\Escape #+debug #\$)

;; Stream to write to xedit.
(defvar *OUTPUT* *STANDARD-OUTPUT*)

;; Stream to read from xedit.
(defvar *INPUT* *STANDARD-INPUT*)

;; Recognized identifiers for wrap mode.
(defconstant *WRAP-MODES* '(:NEVER :LINE :WORD))

;; Recognized identifiers for justification.
(defconstant *JUSTIFICATIONS* '(:LEFT :RIGHT :CENTER :FULL))

;; XawTextScanType
(defconstant
    *SCAN-TYPE*
    '(:POSITIONS :WHITE-SPACE :EOL :PARAGRAPH :ALL :ALPHA-NUMERIC)
)

;; XawTextScanDirection
(defconstant *SCAN-DIRECTION* '(:LEFT :RIGHT))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Data types.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Xlfd description, used when combining properties.
;;  Field names are self descriptive.
;;	XXX Fields should be initialized as strings, but fields
;;	    that have an integer value should be allowed to
;;	    be initialized as such.
;;  Combining properties in supported in Xaw, but not yet in the
;; syntax highlight code interface. Combining properties allow easier
;; implementation for markup languages, for example:
;;	<b>bold<i>italic</i></b>
;;	would render "bold" using a bold version of the default font,
;;	and "italic" using a bold and italic version of the default font
(defstruct XLFD
    foundry
    family
    weight
    slant
    setwidth
    addstyle
    pixel-size
    point-size
    res-x
    res-y
    spacing
    avgwidth
    registry
    encoding
)


;;   At some time this structure should also hold information for at least:
;;	o fontset
;;	o foreground pixmap
;;	o background pixmap
;;   XXX This is also a TODO in Xaw.
(defstruct SYNPROP
    quark	;;   XrmQuark identifier of the XawTextProperty
		;; structure. This field is filled when "compiling"
		;; the syntax-table.

    name	;;   String name of property, must be unique per
		;; property list.
    font	;; Optional font string name of property.
    foreground	;; Optional string representation of foreground color.
    background	;; Optional string representation of background color.
    xlfd	;;   Optional xlfd structure, when combining properties.
		;; Currently combining properties logic not implemented,
		;; but fonts may be specified using the xlfd definition.

    ;; Boolean properties.
    underline	;; Draw a line below the text.
    overscript	;; Draw a line over the text.

    ;; XXX Are these working in Xaw?
    subscript	;; Align text to the bottom of the line.
    superscript	;; Align text to the top of the line.
    ;;  Note: subscript and superscript only have effect when the text
    ;; line has different height fonts displayed.
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Utility macro, to create a "special" variable holding
;; a synprop structure.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defmacro DEFSYNPROP (variable name
		      &key font foreground background xlfd underline
			   overscript subscript superscript)
    `(progn
	(proclaim '(special ,variable))
	(setq ,variable
	    (make-synprop
		:NAME		,name
		:FONT		,font
		:FOREGROUND	,foreground
		:BACKGROUND	,background
		:XLFD		,xlfd
		:UNDERLINE	,underline
		:OVERSCRIPT	,overscript
		:SUBSCRIPT	,subscript
		:SUPERSCRIPT	,superscript
	    )
	)
    )
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Convert a synprop structure  to a string in the format
;; expected by Xaw.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun SYNPROP-TO-STRING (synprop &aux values booleans xlfd)
    (if (setq xlfd (synprop-xlfd synprop))
	(dolist
	    (element
	       `(
		("foundry"	    ,(xlfd-foundry xlfd))
		("family"	    ,(xlfd-family xlfd))
		("weight"	    ,(xlfd-weight xlfd))
		("slant"	    ,(xlfd-slant xlfd))
		("setwidth"	    ,(xlfd-setwidth xlfd))
		("addstyle"	    ,(xlfd-addstyle xlfd))
		("pixelsize"	    ,(xlfd-pixel-size xlfd))
		("pointsize"	    ,(xlfd-point-size xlfd))
		("resx" 	    ,(xlfd-res-x xlfd))
		("resy" 	    ,(xlfd-res-y xlfd))
		("spacing"	    ,(xlfd-spacing xlfd))
		("avgwidth"	    ,(xlfd-avgwidth xlfd))
		("registry"	    ,(xlfd-registry xlfd))
		("encoding"	    ,(xlfd-encoding xlfd))
		)
	    )
	    (if (cadr element)
		(setq values (append values element))
	    )
	)
    )
    (dolist
	(element
	   `(
	    ("font"		,(synprop-font synprop))
	    ("foreground"	,(synprop-foreground synprop))
	    ("background"	,(synprop-background synprop))
	    )
	)
	(if (cadr element)
	    (setq values (append values element))
	)
    )

    ;;  Boolean attributes. These can be specified in the format
    ;; <name>=<anything>, but do a nicer output as the format
    ;; <name> is accepted.
    (dolist
	(element
	    `(
	    ("underline"	,(synprop-underline synprop))
	    ("overscript"	,(synprop-overscript synprop))
	    ("subscript"	,(synprop-subscript synprop))
	    ("superscript"	,(synprop-superscript synprop))
	    )
	)
	(if (cadr element)
	    (setq booleans (append booleans element))
	)
    )

    ;;  Play with format conditionals, list iteration, and goto, to
    ;; make resulting string.
    (format
	NIL
	"~A~:[~;?~]~:[~3*~;~A=~A~{&~A=~A~}~]~:[~;&~]~:[~2*~;~A~{&~A~*~}~]"

	(synprop-name synprop)				;; ~A
	(or values booleans)				;; ~:[~;?~]
	values						;; ~:[
	    (car values) (cadr values) (cddr values)	;; ~A=~A~{&~A=~A~}
	(and values booleans)				;; ~:[~;&~]
	booleans					;; ~:[
	    (car booleans) (cddr booleans)		;; ~A~{&~A~*~}
    )
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Remove XawTextEntity structures in a given text region.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun CLEAR-ENTITIES (left right)
    (format *OUTPUT* "~Cclear-entities ~D ~D~%"
	*ESCAPE*
	left
	right
    )
#-debug
    (read *INPUT*)
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Add the XawTextEntity associated with the identifier
;; to the specified text region.
;; XXX Needs to add support for type, flags and data associated
;;     with the entity.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun ADD-ENTITY (offset length identifier)
    (format *OUTPUT* "~Cadd-entity ~D ~D ~D~%"
	*ESCAPE*
	offset
	length
	identifier
    )
#-debug
    (read *INPUT*)
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Use xedit protocol to create a XawTextPropertyList with the
;; given arguments.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun COMPILE-SYNTAX-PROPERTY-LIST (name properties
				     &aux string-properties quark)

    ;; Create a string representation of the properties.
    (dolist (property properties)
	(setq
	    string-properties
	    (append
		string-properties
		(list (synprop-to-string property))
	    )
	)
    )

    (setq
	string-properties
	(case (length string-properties)
	    (0	"")
	    (1	(car string-properties))
	    (T	(format NIL "~A~{,~A~}"
		    (car string-properties)
		    (cdr string-properties)
		)
	    )
	)
    )

    (format *OUTPUT* "~Cconvert-property-list ~S ~S~%"
	*ESCAPE*
	name
	string-properties
    )

    ;; If nothing wen't wrong, the XrmQuark identifier can be read.
    (setq quark #-debug (read *INPUT*) #+debug 0)

    ;; Store the quark for properties not yet "initialized".
    ;; XXX This is just a call to Xrm{Perm,}StringToQuark, and should
    ;;     be made available if there were a wrapper/interface to
    ;;     that Xlib function.
    (dolist (property properties)
	(unless (integerp (synprop-quark property))
	    (format *OUTPUT* "~Cxrm-string-to-quark ~S~%"
		*ESCAPE*
		(synprop-name property)
	    )
	    (setf
		(synprop-quark property)
#-debug		(read *INPUT*)
#+debug		0
	    )
	)
    )

    quark
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Use xedit protocol to set a previously create
;; XawTextPropertyList to the Sink object of the current text window.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun SET-TEXT-PROPERTY-LIST (quark)
    (format *OUTPUT* "~Cset-text-properties ~D~%" *ESCAPE* quark)
#-debug
    (read *INPUT*)
)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Generic functions.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(defun BACKGROUND (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-background ~S~%" *ESCAPE* value)
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-background~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun FOREGROUND (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-foreground ~S~%" *ESCAPE* value)
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-foreground~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun FONT (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-font ~S~%" *ESCAPE* value)
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-font~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun POINT (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-point ~D~%" *ESCAPE* value)
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-point~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun POINT-MIN ()
    (format *OUTPUT* "~Cpoint-min~%" *ESCAPE*)
#-debug
    (read *INPUT*)
#+debug
    0
)


(defun POINT-MAX ()
    (format *OUTPUT* "~Cpoint-max~%" *ESCAPE*)
#-debug
    (read *INPUT*)
)


(defun INSERT (string)
    (format *OUTPUT* "~Cinsert ~S~%" *ESCAPE* string)
#-debug
    (read *INPUT*)
    string
)


(defun READ-TEXT (offset length)
    (format *OUTPUT* "~Cread-text ~D ~D~%"
	*ESCAPE*
	offset
	length
    )
#-debug
    (read *INPUT*)
#+debug
    (read-line)
)


(defun REPLACE-TEXT (left right string)
    (format *OUTPUT* "~Creplace-text ~D ~D ~S~%"
	*ESCAPE*
	left
	right
	string
    )
#-debug
    (read *INPUT*)
    string
)


(defun SCAN (offset type direction &key (count 1) include)
    (unless (setq type (position type *SCAN-TYPE*))
	(error
	    "SCAN: type must be one of ~A, not ~A"
	    *SCAN-TYPE*
	    type
	)
    )
    (unless (setq direction (position direction *SCAN-DIRECTION*))
	(error
	    "SCAN: direction must be one of ~A, not ~A"
	    *SCAN-DIRECTION*
	    direction
	)
    )
    (format *OUTPUT* "~Cscan ~D ~D ~D ~D ~D~%"
	*ESCAPE*
	offset
	type
	direction
	count
	(if include 1 0)
    )
#-debug
    (read *INPUT*)
#+debug
    offset
)


(defun SEARCH-FORWARD (string &optional case-sensitive)
    (format *OUTPUT* "~Csearch-forward ~S ~D~%"
	*ESCAPE* string (if case-sensitive 1 0)
    )
#-debug
    (read *INPUT*)
)


(defun SEARCH-BACKWARD (string &optional case-sensitive)
    (format *OUTPUT* "~Csearch-backward ~S ~D~%"
	*ESCAPE* string (if case-sensitive 1 0)
    )
#-debug
    (read *INPUT*)
)


(defun WRAP-MODE (&optional (value NIL specified))
    (if specified
	(progn
	    (unless (member value *WRAP-MODES*)
		(error
		    "WRAP-MODE: argument must be one of ~A, not ~A"
		    *WRAP-MODES*
		    value
		)
	    )
	    (format *OUTPUT* "~Cset-wrap-mode ~S~%"
		*ESCAPE*
		(string value)
	    )
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-wrap-mode~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun AUTO-FILL (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-auto-fill ~S~%"
		*ESCAPE*
		(if value "true" "false")
	    )
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-auto-fill~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun JUSTIFICATION (&optional (value NIL specified))
    (if specified
	(progn
	    (unless (member value *JUSTIFICATIONS*)
		(error
		    "JUSTIFICATION: argument must be one of ~A, not ~A"
		    *JUSTIFICATIONS*
		    value
		)
	    )
	    (format *OUTPUT* "~Cset-justification ~S~%"
		*ESCAPE*
		(string value)
	    )
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-justification~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun LEFT-COLUMN (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-left-column ~D~%" *ESCAPE* value)
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-left-column~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun RIGHT-COLUMN (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-right-column ~D~%" *ESCAPE* value)
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-right-column~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun VERTICAL-SCROLLBAR (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-vert-scrollbar ~S~%"
		*ESCAPE*
		(if value "always" "never")
	    )
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-vert-scrollbar~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun HORIZONTAL-SCROLLBAR (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-horiz-scrollbar ~S~%"
		*ESCAPE*
		(if value "always" "never")
	    )
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-horiz-scrollbar~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun CREATE-BUFFER (name)
    (format *OUTPUT* "~Ccreate-buffer ~S~%" *ESCAPE* name)
    (read *INPUT*)
)


(defun REMOVE-BUFFER (name)
    (format *OUTPUT* "~Cremove-buffer ~S~%" *ESCAPE* name)
#-debug
    (read *INPUT*)
)


(defun BUFFER-NAME (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-buffer-name ~S~%" *ESCAPE* value)
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-buffer-name~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun BUFFER-FILENAME (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-buffer-filename ~S~%"
		*ESCAPE*
		(namestring value)
	    )
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-buffer-filename~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    (pathname value)
)


(defun CURRENT-BUFFER (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-current-buffer ~S~%" *ESCAPE* value)
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-current-buffer~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)


(defun OTHER-BUFFER (&optional (value NIL specified))
    (if specified
	(progn
	    (format *OUTPUT* "~Cset-other-buffer ~S~%" *ESCAPE* value)
#-debug	    (read *INPUT*)
	)
	(progn
	    (format *OUTPUT* "~Cget-other-buffer~%" *ESCAPE*)
#-debug	    (setq value (read *INPUT*))
	)
    )
    value
)

;;
;; Copyright (c) 2001 by The XFree86 Project, Inc.
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
;; $XFree86: xc/programs/xedit/lisp/modules/fun.lsp,v 1.1 2001/08/31 15:00:14 paulo Exp $
;;
(provide "fun")

(defun caar (a)		(car (car a)))
(defun cadr (a)		(nth 1 a))
(defun cdar (a)		(cdr (car a)))
(defun cddr (a)		(nthcdr 2 a))
(defun caaar (a)	(car (car (car a))))
(defun caadr (a)	(car (car (cdr a))))
(defun cadar (a)	(car (cdr (car a))))
(defun caddr (a)	(nth 2 a))
(defun cdaar (a)	(cdr (car (car a))))
(defun cdadr (a)	(cdr (car (cdr a))))
(defun cddar (a)	(cdr (cdr (car a))))
(defun cdddr (a)	(nthcdr 3 a))
(defun caaaar (a)	(car (car (car (car a)))))
(defun caaadr (a)	(car (car (car (cdr a)))))
(defun caadar (a)	(car (car (cdr (car a)))))
(defun caaddr (a)	(car (car (cdr (cdr a)))))
(defun cadaar (a)	(car (cdr (car (car a)))))
(defun cadadr (a)	(car (cdr (car (cdr a)))))
(defun caddar (a)	(car (cdr (cdr (car a)))))
(defun cadddr (a)	(nth 3 a))
(defun cdaaar (a)	(cdr (car (car (car a)))))
(defun cdaadr (a)	(cdr (car (car (cdr a)))))
(defun cdadar (a)	(cdr (car (cdr (car a)))))
(defun cdaddr (a)	(cdr (car (cdr (cdr a)))))
(defun cddaar (a)	(cdr (cdr (car (car a)))))
(defun cddadr (a)	(cdr (cdr (car (cdr a)))))
(defun cdddar (a)	(cdr (cdr (cdr (car a)))))
(defun cddddr (a)	(nthcdr 4 a))

(defun first (a)	(nth 0 a))
(defun second (a)	(nth 1 a))
(defun third (a)	(nth 2 a))
(defun fourth (a)	(nth 3 a))
(defun fifth (a)	(nth 4 a))
(defun sixth (a)	(nth 5 a))
(defun seventh (a)	(nth 6 a))
(defun eighth (a)	(nth 7 a))
(defun ninth (a)	(nth 8 a))
(defun tenth (a)	(nth 9 a))

(defun rest (a)		(cdr a))

(defmacro push (object place)
    (list 'setf place (list 'cons object place)))

(defmacro pop (place)
    (list 'prog1 (list 'car place) (list 'setf place (list 'cdr place))))

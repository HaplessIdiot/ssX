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
;; $XFree86: xc/programs/xedit/lisp/test/list.lsp,v 1.1 2002/11/21 07:25:17 paulo Exp $
;;

;; basic lisp function tests

;; Most of the tests are just the examples from the
;;
;;	Common Lisp HyperSpec (TM)
;;	Copyright 1996-2001, Xanalys Inc. All rights reserved.
;;
;; Some tests are hand crafted, to test how the interpreter treats
;; uncommon arguments or special conditions


#|
 MAJOR PROBLEMS:

 o NIL and T should be always treated as symbols, actually it is
   legal to say (defun nil (...) ...)
 o There aren't true uninterned symbols, there are only symbols that
   did not yet establish a home package, but once one is created, an
   interned symbol is always returned.
|#


(defun compare-test (test expect function arguments
		     &aux result (error t) unused error-value)
    (multiple-value-setq
	(unused error-value)
	(ignore-errors
	    (setq result (apply function arguments))
	    (setq error nil)
	)
    )
    (if error
	(format t "ERROR: (~A~{ ~S~}) => ~S~%" function arguments error-value)
	(or (funcall test result expect)
	    (format t "(~A~{ ~S~}) => should be ~S not ~S~%"
		function arguments expect result
	    )
	)
    )
)

(defun compare-eval (test expect form
		     &aux result (error t) unused error-value)
    (multiple-value-setq
	(unused error-value)
	(ignore-errors
	    (setq result (eval form))
	    (setq error nil)
	)
    )
    (if error
	(format t "ERROR: ~S => ~S~%" form error-value)
	(or (funcall test result expect)
	    (format t "~S => should be ~S not ~S~%"
		form expect result
	    )
	)
    )
)

(defun error-test (function &rest arguments &aux result (error t))
    (ignore-errors
	(setq result (apply function arguments))
	(setq error nil)
    )
    (or error
	(format t "ERROR: no error for (~A~{ ~S~}), result was ~S~%"
	    function arguments result)
    )
)

(defun error-eval (form &aux result (error t))
    (ignore-errors
	(setq result (eval form))
	(setq error nil)
    )
    (or error
	(format t "ERROR: no error for ~S, result was ~S~%" form result)
    )
)

(defun eq-test (expect function &rest arguments)
    (compare-test #'eq expect function arguments))

(defun eql-test (expect function &rest arguments)
    (compare-test #'eql expect function arguments))

(defun equal-test (expect function &rest arguments)
    (compare-test #'equal expect function arguments))

(defun equalp-test (expect function &rest arguments)
    (compare-test #'equalp expect function arguments))


(defun eq-eval (expect form)
    (compare-eval #'eq expect form))

(defun eql-eval (expect form)
    (compare-eval #'eql expect form))

(defun equal-eval (expect form)
    (compare-eval #'equal expect form))

(defun equalp-eval (expect form)
    (compare-eval #'equalp expect form))

;; apply				- function
(equal-test '((+ 2 3) . 4) #'apply 'cons '((+ 2 3) 4))
(eql-test -1 #'apply #'- '(1 2))
(eql-test 7 #'apply #'max 3 5 '(2 7 3))

;; eq					- function
(eq-eval t '(let* ((a #\a) (b a)) (eq a b)))
(eq-test t #'eq 'a 'a)
(eq-test nil #'eq 'a 'b)
(eq-eval t '(eq #1=1 #1#))
(eq-test nil #'eq "abc" "abc")
(setq a '('x #c(1 2) #\z))
(eq-test nil #'eq a (copy-seq a))

;; eql					- function
(eq-test t #'eql 1 1)
(eq-test t #'eql 1.3d0 1.3d0)
(eq-test nil #'eql 1 1d0)
(eq-test t #'eql #c(1 -5) #c(1 -5))
(eq-test t #'eql 'a 'a)
(eq-test nil #'eql :a 'a)
(eq-test t #'eql #c(5d0 0) 5d0)
(eq-test nil #'eql #c(5d0 0d0) 5d0)
(eq-test nil #'eql "abc" "abc")
(equal-eval '(1 5/6 #p"test" #\#) '(setq a '(1 5/6 #p"test" #\#)))
(eq-test nil #'eql a (copy-seq a))

(setf
    hash0 (make-hash-table)
    hash1 (make-hash-table)
    (gethash 1 hash0) 2
    (gethash 1 hash1) 2
    (gethash :foo hash0) :bar
    (gethash :foo hash1) :bar
)
(defstruct test a b c)
(setq
    struc0 (make-test :a 1 :b 2 :c #\c)
    struc1 (make-test :a 1 :b 2 :c #\c)
)

;; equal				- function
(eq-test t #'equal "abc" "abc")
(eq-test t #'equal 1 1)
(eq-test t #'equal #c(1 2) #c(1 2))
(eq-test nil #'equal #c(1 2) #c(1 2d0))
(eq-test t #'equal #\A #\A)
(eq-test nil #'equal #\A #\a)
(eq-test nil #'equal "abc" "Abc")
(equal-eval '(1 2 3/5 #\a) '(setq a '(1 2 3/5 #\a)))
(eq-test t #'equal a (copy-seq a))
(eq-test nil #'equal hash0 hash1)
(eq-test nil #'equal struc0 struc1)
(eq-test nil #'equal #(1 2 3 4) #(1 2 3 4))

;; equalp				- function
(eq-test t #'equalp hash0 hash1)
(setf
    (gethash 2 hash0) "FoObAr"
    (gethash 2 hash1) "fOoBaR"
)
(eq-test t #'equalp hash0 hash1)
(setf
    (gethash 3 hash0) 3
    (gethash 3d0 hash1) 3
)
(eq-test nil #'equalp hash0 hash1)
(eq-test t #'equalp struc0 struc1)
(setf
    (test-a struc0) #\a
    (test-a struc1) #\A
)
(eq-test t #'equalp struc0 struc1)
(setf
    (test-b struc0) 'test
    (test-b struc1) :test
)
(eq-test nil #'equalp struc0 struc1)
(eq-test t #'equalp #c(1/2 1d0) #c(0.5d0 1))
(eq-test t #'equalp 1 1d0)
(eq-test t #'equalp #(1 2 3 4) #(1 2 3 4))
(eq-test t #'equalp #(1 #\a 3 4d0) #(1 #\A 3 4))

;; acons				- function
(equal-test '((1 . "one")) #'acons 1 "one" nil)
(equal-test '((2 . "two") (1 . "one")) #'acons 2 "two" '((1 . "one")))

;; adjoin				- function
(equal-test '(nil) #'adjoin nil nil)
(equal-test '(a) #'adjoin 'a nil)
(equal-test '(1 2 3) #'adjoin 1 '(1 2 3))
(equal-test '(1 2 3) #'adjoin 2 '(1 2 3))
(equal-test '((1) (1) (2) (3)) #'adjoin '(1) '((1) (2) (3)))
(equal-test '((1) (2) (3)) #'adjoin '(1) '((1) (2) (3)) :key #'car)
(error-test #'adjoin nil 1)

;; alpha-char-p				- function
(eq-test t #'alpha-char-p #\a)
(eq-test nil #'alpha-char-p #\5)
(error-test #'alpha-char-p 'a)

;; alphanumericp			- function
(eq-test t #'alphanumericp #\Z)
(eq-test t #'alphanumericp #\8)
(eq-test nil #'alphanumericp #\#)

;; append				- function
(equal-test '(a b c d e f g) #'append '(a b c) '(d e f) '() '(g))
(equal-test '(a b c . d) #'append '(a b c) 'd)
(eq-test nil #'append)
(eql-test 'a #'append nil 'a)
(error-test #'append 1 2)

;; assoc				- function
(equal-test '(1 . "one") #'assoc 1 '((2 . "two") (1 . "one")))
(equal-test '(2 . "two") #'assoc 2 '((1 . "one") (2 . "two")))
(eq-test nil #'assoc 1 nil)
(equal-test '(2 . "two") #'assoc-if #'evenp '((1 . "one") (2 . "two")))
(equal-test '(3 . "three") #'assoc-if-not #'(lambda(x) (< x 3))
	'((1 . "one") (2 . "two") (3 . "three")))
(equal-test '("two" . 2) #'assoc #\o '(("one" . 1) ("two" . 2) ("three" . 3))
	:key #'(lambda (x) (char x 2)))
(equal-test '(a . b) #'assoc 'a '((x . a) (y . b) (a . b) (a . c)))

;; atom					- function
(eq-test t #'atom 1)
(eq-test t #'atom '())
(eq-test nil #'atom '(1))
(eq-test t #'atom 'a)

;; block				- special operator
(eq-eval nil '(block empty))
(eql-eval 2 '(let ((x 1))
		(block stop (setq x 2) (return-from stop) (setq x 3)) x))
(eql-eval 2 '(block twin (block twin (return-from twin 1)) 2))

;; both-case-p				- function
(eq-test t #'both-case-p #\a)
(eq-test nil #'both-case-p #\1)

;; boundp				- function
(eql-eval 1 '(setq x 1))
(eq-test t #'boundp 'x)
(makunbound 'x)
(eq-test nil #'boundp 'x)
(eq-eval nil '(let ((x 1)) (boundp 'x)))
(error-test #'boundp 1)

;; butlast, nbutlast			- function
(setq x '(1 2 3 4 5 6 7 8 9))
(equal-test '(1 2 3 4 5 6 7 8) #'butlast x)
(equal-eval '(1 2 3 4 5 6 7 8 9) 'x)
(eq-eval nil '(nbutlast x 9))
(equal-test '(1) #'nbutlast x 8)
(equal-eval '(1) 'x)
(eq-test nil #'butlast nil)
(eq-test nil #'nbutlast '())
(error-test #'butlast 1 2)
(error-test #'butlast -1 '(1 2))

;; car, cdr, caar ...			- function
(eql-test 1 #'car '(1 2))
(eql-test 2 #'cdr '(1 . 2))
(eql-test 1 #'caar '((1 2)))
(eql-test 2 #'cadr '(1 2))
(eql-test 2 #'cdar '((1 . 2)))
(eql-test 3 #'cddr '(1 2 . 3))
(eql-test 1 #'caaar '(((1 2))))
(eql-test 2 #'caadr '(1 (2 3)))
(eql-test 2 #'cadar '((1 2) 2 3))
(eql-test 3 #'caddr '(1 2 3 4))
(eql-test 2 #'cdaar '(((1 . 2)) 3))
(eql-test 3 #'cdadr '(1 (2 . 3) 4))
(eql-test 3 #'cddar '((1 2 . 3) 3))
(eql-test 4 #'cdddr '(1 2 3 . 4))
(eql-test 1 #'caaaar '((((1 2)))))
(eql-test 2 #'caaadr '(1 ((2))))
(eql-test 2 #'caadar '((1 (2)) 3))
(eql-test 3 #'caaddr '(1 2 (3 4)))
(eql-test 2 #'cadaar '(((1 2)) 3))
(eql-test 3 #'cadadr '(1 (2 3) 4))
(eql-test 3 #'caddar '((1 2 3) 4))
(eql-test 4 #'cadddr '(1 2 3 4 5))
(eql-test 2 #'cdaaar '((((1 . 2))) 3))
(eql-test 3 #'cdaadr '(1 ((2 . 3)) 4))
(eql-test 3 #'cdadar '((1 (2 . 3)) 4))
(eql-test 4 #'cdaddr '(1 2 (3 . 4) 5))
(eql-test 3 #'cddaar '(((1 2 . 3)) 4))
(eql-test 4 #'cddadr '(1 (2 3 . 4) 5))
(eql-test 4 #'cdddar '((1 2 3 . 4) 5))
(eql-test 5 #'cddddr '(1 2 3 4 . 5))

;; first ... tenth, rest		- function
(eql-test 2 #'rest '(1 . 2))
(eql-test 1 #'first '(1 2))
(eql-test 2 #'second '(1 2 3))
(eql-test 2 #'second '(1 2 3))
(eql-test 3 #'third '(1 2 3 4))
(eql-test 4 #'fourth '(1 2 3 4 5))
(eql-test 5 #'fifth '(1 2 3 4 5 6))
(eql-test 6 #'sixth '(1 2 3 4 5 6 7))
(eql-test 7 #'seventh '(1 2 3 4 5 6 7 8))
(eql-test 8 #'eighth '(1 2 3 4 5 6 7 8 9))
(eql-test 9 #'ninth '(1 2 3 4 5 6 7 8 9 10))
(eql-test 10 #'tenth '(1 2 3 4 5 6 7 8 9 10 11))
(error-test #'car 1)
(error-test #'car #c(1 2))
(error-test #'car #(1 2))

;; case					- macro
(eql-eval t '(let ((a 1)) (case a ((4 5 6) nil) ((3 2 1) t) (otherwise :error))))
(eql-eval t '(let ((a 1)) (case a ((3 2) nil) (1 t) (t :error))))
(error-eval '(let ((a 1)) (case a (2 :error) (t nil) (otherwise t))))
(error-eval '(let ((a 1)) (case a (2 :error) (otherwise t) (t nil))))

;; char					- function
(eql-test #\a #'char "abc" 0)
(eql-test #\b #'char "abc" 1)
(error-test #'char "abc" 3)

;; char-*				- function
(eq-test nil #'alpha-char-p #\3)
(eq-test t #'alpha-char-p #\y)
(eql-test #\a #'char-downcase #\a)
(eql-test #\a #'char-downcase #\a)
(eql-test #\1 #'char-downcase #\1)
(error-test #'char-downcase 1)
(eql-test #\A #'char-upcase #\a)
(eql-test #\A #'char-upcase #\A)
(eql-test #\1 #'char-upcase #\1)
(error-test #'char-upcase 1)
(eq-test t #'lower-case-p #\a)
(eq-test nil #'lower-case-p #\A)
(eq-test t #'upper-case-p #\W)
(eq-test nil #'upper-case-p #\w)
(eq-test t #'both-case-p #\x)
(eq-test nil #'both-case-p #\%)
(eq-test t #'char= #\d #\d)
(eq-test t #'char-equal #\d #\d)
(eq-test nil #'char= #\A #\a)
(eq-test t #'char-equal #\A #\a)
(eq-test nil #'char= #\d #\x)
(eq-test nil #'char-equal #\d #\x)
(eq-test nil #'char= #\d #\D)
(eq-test t #'char-equal #\d #\D)
(eq-test nil #'char/= #\d #\d)
(eq-test nil #'char-not-equal #\d #\d)
(eq-test nil #'char/= #\d #\d)
(eq-test nil #'char-not-equal #\d #\d)
(eq-test t #'char/= #\d #\x)
(eq-test t #'char-not-equal #\d #\x)
(eq-test t #'char/= #\d #\D)
(eq-test nil #'char-not-equal #\d #\D)
(eq-test t #'char= #\d #\d #\d #\d)
(eq-test t #'char-equal #\d #\d #\d #\d)
(eq-test nil #'char= #\d #\D #\d #\d)
(eq-test t #'char-equal #\d #\D #\d #\d)
(eq-test nil #'char/= #\d #\d #\d #\d)
(eq-test nil #'char-not-equal #\d #\d #\d #\d)
(eq-test nil #'char/= #\d #\d #\D #\d)
(eq-test nil #'char-not-equal #\d #\d #\D #\d)
(eq-test nil #'char= #\d #\d #\x #\d)
(eq-test nil #'char-equal #\d #\d #\x #\d)
(eq-test nil #'char/= #\d #\d #\x #\d)
(eq-test nil #'char-not-equal #\d #\d #\x #\d)
(eq-test nil #'char= #\d #\y #\x #\c)
(eq-test nil #'char-equal #\d #\y #\x #\c)
(eq-test t #'char/= #\d #\y #\x #\c)
(eq-test t #'char-not-equal #\d #\y #\x #\c)
(eq-test nil #'char= #\d #\c #\d)
(eq-test nil #'char-equal #\d #\c #\d)
(eq-test nil #'char/= #\d #\c #\d)
(eq-test nil #'char-not-equal #\d #\c #\d)
(eq-test t #'char< #\d #\x)
(eq-test t #'char-lessp #\d #\x)
(eq-test t #'char-lessp #\d #\X)
(eq-test t #'char-lessp #\D #\x)
(eq-test t #'char-lessp #\D #\X)
(eq-test t #'char<= #\d #\x)
(eq-test t #'char-not-greaterp #\d #\x)
(eq-test t #'char-not-greaterp #\d #\X)
(eq-test t #'char-not-greaterp #\D #\x)
(eq-test t #'char-not-greaterp #\D #\X)
(eq-test nil #'char< #\d #\d)
(eq-test nil #'char-lessp #\d #\d)
(eq-test nil #'char-lessp #\d #\D)
(eq-test nil #'char-lessp #\D #\d)
(eq-test nil #'char-lessp #\D #\D)
(eq-test t #'char<= #\d #\d)
(eq-test t #'char-not-greaterp #\d #\d)
(eq-test t #'char-not-greaterp #\d #\D)
(eq-test t #'char-not-greaterp #\D #\d)
(eq-test t #'char-not-greaterp #\D #\D)
(eq-test t #'char< #\a #\e #\y #\z)
(eq-test t #'char-lessp #\a #\e #\y #\z)
(eq-test t #'char-lessp #\a #\e #\y #\Z)
(eq-test t #'char-lessp #\a #\E #\y #\z)
(eq-test t #'char-lessp #\A #\e #\y #\Z)
(eq-test t #'char<= #\a #\e #\y #\z)
(eq-test t #'char-not-greaterp #\a #\e #\y #\z)
(eq-test t #'char-not-greaterp #\a #\e #\y #\Z)
(eq-test t #'char-not-greaterp #\A #\e #\y #\z)
(eq-test nil #'char< #\a #\e #\e #\y)
(eq-test nil #'char-lessp #\a #\e #\e #\y)
(eq-test nil #'char-lessp #\a #\e #\E #\y)
(eq-test nil #'char-lessp #\A #\e #\E #\y)
(eq-test t #'char<= #\a #\e #\e #\y)
(eq-test t #'char-not-greaterp #\a #\e #\e #\y)
(eq-test t #'char-not-greaterp #\a #\E #\e #\y)
(eq-test t #'char> #\e #\d)
(eq-test t #'char-greaterp #\e #\d)
(eq-test t #'char-greaterp #\e #\D)
(eq-test t #'char-greaterp #\E #\d)
(eq-test t #'char-greaterp #\E #\D)
(eq-test t #'char>= #\e #\d)
(eq-test t #'char-not-lessp #\e #\d)
(eq-test t #'char-not-lessp #\e #\D)
(eq-test t #'char-not-lessp #\E #\d)
(eq-test t #'char-not-lessp #\E #\D)
(eq-test t #'char> #\d #\c #\b #\a)
(eq-test t #'char-greaterp #\d #\c #\b #\a)
(eq-test t #'char-greaterp #\d #\c #\b #\A)
(eq-test t #'char-greaterp #\d #\c #\B #\a)
(eq-test t #'char-greaterp #\d #\C #\b #\a)
(eq-test t #'char-greaterp #\D #\C #\b #\a)
(eq-test t #'char>= #\d #\c #\b #\a)
(eq-test t #'char-not-lessp #\d #\c #\b #\a)
(eq-test t #'char-not-lessp #\d #\c #\b #\A)
(eq-test t #'char-not-lessp #\D #\c #\b #\a)
(eq-test t #'char-not-lessp #\d #\C #\B #\a)
(eq-test nil #'char> #\d #\d #\c #\a)
(eq-test nil #'char-greaterp #\d #\d #\c #\a)
(eq-test nil #'char-greaterp #\d #\d #\c #\A)
(eq-test nil #'char-greaterp #\d #\D #\c #\a)
(eq-test nil #'char-greaterp #\d #\D #\C #\a)
(eq-test t #'char>= #\d #\d #\c #\a)
(eq-test t #'char-not-lessp #\d #\d #\c #\a)
(eq-test t #'char-not-lessp #\d #\D #\c #\a)
(eq-test t #'char-not-lessp #\D #\d #\c #\a)
(eq-test t #'char-not-lessp #\D #\D #\c #\A)
(eq-test nil #'char> #\e #\d #\b #\c #\a)
(eq-test nil #'char-greaterp #\e #\d #\b #\c #\a)
(eq-test nil #'char-greaterp #\E #\d #\b #\c #\a)
(eq-test nil #'char-greaterp #\e #\D #\b #\c #\a)
(eq-test nil #'char-greaterp #\E #\d #\B #\c #\A)
(eq-test nil #'char>= #\e #\d #\b #\c #\a)
(eq-test nil #'char-not-lessp #\e #\d #\b #\c #\a)
(eq-test nil #'char-not-lessp #\e #\d #\b #\c #\A)
(eq-test nil #'char-not-lessp #\E #\d #\B #\c #\a)

;; char-code				- function
;; XXX assumes ASCII
(eql-test 49 #'char-code #\1)
(eql-test 90 #'char-code #\Z)
(eql-test 127 #'char-code #\Delete)
(eql-test 27 #'char-code #\Escape)
(eql-test 13 #'char-code #\Return)
(eql-test 0 #'char-code #\Null)
(eql-test 10 #'char-code #\Newline)
(error-test #'char-code 65)

;; character				- function
(eql-test #\a #'character #\a)
(eql-test #\a #'character "a")
(eql-test #\A #'character 'a)

;; XXX assumes ASCII, and should be allowed to fail?
(eql-test #\A #'character 65)

(error-test #'character 1/2)
(error-test #'character "abc")
(error-test #'character :test)
(eq-test #\T #'character t)
(error-test #'character nil)

;; characterp				- function
(eq-test t #'characterp #\a)
(eq-test nil #'characterp 1)
(eq-test nil #'characterp 1/2)
(eq-test nil #'characterp 'a)
(eq-test nil #'characterp '`a)




;; TODO coerce




;; cond					- macro
(eql-eval 2 '(let ((a 1)) (cond ((= a 2) 1) ((= a 1) 2) ((= a 0) 1) (t nil))))
(eql-eval nil '(let ((a 1)) (cond ((= a 2) 1) (t nil) ((= a 1) 2) ((= a 0) 1))))

;; consp				- function (predicate)
(eq-test t #'consp '(1 2))
(eq-test t #'consp '(1 . 2))
(eq-test nil #'consp nil)
(eq-test nil #'consp 1)

;; constantp				- function (predicate)
(eq-test t #'constantp 1)
(eq-test t #'constantp #\x)
(eq-test t #'constantp :test)
(eq-test nil #'constantp 'test)
(eq-test t #'constantp ''1)
(eq-test t #'constantp '(quote 1))
(eq-test t #'constantp "string")
(eq-test t #'constantp #c(1 2))
(eq-test t #'constantp #(1 2))
(eq-test nil #'constantp #p"test")
(eq-test nil #'constantp '(1 2))
(eq-test nil #'constantp (make-hash-table))
(eq-test nil #'constantp *package*)
(eq-test nil #'constantp *standard-input*)

;; copy-list, copy-alist and copy-tree	- function
(equal-test '(1 2) #'copy-list '(1 2))
(equal-test '(1 . 2) #'copy-list '(1 . 2))
(eq-test nil #'copy-list nil)
(error-test #'copy-list 1)
(equal-eval '(1 (2 3)) '(setq x '(1 (2 3))))
(equal-eval x '(setq y (copy-list x)))
(equal-test '("one" (2 3)) #'rplaca x "one")
(eql-test 1 #'car y)
(equal-test '("two" 3) #'rplaca (cadr x) "two")
(eq-test (caadr x) #'caadr y)
(equal-eval '(1 (2 3) 4) '(setq a '(1 (2 3) 4) b (copy-list a)))
(eq-eval t '(eq (cadr a) (cadr b)))
(eq-eval t '(eq (car a) (car b)))
(setq a '(1 (2 3) 4) b (copy-alist a))
(eq-eval nil '(eq (cadr a) (cadr b)))
(eq-eval t '(eq (car a) (car b)))
(eq-test nil #'copy-alist nil)
(eq-test nil #'copy-list nil)
(error-test #'copy-list 1)
(setq a '(1 (2 (3))))
(setq as-list (copy-list a))
(setq as-alist (copy-alist a))
(setq as-tree (copy-tree a))
(eq-eval t '(eq (cadadr a) (cadadr as-list)))
(eq-eval t '(eq (cadadr a) (cadadr as-alist)))
(eq-eval nil '(eq (cadadr a) (cadadr as-tree)))

;; decf					- macro
(setq n 2)
(eql-eval 1 '(decf n))
(eql-eval 1 'n)
(setq n -2147483648)
(eql-eval -2147483649 '(decf n))
(eql-eval -2147483649 'n)
(setq n 0)
(eql-eval -0.5d0 '(decf n 0.5d0))
(eql-eval -0.5d0 'n)
(setq n 1)
(eql-eval 1/2 '(decf n 1/2))
(eql-eval 1/2 'n)

;; delete and remove			- function
(setq a '(1 3 4 5 9) b a)
(equal-test '(1 3 5 9) #'remove 4 a)
(eq-eval t '(eq a b))
(setq a (delete 4 a))
(equal-eval '(1 3 5 9) 'a)
(setq a '(1 2 4 1 3 4 5) b a)
(equal-test '(1 2 1 3 5) #'remove 4 a)
(eq-eval t '(eq a b))
(equal-test '(1 2 1 3 4 5) #'remove 4 a :count 1)
(eq-eval t '(eq a b))
(equal-test '(1 2 4 1 3 5) #'remove 4 a :count 1 :from-end t)
(eq-eval t '(eq a b))
(equal-test '(4 3 4 5) #'remove 3 a :test #'>)
(eq-eval t '(eq a b))
(setq a (delete 4 '(1 2 4 1 3 4 5)))
(equal-eval '(1 2 1 3 5) 'a)
(setq a (delete 4 '(1 2 4 1 3 4 5) :count 1))
(equal-eval '(1 2 1 3 4 5) 'a)
(setq a (delete 4 '(1 2 4 1 3 4 5) :count 1 :from-end t))
(equal-eval '(1 2 4 1 3 5) 'a)
(equal-test "abc" #'delete-if #'digit-char-p "a1b2c3")
(equal-test "123" #'delete-if-not #'digit-char-p "a1b2c3")
(eq-test nil #'delete 1 nil)
(eq-test nil #'remove 1 nil)
(setq a '(1 2 3 4 :test 5 6 7 8) b a)
(equal-test '(1 2 :test 7 8) #'remove-if #'numberp a :start 2 :end 7)
(eq-eval t '(eq a b))
(setq a (delete-if #'numberp a :start 2 :end 7))
(equal-eval '(1 2 :test 7 8) 'a)

;; digit-char				- function
(eql-test #\0 #'digit-char 0)
(eql-test #\A #'digit-char 10 11)
(eq-test nil #'digit-char 10 10)
(eql-test 35 #'digit-char-p #\z 36)
(error-test #'digit-char #\a)
(error-test #'digit-char-p 1/2)



;; TODO directory (known to have problems with parameters like "../*/../*/")



;; elt					- function
(eql-test #\a #'elt "xabc" 1)
(eql-test 3 #'elt '(0 1 2 3) 3)
(error-test #'elt nil 0)

;; endp					- function
(eql-test t #'endp nil)
(error-test #'endp t)
(eql-test nil #'endp '(1 . 2))
(error-test #'endp #(1 2))

;; fboundp and fmakunbound		- function
(eq-test t #'fboundp 'car)
(eq-eval 'test '(defun test ()))
(eq-test t #'fboundp 'test)
(eq-test 'test #'fmakunbound 'test)
(eq-test nil #'fboundp 'test)
(eq-eval 'test '(defmacro test (x) x))
(eq-test t #'fboundp 'test)
(eq-test 'test #'fmakunbound 'test)

;; fill					- function
(setq x (list 1 2 3 4))
(equal-test '((4 4 4 4) (4 4 4 4) (4 4 4 4) (4 4 4 4)) #'fill x '(4 4 4 4))
(eq-eval t '(eq (car x) (cadr x)))
(equalp-test '#(a z z d e) #'fill '#(a b c d e) 'z :start 1 :end 3)
(equal-test "012ee" #'fill "01234" #\e :start 3)
(error-test #'fill 1 #\a)

;; find
(eql-test #\Space #'find #\d "here are some letters that can be looked at" :test #'char>)
(eql-test 3 #'find-if #'oddp '(1 2 3 4 5) :end 3 :from-end t)
(eq-test nil #'find-if-not #'complexp '#(3.5 2 #C(1.0 0.0) #C(0.0 1.0)) :start 2)
(eq-test nil #'find 1 "abc")
(error-test #'find 1 #c(1 2))



;; TODO format



;; funcall				- function
(eql-test 6 #'funcall #'+ 1 2 3)
(eql-test 1 #'funcall #'car '(1 2 3))
(equal-test '(1 2 3) #'funcall #'list 1 2 3)



;; TODO properly implement function



;; functionp				- function (predicate)
(eq-test nil #'functionp 'append)
(eq-test t #'functionp #'append)
(eq-test nil #'functionp '(lambda (x) (* x x)))
(eq-test t #'functionp #'(lambda (x) (* x x)))
(eq-test t #'functionp (symbol-function 'append))
(eq-test nil #'functionp 1)
(eq-test nil #'functionp nil)

;; gensym				- function
(setq sym1 (gensym))
(eq-test nil #'symbol-package sym1)
;; XXX this will fail, uninterned symbols with the same printed
;; representation are currently eq in the interpreter, that is,
;; they are in a NULL package, just waiting to be claimed by a
;; real package.
(setq sym1 (gensym 100))
(setq sym2 (gensym 100))
(eq-test nil #'eq sym1 sym2)
(eq-test nil #'equalp (gensym) (gensym))

;; graphic-char-p			- function
(eq-test t #'graphic-char-p #\a)
(eq-test t #'graphic-char-p #\Space)
(eq-test nil #'graphic-char-p #\Newline)
(eq-test nil #'graphic-char-p #\Tab)
(eq-test nil #'graphic-char-p #\Rubout)

;; if					- special operator
(eq-eval nil '(if nil t))
(eq-eval nil '(if t nil t))
(eq-eval nil '(if nil t nil))
(eq-eval nil '(if nil t (if nil (if nil t) nil)))

;; incf					- macro
(setq n 1)
(eql-eval 2 '(incf n))
(eql-eval 2 'n)
(setq n 2147483647)
(eql-eval 2147483648 '(incf n))
(eql-eval 2147483648 'n)
(setq n 0)
(eql-eval 0.5d0 '(incf n 0.5d0))
(eql-eval 0.5d0 'n)
(setq n 1)
(eql-eval 3/2 '(incf n 1/2))
(eql-eval 3/2 'n)

;; keywordp				- function (predicate)
(eq-test t #'keywordp :test)
(eq-test nil #'keywordp 'test)
(eq-test nil #'keywordp '#:test)
(eq-test nil #'keywordp 1)
(eq-test nil #'keywordp #'keywordp)
(eq-test nil #'keywordp nil)

;; last					- function
(equal-test '(3) #'last '(1 2 3))
(equal-test '(2 . 3) #'last '(1 2 . 3))
(eq-test nil #'last nil)
(eql-test () #'last '(1 2 3) 0)
(setq a '(1 . 2))
(eql-test 2 #'last a 0)
(eq-test a #'last a 1)
(eq-test a #'last a 2)
(eq-test t #'last t)
(equal-test #c(1 2) #'last #c(1 2))
(equalp-test #(1 2 3) #'last #(1 2 3))

;; length				- function
(eql-test 3 #'length "abc")
(eql-test 0 #'length nil)
(eql-test 1 #'length '(1 . 2))
(eql-test 2 #'length #(1 2))
(error-test #'length #c(1 2))
(error-test #'length t)

;; let					- special operator
(eql-eval 2 '(setq a 1 b 2))
(eql-eval 2 '(let ((a 2)) a))
(eql-eval 1 'a)
(eql-eval 1 '(let ((a 3) (b a)) b))
(eql-eval 2 'b)

;; let*					- special operator
(setq a 1 b 2)
(eql-eval 2 '(let* ((a 2)) a))
(eql-eval 1 'a)
(eql-eval 3 '(let* ((a 3) (b a)) b))
(eql-eval 2 'b)

;; list					- function
(equal-test '(1) #'list 1)
(equal-test '(3 4 a b 4) #'list 3 4 'a (car '(b . c)) (+ 6 -2))
(eq-test nil #'list)

;; list*				- function
(eql-test 1 #'list* 1)
(equal-test '(a b c . d) #'list* 'a 'b 'c 'd)
(error-test #'list*)
(setq a '(1 2))
(eq-test a #'list* a)

;; listp				- function (predicate)
(eq-test t #'listp nil)
(eq-test t #'listp '(1 . 2))
(eq-test nil #'listp t)
(eq-test nil #'listp #'listp)
(eq-test nil #'listp #(1 2))
(eq-test nil #'listp #c(1 2))

;; lower-case-p				- function
(eq-test t #'lower-case-p #\a)
(eq-test nil #'lower-case-p #\1)
(eq-test nil #'lower-case-p #\Newline)
(error-test #'lower-case-p 1)



;; TODO make-array	(will be rewritten)



;; make-list				- function
(equal-test '(nil nil nil) #'make-list 3)
(equal-test '((1 2) (1 2)) #'make-list 2 :initial-element '(1 2))
(eq-test nil #'make-list 0)
(eq-test nil #'make-list 0 :initial-element 1)

;; make-package				- function
(setq pack1 (make-package "PACKAGE-1" :nicknames '("PACK-1" "PACK1")))
(setq pack2 (make-package "PACKAGE-2" :nicknames '("PACK-2" "PACK2") :use '("PACK1")))
(equal-test (list pack2) #'package-used-by-list pack1)
(equal-test (list pack1) #'package-use-list pack2)
(eq-test pack1 #'symbol-package 'pack1::test)
(eq-test pack2 #'symbol-package 'pack2::test)



;; TODO make-pathname	(pathnames are broken)



;; make-string				- function
(equal-test "55555" #'make-string 5 :initial-element #\5)
(equal-test "" #'make-string 0)
(error-test #'make-string 10 :initial-element t)
(error-test #'make-string 10 :initial-element nil)
(error-test #'make-string 10 :initial-element 1)
(eql-test 10 #'length (make-string 10))

;; make-symbol				- function
(setq a "TEST")
;; This will fail
(eq-test nil #'eq (make-symbol a) (make-symbol a))
(equal-test a #'symbol-name (make-symbol a))

;; makunbound				- function
(eq-eval 1 '(setf (symbol-value 'a) 1))
(eq-test t #'boundp 'a)
(eql-eval 1 'a)
(eq-test 'a #'makunbound 'a)
(eq-test nil #'boundp 'a)
(error-test #'makunbound 1)

;; mapc					- function
(setq dummy nil)
(equal-test '(1 2 3 4)
    #'mapc #'(lambda (&rest x) (setq dummy (append dummy x)))
   '(1 2 3 4)
   '(a b c d e)
   '(x y z))
(equal-eval '(1 a x 2 b y 3 c z) 'dummy)

;; mapcar				- function
(equal-test '(1 2 3) #'mapcar #'car '((1 a) (2 b) (3 c)))
(equal-test '(3 4 2 5 6) #'mapcar #'abs '(3 -4 2 -5 -6))
(equal-test '((a . 1) (b . 2) (c . 3)) #'mapcar #'cons '(a b c) '(1 2 3))
(equal-test '((1 3 5)) #'mapcar #'list* '(1 2) '(3 4) '((5)))
(equal-test '((1 3 5) (2 4 6)) #'mapcar #'list* '(1 2) '(3 4) '((5) (6)))

;; mapl					- function
(setq dummy nil)
(equal-test '(1 2 3 4) #'mapl #'(lambda (x) (push x dummy)) '(1 2 3 4))
(equal-eval '((4) (3 4) (2 3 4) (1 2 3 4)) 'dummy)

;; maplist				- function
(equal-test '((1 2 3 4 1 2 1 2 3) (2 3 4 2 2 3))
    #'maplist #'append '(1 2 3 4) '(1 2) '(1 2 3))
(equal-test '((foo a b c d) (foo b c d) (foo c d) (foo d))
    #'maplist #'(lambda (x) (cons 'foo x)) '(a b c d))
(equal-test '(0 0 1 0 1 1 1)
    #'maplist #'(lambda (x) (if (member (car x) (cdr x)) 0 1)) '(a b a c d b c))

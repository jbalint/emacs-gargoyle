;;; gargoyle.el --- Java in Emacs

;; Copyright (C) 2016 Jess Balint
;; Author: Jess Balint <jbalint@gmail.com>
;; Maintainer: Jess Balint <jbalint@gmail.com>
;; Created: 13 Feb 2016
;; Keywords: java
;; Homepage: https://github.com/jbalint/emacs-gargoyle

;; This file is free software
;; along with this file.  If not, see <http://www.gnu.org/licenses/>.

;; The MIT License (MIT)
;; 
;; Copyright (c) 2016 Jess Balint
;; 
;; Permission is hereby granted, free of charge, to any person obtaining a copy
;; of this software and associated documentation files (the "Software"), to deal
;; in the Software without restriction, including without limitation the rights
;; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;; copies of the Software, and to permit persons to whom the Software is
;; furnished to do so, subject to the following conditions:
;; 
;; The above copyright notice and this permission notice shall be included in all
;; copies or substantial portions of the Software.
;; 
;; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;; SOFTWARE.

;; This file is not part of GNU Emacs.

;;; Commentary:

;; The Gargoyle system provides a facility for combining Java and
;; Emacs/Elisp. This file is the Emacs side of the bridge. The
;; remainder is implemented as a dynamic module (must be enabled in
;; Emacs 25 builds).

;;; Change Log:

;;; Code:

(require 'gargoyle-dm)

(require 'cl-lib)

(defvar gg-to-java-mappings nil
  "Mappings to Java objects")

(defvar gg--class-hierarchy (make-hash-table)
  "Class hierarchy (c.f. internals.org)")

;; type predicates - mainly for internal use
(defun gg--type-arrayp (type)
  "Return t if `type' is an array type."
  (and
   (listp type)
   (eq (car type) 'gg-array)))

(defun gg--type-array-component (array-type)
  "Return the component type of the given array type."
  (if (gg--type-arrayp array-type)
      (cdr array-type)
    nil))

(defun gg--type-primitivep (type)
  "Return t if `type' is a primitive type."
  (and
   (listp type)
   (eq (car type) 'gg-prim)))

(defun gg--type-classp (type)
  "Return t if `type' is a Java class type."
  (eq (type-of type) 'symbol))

;; object type predicates
(defun gg-objectp (object)
  "Return t if `object' is a Java object."
  (and
   (listp object)
   (eq (car object) 'gg-obj)))

(defun gg-classp (class)
  "Return t if `class' is a Java class."
  (and
   (listp class)
   (eq (car class) 'gg-obj)
   (eq (nth 2 class) 'java.lang.Class)))

(defun gg--new-object (ptr class-name-sym)
  "Create a new object from a raw JNI pointer."
  (list 'gg-obj ptr class-name-sym))

(defun gg-toString (obj)
  "Return the string representation of the object."
  (gg--toString-raw (cadr obj)))

(defun gg-new (class-or-name &rest ctor-args)
  "Create a new instance."
  (let ((class (cond
				((stringp class-or-name) (gg-find-class class-or-name))
				((gg-objectp class-or-name) class-or-name)
				(t (signal 'wrong-type-argument `("Expected class object or class name string:"
												  ,(type-of class-or-name)
												  ,class-or-name))))))
	(gg--new-raw (cadr class))))

(define-error 'java-exception
  "A Java exception. The cdr is the exception.")

(defun gg--class-add-by-name (class-name-sym)
  "Add a class to the class hierarchy by providing the name of the class"
  (unless (gethash class-name-sym gg--class-hierarchy)))

(defun gg--class-add-by-obj (class))

(defun gg-get-class-name (class)
  (gg--get-class-name-raw (cadr class)))

(defun gg-get-superclass (class)
  (gg--get-superclass-raw (cadr class)))

(provide 'gargoyle)

;;; gargoyle.el ends here

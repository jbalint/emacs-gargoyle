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
;; Emacs/Elisp. This file is the Emacs side of the side. The remainder
;; is implemented as a dynamic module (must be enabled in Emacs 25
;; builds).

;;; Change Log:

;;; Code:

(require 'gargoyle-dm)

(defun gg-objectp (object)
  "Return t if `object' is a Java object."
  (and
   (listp object)
   (eq (car object) 'gg-obj)))

(defun gg--new-object (ptr)
  "Create a new object from a raw JNI pointer."
  `(gg-obj ,ptr))

(provide 'gargoyle)

;;; gargoyle.el ends here

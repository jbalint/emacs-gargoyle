(ert-deftest new-string ()
  "Basic test of `gg-new-string'"
  (let* ((a-string (gg-new-string "some string"))
		 (back-to-lisp (gg-toString a-string)))
	(should (string-equal "some string" back-to-lisp))
    (should (gg-objectp a-string))
    (should (not (gg-objectp back-to-lisp)))))

(ert-deftest big-string ()
  (let* ((the-string (make-string 100000 ?x))
		 (a-string (gg-new-string the-string))
		 (back-to-lisp (gg-toString a-string)))
	(should (string-equal the-string back-to-lisp))))

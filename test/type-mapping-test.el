(ert-deftest lisp-string-to-java-string ()
  "Test the built-in Lisp string to Java String mapping"
  :expected-result :failed
  (setq 'gg-to-java-mapping-alist
		'(java.util.String stringp gg-new-string))
  (let ((t (gg-new "java.lang.Thread")))
	(/setName t "New thread name")
	(should (string-equal "New thread name" (/getName t)))))

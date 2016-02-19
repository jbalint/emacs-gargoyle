(ert-deftest basic-class-access ()
  (let* ((c (gg-find-class "java.util.ArrayList"))
		 (c-as-string (gg-toString c)))
	(should (string-equal "class java.util.ArrayList" c-as-string))))

(ert-deftest no-class-def ()
  (condition-case exc (gg-find-class "java.DoesntExist")
	(java-exception (should (eq 'java-exception (car exc))))))

(ert-deftest find-class-on-non-string ()
  (condition-case err (gg-find-class nil)
	(wrong-type-argument (should (string-equal (cadr err) "Expected string"))))
  (condition-case err (gg-find-class 42)
	(wrong-type-argument (should (string-equal (cadr err) "Expected string")))))

(ert-deftest basic-class-access ()
  (let* ((c (gg-find-class "java.util.ArrayList"))
		 (c-as-string (gg-toString c)))
	(should (string-equal "class java.util.ArrayList" c-as-string))))

(ert-deftest no-class-def ()
  (condition-case exc (gg-find-class "java.DoesntExist")
	(java-exception (should (eq 'java-exception (car exc))))))

(ert-deftest find-class-on-non-string ()
  (condition-case err (gg-find-class nil)
	(wrong-type-argument (should (string-equal "Expected string:" (cadr err)))))
  (condition-case err (gg-find-class 42)
	(wrong-type-argument (should (string-equal "Expected string:" (cadr err))))))

(ert-deftest new-instance-0arg ()
  (let* ((o (gg-new "java.util.ArrayList"))
		 (o-as-string (gg-toString o)))
	(should (string-equal "[]" o-as-string)))
  (let* ((o (gg-new (gg-find-class "java.util.ArrayList")))
		 (o-as-string (gg-toString o)))
	(should (string-equal "[]" o-as-string))))

(ert-deftest new-badtype ()
  (condition-case err (gg-new nil)
	(wrong-type-argument (should (string-equal "Expected class object or class name string:"
											   (cadr err))))))

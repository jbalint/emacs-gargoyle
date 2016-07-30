(ert-deftest basic-class-access-string ()
  "Can we load a class given the class name as a string"
  (let* ((c (gg-find-class "java.util.ArrayList"))
		 (c-as-string (gg-toString c))
		 (class-name (gg-get-class-name c)))
	(should (string-equal "class java.util.ArrayList" c-as-string))
	(should (equal 'java.util.ArrayList class-name))))

(ert-deftest basic-class-access-symbol ()
  "Can we load a class given the class name as a symbol"
  :expected-result :failed
  (let* ((c (gg-find-class 'java.util.ArrayList))
		 (c-as-string (gg-toString c)))
	(should (string-equal "class java.util.ArrayList" c-as-string))))

(ert-deftest no-class-def ()
  "Does an attempt to load a non-existent class throw a proper exception?"
  (message "* The following NoClassDefError exception is expected:")
  (condition-case exc (gg-find-class "java.DoesntExist")
	(java-exception (should (eq 'java-exception (car exc))))))

(ert-deftest find-class-on-non-string ()
  "Does an attempt to load a class given a wrong type throw a proper exception?"
  (condition-case err (gg-find-class nil)
	(wrong-type-argument (should (string-equal "Expected string:" (cadr err)))))
  (condition-case err (gg-find-class 42)
	(wrong-type-argument (should (string-equal "Expected string:" (cadr err))))))

(ert-deftest new-instance-0arg ()
  "Can we call a constructor with no arguments?"
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

(ert-deftest get-superclass ()
  (let* ((c (gg-find-class "java.util.ArrayList"))
         (sc (gg-get-superclass c))
         (sc-name (gg-get-class-name sc)))
    (should (equal 'java.util.AbstractList sc-name))))

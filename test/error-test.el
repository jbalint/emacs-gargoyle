(ert-deftest cant-start-two-vms-error-test ()
  "We should get an error when attempt to start the VM when it's
already running"
  (should (eq t (gg-java-running)))
  (condition-case err (gg-java-start)
	(error
	 (should (string-equal "JVM already running" (cdr err))))))

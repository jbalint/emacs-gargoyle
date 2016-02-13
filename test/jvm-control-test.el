;; Simple test we just start/stop the JVM
(ert-deftest basic-control-test ()
  (should (eq nil (gg-java-running)))
  (should (eq t (gg-java-start)))
  (should (eq t (gg-java-running)))
  (should (string-equal "1.8" (gg-jni-version)))
  (should (eq t (gg-java-stop)))
  (should (eq nil (gg-java-running))))

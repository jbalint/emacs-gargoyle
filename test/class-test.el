(ert-deftest basic-class-access-string ()
  "Can we load a class given the class name as a string"
  (let* ((c (gg-find-class "java.util.ArrayList"))
		 (c-as-string (gg-toString c))
		 (class-name (gg-get-class-name c)))
	(should (string-equal "class java.util.ArrayList" c-as-string))
    (should (gg-objectp c))
    (should (gg-classp c))
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
    (should (not (gg-classp o)))
    (should (gg-objectp o))
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

;;;;;;;;;;;;;;;;;;;;;
;; Class structure ;;
;;;;;;;;;;;;;;;;;;;;;

(defun find-method (name class-struct)
  (let ((methods (plist-get class-struct :methods)))
    (cl-find-if (lambda (m) (eq name (plist-get m :name))) methods)))

(ert-deftest class-method-modifiers ()
  "Test that all method modifiers are exposed properly"
  (let* (;; use Thread's methods for most of the tests
         (thread-struct (gg--get-class-struct 'java.lang.Thread))
         (currentThread-method (find-method 'currentThread thread-struct))
         (setName-method (find-method 'setName thread-struct))
         (clone-method (find-method 'clone thread-struct))
         (setNativeName-method (find-method 'setNativeName thread-struct))
         ;; good example using a covariant return type which generates a bridge method
         (isochronology-struct (gg--get-class-struct 'java.time.chrono.IsoChronology))
         (period-method (find-method 'period isochronology-struct))
         ;; has an abstract+varargs method
         (filesystem-struct (gg--get-class-struct 'java.nio.file.FileSystem))
         (getPath-method (find-method 'getPath filesystem-struct))
         ;; for a strictfp method
         (strictmath-struct (gg--get-class-struct 'java.lang.StrictMath))
         (toRadians-method (find-method 'toRadians strictmath-struct)))
    (should (equal '(public static native) (plist-get currentThread-method :modifiers)))
    (should (equal '(public final synchronized) (plist-get setName-method :modifiers)))
    (should (equal '(protected) (plist-get clone-method :modifiers)))
    (should (equal '(private native) (plist-get setNativeName-method :modifiers)))

    (should (equal '(public bridge synthetic) (plist-get period-method :modifiers)))

    (should (equal '(public varargs abstract) (plist-get getPath-method :modifiers)))

    (should (equal '(public static strictfp) (plist-get toRadians-method :modifiers)))))

(ert-deftest class-struct-method-return-types ()
  "Test that all types regarding methods are represented properly"
  (let* ((thread-struct (gg--get-class-struct 'java.lang.Thread))
         (getName-method (find-method 'getName thread-struct))
         (getName-returns (plist-get getName-method :returns))
         (dumpStack-method (find-method 'dumpStack thread-struct))
         (dumpStack-returns (plist-get dumpStack-method :returns))
         (getPriority-method (find-method 'getPriority thread-struct))
         (getPriority-returns (plist-get getPriority-method :returns))
         (getStackTrace-method (find-method 'getStackTrace thread-struct))
         (getStackTrace-returns (plist-get getStackTrace-method :returns)))

    (should (equal 'java.lang.String getName-returns))
    (should (not (gg--type-arrayp getName-returns)))
    (should (not (gg--type-primitivep getName-returns)))
    (should (gg--type-classp getName-returns))
    (should (eq nil (gg--type-array-component getName-returns)))

    (should (equal '(gg-prim . V) dumpStack-returns))
    (should (not (gg--type-arrayp dumpStack-returns)))
    (should (not (gg--type-classp dumpStack-returns)))
    (should (gg--type-primitivep dumpStack-returns))
    (should (eq nil (gg--type-array-component dumpStack-returns)))

    (should (equal '(gg-prim . I) getPriority-returns))
    (should (not (gg--type-arrayp getPriority-returns)))
    (should (not (gg--type-classp getPriority-returns)))
    (should (gg--type-primitivep getPriority-returns))
    (should (eq nil (gg--type-array-component getPriority-returns)))

    (should (equal '(gg-array . java.lang.StackTraceElement) getStackTrace-returns))
    (should (not (gg--type-classp getStackTrace-returns)))
    (should (not (gg--type-primitivep getStackTrace-returns)))
    (should (gg--type-arrayp getStackTrace-returns))
    (should (eq 'java.lang.StackTraceElement (gg--type-array-component getStackTrace-returns)))))

(ert-deftest class-struct-method-arguments ()
  "Test for argument types"
  ;; TODO could be expanded
  (let* ((thread-struct (gg--get-class-struct 'java.lang.Thread))
         (enumerate-method (find-method 'enumerate thread-struct))
         (executor-service-struct (gg--get-class-struct 'java.util.concurrent.ExecutorService))
         (awaitTermination-method (find-method 'awaitTermination executor-service-struct)))
    (should (equal '((gg-array . java.lang.Thread)) (plist-get enumerate-method :accepts)))
    (should (equal '((gg-prim . J) java.util.concurrent.TimeUnit) (plist-get awaitTermination-method :accepts)))))

(ert-deftest class-struct-basic-access ()
  "Can we load the ArrayList struct?"
  (let* ((arraylist-struct (gg--get-class-struct 'java.util.ArrayList)))
    (should (eq 'java.util.ArrayList (plist-get arraylist-struct :name)))
    (should (eq 'java.util.AbstractList (plist-get arraylist-struct :superclass)))
    (should (equal '(java.util.List java.util.RandomAccess java.lang.Cloneable java.io.Serializable) (plist-get arraylist-struct :interfaces)))
    (should (equal '(public super) (plist-get arraylist-struct :modifiers)))
    ;; basic sanity check. There are 49 methods on ArrayList in JAVA 1.8.0_66-b17
    (should (> (length (plist-get arraylist-struct :methods)) 30))
    (should (> (length (plist-get arraylist-struct :fields)) 4))))

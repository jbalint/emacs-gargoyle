;; start from gargoyle directory
(progn
  (add-to-list 'load-path (getenv "PWD"))
  (require 'gargoyle)
  (gg-java-start)
  (find-file "test"))

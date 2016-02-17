(add-to-list 'load-path (getenv "PWD"))
(require 'gargoyle)
;; VM starter for tests requiring a running VM. Never cleaned up / stopped
(gg-java-start)

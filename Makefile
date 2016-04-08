# The MIT License (MIT)
# 
# Copyright (c) 2016 Jess Balint
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Set this externally until Emacs 25 becomes widely used
EMACS ?= /home/jbalint/sw/emacs-sw/emacs/src/emacs
EMACS_INCLUDE=/home/jbalint/sw/emacs-sw/emacs/src

JAVA_INCLUDE=$(JAVA_HOME)/include
# LD_LIBRARY_PATH needs to include this directory
JVM_LIB_DIR=$(JAVA_HOME)/jre/lib/amd64/server

CFLAGS  = -I$(JAVA_INCLUDE) -I$(JAVA_INCLUDE)/linux -I$(EMACS_INCLUDE) -Isrc -std=gnu99 -ggdb3 -Wall -fPIC -D_POSIX_C_SOURCE
LDFLAGS = -L$(JVM_LIB_DIR)

all: gargoyle-dm.so

gargoyle-dm.so: src/class.o src/ctrl.o src/el_util.o src/main.o
	$(LD) -shared $(LDFLAGS) -o $@ $^ -ljvm -ljsig

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

# Separate vm from no-vm tests (this requires fixed ert-runner: https://github.com/rejeep/ert-runner.el/pull/26)
check-no-vm: gargoyle-dm.so
	ls test/no-vm/*-test.el | EMACS=$(EMACS) xargs -n 1 cask exec ert-runner $i

# IF running inside emacs, INSIDE_EMACS must be UNSET for this to work (see Cask Issue #260)
check-vm: gargoyle-dm.so
	EMACS=$(EMACS) cask exec ert-runner -l test/start-vm.el test/*-test.el

check: check-no-vm check-vm

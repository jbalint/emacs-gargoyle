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
EMACS=/home/jbalint/sw/emacs-sw/emacs/src/emacs
EMACS_INCLUDE=/home/jbalint/sw/emacs-sw/emacs/src

JAVA_INCLUDE=$(JAVA_HOME)/include
# LD_LIBRARY_PATH needs to include this directory
JVM_LIB_DIR=$(JAVA_HOME)/jre/lib/amd64/server

CFLAGS  = -I$(JAVA_INCLUDE) -I$(JAVA_INCLUDE)/linux -I$(EMACS_INCLUDE) -Isrc -std=gnu99 -ggdb3 -Wall -fPIC -D_POSIX_C_SOURCE
LDFLAGS = -L$(JVM_LIB_DIR)

all: gargoyle.so

gargoyle.so: src/ctrl.o src/main.o
	$(LD) -shared $(LDFLAGS) -o $@ $^ -ljvm

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

test: gargoyle.so
	for i in test/test-*.el ; do
		$EMACS -batch -l ert -l $i -f ert-run-tests-batch-and-exit
	done

/*
 * The MIT License (MIT)
 * 
 * Copyright (c) 2016 Jess Balint
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Emacs lisp utilities
 */

#include <emacs-module.h>

#define ASSERT_JVM_RUNNING(E) if (!jvm_running(E)) { return NULL; }

void noop_finalizer(void *x);
emacs_value list(emacs_env *env, int n, ...);
int type_is(emacs_env *env, emacs_value v, const char *type_name);
emacs_value new_java_object(emacs_env *env, jobject o, jclass class);
int jvm_running(emacs_env *env);
int handle_exception(emacs_env *env);
emacs_value jstring_to_symbol (emacs_env *env, jstring string);
bool symbol_to_string (emacs_env *env, emacs_value symbol, char *string, ptrdiff_t *size);
int check_jvmti_error(emacs_env *env);
emacs_value jclass_to_symbol (emacs_env *env, jclass class);
jstring get_class_name (emacs_env *env, jclass class);

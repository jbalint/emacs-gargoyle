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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <emacs-module.h>

#include <jni.h>

#include "class.h"
#include "ctrl.h"
#include "el_util.h"

/* finalizer needed as userptr finalizer isn't optional in dynamic modules */
void noop_finalizer(void *x)
{
}

/* create a list */
emacs_value list(emacs_env *env, int n, ...)
{
    va_list ap;
    emacs_value list;
    int i;
    emacs_value *args;

    va_start(ap, n);

    args = malloc(n * sizeof(emacs_value));
    assert(args);

    for (i = 0; i < n; ++i) {
        args[i] = va_arg(ap, emacs_value);
    }

    va_end(ap);

    list = env->funcall(env, env->intern(env, "list"), n, args);

    assert(list);

    free(args);

    return list;
}

/*
 * Check whether the `type-of' an emacs_value matches the given type.
 *
 * e.g. type_is(env, some_string, "string") => 1
 *
 * TODO: rename this (and other functions that push non-local exits to `assert_type', etc
 */
int type_is(emacs_env *env, emacs_value v, const char *type_name)
{
    static char errmsg[50];
    emacs_value t = env->type_of(env, v);
    if (!env->eq(env, t, env->intern(env, type_name))) {
        sprintf(errmsg, "Expected %s:", type_name);
        env->non_local_exit_signal(env, env->intern(env, "wrong-type-argument"),
                                   list(env, 3, env->make_string(env, errmsg, strlen(errmsg)), t, v));
        return 0;
    } else {
        return 1;
    }
}

/*
 * Wrap a jobject pointer to a Lisp "Java object".
 */
emacs_value new_java_object(emacs_env *env, jobject o, jclass class)
{
    emacs_value args[2];
    emacs_value wrapped;
    jstring class_name;

    if (class == NULL) {
        class = (*g_jni)->GetObjectClass(g_jni, o);
        if (handle_exception(env)) { return NULL; }
        assert(class);
        class_name = get_class_name(env, class);
        (*g_jni)->DeleteLocalRef(g_jni, class);
    } else {
        class_name = get_class_name(env, class);
    }

    args[0] = env->make_user_ptr(env, noop_finalizer, o);
    args[1] = jstring_to_symbol(env, class_name);

    (*g_jni)->DeleteLocalRef(g_jni, class_name);

    wrapped = env->funcall(env, env->intern(env, "gg--new-object"), 2, args);
    if (env->non_local_exit_check(env) != emacs_funcall_exit_return) {
        return NULL;
    }
    assert(wrapped);

    /*
     * TODO  TODO  TODO  TODO  TODO  TODO  TODO 
     * Map the methods available on this object (and static methods if
     * it's a class) into the Lisp environment
     */

    return wrapped;
}

/*
 * Assert that the JVM is running. If it's not (error "Gargoyle JVM
 * not running") is thrown. Return 1/true or 0/false for JVM running.
 */
int jvm_running(emacs_env *env)
{
    static const char *err_msg = "Gargoyle JVM not running";
    emacs_value wrapped_err_msg;
    if (g_jni) {
        return 1;
    }
    wrapped_err_msg = env->make_string(env, err_msg, strlen(err_msg));
    assert(wrapped_err_msg);
    env->non_local_exit_signal(env, env->intern(env, "error"), wrapped_err_msg);
    return 0;
}

/*
 * Check for JNI exceptions. If one exists, it will be "thrown" into
 * the Emacs environment as a `java-exception' error.
 */
int handle_exception(emacs_env *env)
{
    jthrowable exception;
    exception = (*g_jni)->ExceptionOccurred(g_jni);
    if (exception) {
        if /* should I print exceptions? */ (1) {
            (*g_jni)->ExceptionDescribe(g_jni);
        }
        (*g_jni)->ExceptionClear(g_jni);
        env->non_local_exit_signal(env, env->intern(env, "java-exception"),
                                   new_java_object(env, exception, NULL));
        return 1;
    }
    return 0;
}

/*
 * Wrap a Java string in an Emacs symbol.
 */
emacs_value jstring_to_symbol (emacs_env *env, jstring string)
{
    const char *bytes;
    jboolean isCopy;
    emacs_value symbol;

    bytes = (*g_jni)->GetStringUTFChars(g_jni, string, &isCopy);
    if (handle_exception(env)) { return NULL; }
    assert(bytes);
    symbol = env->intern(env, bytes);
    (*g_jni)->ReleaseStringUTFChars(g_jni, string, bytes);

    return symbol;
}

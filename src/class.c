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

#include <emacs-module.h>
#include <jvmti.h>
#include <jni.h>

#include "ctrl.h"
#include "el_util.h"

emacs_value
Fgg_get_superclass_raw (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    jclass class;
    jclass superclass;

    if (!type_is(env, args[0], "user-ptr")) {
        return NULL;
    }

    ASSERT_JVM_RUNNING(env);

    class = env->get_user_ptr(env, args[0]);

    superclass = (*g_jni)->GetSuperclass(g_jni, class);
    if (handle_exception(env)) { return NULL; }
    if (superclass == NULL) {
        return env->intern (env, "nil");
    } else {
        return new_java_object(env, superclass, NULL);
    }
}

#define MAX_CLASS_NAME_SIZE 128
emacs_value
Fgg_find_class (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    static char class_name[MAX_CLASS_NAME_SIZE];
    ptrdiff_t size = MAX_CLASS_NAME_SIZE;
    bool ok;
    jclass class;
    int i;
    /* Are we past the class when transforming class name string? If
     * so, use "$" instead of "/". Eg. com.x.Class.Other ->
     * com/x/Class$Other
     */
    int past_class = 0;

    if (!type_is(env, args[0], "string")) {
        return NULL;
    }

    ASSERT_JVM_RUNNING(env);

    ok = env->copy_string_contents(env, args[0], class_name, &size);
    assert(ok);
    assert(size < MAX_CLASS_NAME_SIZE);

    /* replace . with / to conform to internal naming convention */
    for (i = 0; i < MAX_CLASS_NAME_SIZE && class_name[i]; ++i) {
        if (class_name[i] != '.') continue;
        if (past_class) {
            class_name[i] = '$';
        } else {
            class_name[i] = '/';
            if (class_name[i+1] >= 65 && class_name[i+1] <= 90) {
                past_class = 1;
            }
        }
    }
    class = (*g_jni)->FindClass(g_jni, class_name);
    if (class) {
        return new_java_object(env, class, NULL);
    } else {
        handle_exception(env);
        return NULL;
    }
}

/*
 * Get a class's name. The returned jstring is a local reference.
 */
jstring get_class_name (emacs_env *env, jclass class)
{
    jmethodID mid_getName;
    jstring name;

    mid_getName = (*g_jni)->GetMethodID(g_jni, g_java_lang_Class, "getName", "()Ljava/lang/String;");
    handle_exception(env);
    assert(mid_getName);
    if (handle_exception(env)) { assert(0);return NULL; }
    assert(mid_getName);

    name = (*g_jni)->CallObjectMethod(g_jni, class, mid_getName);
    if (handle_exception(env)) { return NULL; }
    assert(name);

    return name;
}

emacs_value
Fgg_get_class_name_raw (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    /* String object return from getName() */
    jobject name;
    emacs_value name_symbol;

    /* TODO, we could also check the pointer is a class, otherwise
     * GetMethodID may fail */

    if (!type_is(env, args[0], "user-ptr")) {
        return NULL;
    }

    ASSERT_JVM_RUNNING(env);

    name = get_class_name(env, env->get_user_ptr(env, args[0]));

    name_symbol = jstring_to_symbol(env, name);

    (*g_jni)->DeleteLocalRef(g_jni, name);

    return name_symbol;
}

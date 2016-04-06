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
        return new_java_object(env, class);
    } else {
        handle_exception(env);
        return NULL;
    }
}

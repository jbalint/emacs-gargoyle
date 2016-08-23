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
#include <stdlib.h>
#include <string.h>

#include <emacs-module.h>
#include <jvmti.h>
#include <jni.h>
#include <classfile_constants.h>

#include "ctrl.h"
#include "el_util.h"

#define GG_ARRAY_TAG "gg-array"
#define GG_PRIMITIVE_TAG "gg-prim"

/*
 * An arbitrary number
 */
#define MAX_CLASS_NAME_SIZE 128

static void class_name_to_fq(char *class_name)
{
    int i;
    for (i = 0; i < MAX_CLASS_NAME_SIZE && class_name[i]; ++i) {
        if (class_name[i] == '/' || class_name[i] == '$') {
            class_name[i] = '.';
        }
    }
}

/*
 * Convert (in-place) a class name to internal format, i.e. replace
 * '.' with '/' and handle inner classes:
 * java.lang.Object -> java/lang/Object
 * com.x.Y.Z -> com/x/Y$Z
 */
static void class_name_to_internal(char *class_name)
{
    int i;
    /* Are we past the class when transforming class name string? If
     * so, use "$" instead of "/". Eg. com.x.Class.Other ->
     * com/x/Class$Other
     */
    int past_class = 0;
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
}

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

emacs_value
Fgg_find_class (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    static char class_name[MAX_CLASS_NAME_SIZE];
    ptrdiff_t size = MAX_CLASS_NAME_SIZE;
    bool ok;
    jclass class;
    emacs_value retval;

    if (!type_is(env, args[0], "string")) {
        return NULL;
    }

    ASSERT_JVM_RUNNING(env);

    ok = env->copy_string_contents(env, args[0], class_name, &size);
    assert(ok);
    assert(size < MAX_CLASS_NAME_SIZE);

    class_name_to_internal(class_name);
    class = (*g_jni)->FindClass(g_jni, class_name);
    if (class) {
        retval = new_java_object(env, class, NULL);
        (*g_jni)->DeleteLocalRef(g_jni, class);
        return retval;
    } else {
        handle_exception(env);
        return NULL;
    }
}

emacs_value
Fgg_get_class_name_raw (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    /* TODO, we could also check the pointer is a class, otherwise
     * GetMethodID will fail */

    if (!type_is(env, args[0], "user-ptr")) {
        return NULL;
    }

    ASSERT_JVM_RUNNING(env);

    return jclass_to_symbol(env, env->get_user_ptr(env, args[0]));
}

static emacs_value wrap_type(emacs_env *env, emacs_value type, const char *wrap_symbol)
{
    emacs_value args[2];
    args[0] = env->intern(env, wrap_symbol);
    args[1] = type;
    return env->funcall(env, env->intern(env, "cons"), 2, args);
}

/*
 * method structure generate - helper method for `Fgg_get_class_struct'
 */
#define fieldID_to_struct_LIST_ARGS 4
static emacs_value fieldID_to_struct(emacs_env *env, jclass class, jfieldID field)
{
    emacs_value list_args[fieldID_to_struct_LIST_ARGS];
    char *name;
    char *sig;
    char *orig_sig;
    int is_array_type = 0;
    emacs_value field_struct;

    g_jvmtiError = (*g_jvmti)->GetFieldName(g_jvmti, class, field, &name, &sig, NULL);
    if (check_jvmti_error(env)) {
        return NULL;
    }
    list_args[0] = env->intern(env, ":name");
    list_args[1] = env->intern(env, name);
    list_args[2] = env->intern(env, ":type");
    orig_sig = sig;
    if (*sig == '[') {
        is_array_type = 1;
        sig++;
    }
    if (*sig == 'L') {
        sig++;
        sig[strlen(sig)-1] = 0; /* remove trailing ; */
        list_args[3] = env->intern(env, sig);
    } else {
        list_args[3] = wrap_type(env, env->intern(env, sig), GG_PRIMITIVE_TAG);
    }
    if (is_array_type) {
        list_args[3] = wrap_type(env, list_args[3], GG_ARRAY_TAG);
    }
    is_array_type = 0;

    field_struct = env->funcall(env, env->intern(env, "list"), fieldID_to_struct_LIST_ARGS, list_args);
    (*g_jvmti)->Deallocate(g_jvmti, (void *) name);
    (*g_jvmti)->Deallocate(g_jvmti, (void *) orig_sig);
    return field_struct;
}

/*
 * method structure generate - helper method for `Fgg_get_class_struct'
 */
#define methodID_to_struct_LIST_ARGS 8
static emacs_value methodID_to_struct(emacs_env *env, jmethodID method)
{
    emacs_value list_args[methodID_to_struct_LIST_ARGS];
    char *name;
    char *sig;
    char *return_type_name = NULL;
    emacs_value method_struct;
    int i, j;
    size_t sig_len;
    static char class_name[MAX_CLASS_NAME_SIZE];
    int arg_count = 0;
    emacs_value *arg_types;
    emacs_value return_type;
    /* TODO realloc() when this limit is hit */
    int MAX_ARGS = 10;
    int is_array_type = 0;
    char prim_type[2] = {0, 0};
    jint modifiers;
    emacs_value modifiers_array[12];
    emacs_value modifiers_list;
    int modifiers_count = 0;

    g_jvmtiError = (*g_jvmti)->GetMethodName(g_jvmti, method, &name, &sig, NULL);
    if (check_jvmti_error(env)) {
        return NULL;
    }

    sig_len = strlen(sig);
    arg_types = malloc(sizeof(emacs_value) * MAX_ARGS);
    /* start at 1, skip the '(' */
    for (i = 1; i < sig_len; ++i) {
        if (sig[i] == '[') {
            is_array_type = 1;
            continue;
        } else if (sig[i] == 'L') {
            j = 0;
            while (sig[++i] != ';') {
                class_name[j++] = sig[i];
            }
            class_name[j] = 0;
            class_name_to_fq(class_name);
            arg_types[arg_count] = env->intern(env, class_name);
        } else if (sig[i] == ')') {
            return_type_name = sig + i + 1;
            if (*return_type_name == '[') {
                is_array_type = 1;
                return_type_name++;
            }
            if (*return_type_name == 'L') {
                return_type_name++;
                *strchr(return_type_name, ';') = 0;
                class_name_to_fq(return_type_name);
                return_type = env->intern(env, return_type_name);
            } else {
                prim_type[0] = *return_type_name;
                return_type = wrap_type(env, env->intern(env, prim_type), GG_PRIMITIVE_TAG);
            }
            if (is_array_type) {
                return_type = wrap_type(env, return_type, GG_ARRAY_TAG);
            }
            break;
        } else {
            prim_type[0] = sig[i];
            arg_types[arg_count] = wrap_type(env, env->intern(env, prim_type), GG_PRIMITIVE_TAG);
        }
        /* post-process the argument, if necessary */
        if (is_array_type) {
            arg_types[arg_count] = wrap_type(env, arg_types[arg_count], GG_ARRAY_TAG);
        }
        is_array_type = 0;
        arg_count++;
        assert(arg_count <= MAX_ARGS);
    }

    g_jvmtiError = (*g_jvmti)->GetMethodModifiers(g_jvmti, method, &modifiers);
    if (check_jvmti_error(env)) {
        free(arg_types);
        return NULL;
    }
    /* Access flags here: https://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html#jvms-4.6 */
    if (modifiers & JVM_ACC_PUBLIC) { modifiers_array[modifiers_count++] = env->intern(env, "public"); }
    if (modifiers & JVM_ACC_PRIVATE) { modifiers_array[modifiers_count++] = env->intern(env, "private"); }
    if (modifiers & JVM_ACC_PROTECTED) { modifiers_array[modifiers_count++] = env->intern(env, "protected"); }
    if (modifiers & JVM_ACC_STATIC) { modifiers_array[modifiers_count++] = env->intern(env, "static"); }
    if (modifiers & JVM_ACC_FINAL) { modifiers_array[modifiers_count++] = env->intern(env, "final"); }
    if (modifiers & JVM_ACC_SYNCHRONIZED) { modifiers_array[modifiers_count++] = env->intern(env, "synchronized"); }
    if (modifiers & JVM_ACC_BRIDGE) { modifiers_array[modifiers_count++] = env->intern(env, "bridge"); }
    if (modifiers & JVM_ACC_VARARGS) { modifiers_array[modifiers_count++] = env->intern(env, "varargs"); }
    if (modifiers & JVM_ACC_NATIVE) { modifiers_array[modifiers_count++] = env->intern(env, "native"); }
    if (modifiers & JVM_ACC_ABSTRACT) { modifiers_array[modifiers_count++] = env->intern(env, "abstract"); }
    if (modifiers & JVM_ACC_STRICT) { modifiers_array[modifiers_count++] = env->intern(env, "strictfp"); }
    if (modifiers & JVM_ACC_SYNTHETIC) { modifiers_array[modifiers_count++] = env->intern(env, "synthetic"); }
    modifiers_list = env->funcall(env, env->intern(env, "list"), modifiers_count, modifiers_array);

    list_args[0] = env->intern (env, ":name");
    list_args[1] = env->intern (env, name);
    list_args[2] = env->intern (env, ":returns");
    list_args[3] = return_type;
    list_args[4] = env->intern (env, ":accepts");
    list_args[5] = env->funcall(env, env->intern(env, "list"), arg_count, arg_types);
    free(arg_types);
    list_args[6] = env->intern (env, ":modifiers");
    list_args[7] = modifiers_list;

    method_struct = env->funcall(env, env->intern(env, "list"), methodID_to_struct_LIST_ARGS, list_args);
    (*g_jvmti)->Deallocate(g_jvmti, (void *) name);
    (*g_jvmti)->Deallocate(g_jvmti, (void *) sig);
    return method_struct;
}

/*
 * Generate the class structure for the class named by the given
 * symbol. c.f. "internals.org" file (and tests) for a description of
 * the structure
 */
#define Fgg_get_class_struct_LIST_ARGS 12
emacs_value
Fgg_get_class_struct (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    jclass class, superclass;
    static char class_name[MAX_CLASS_NAME_SIZE];
    ptrdiff_t size = MAX_CLASS_NAME_SIZE;
    jint modifiers;
    int modifiers_count = 0;
    emacs_value modifiers_array[8];
    emacs_value modifiers_list;
    bool ok;
    int i;
    jclass *interfaces;
    jmethodID *methods;
    jfieldID *fields;
    jint count;
    emacs_value superclass_sym;
    emacs_value list_args[10];
    emacs_value interfaces_list;
    emacs_value methods_list;
    emacs_value fields_list;
    /* args for calling (list) to construct interface/method/field lists */
    emacs_value *dynamic_args;

    if (!type_is(env, args[0], "symbol")) {
        return NULL;
    }

    ASSERT_JVM_RUNNING(env);

    /* Class name */
    ok = symbol_to_string(env, args[0], class_name, &size);
    assert(ok);

    class_name_to_internal(class_name);
    class = (*g_jni)->FindClass(g_jni, class_name);
    if (!class) {
        handle_exception(env);
        return NULL;
    }

    /* modifiers */
    g_jvmtiError = (*g_jvmti)->GetClassModifiers(g_jvmti, class, &modifiers);
    if (check_jvmti_error(env)) {
        (*g_jni)->DeleteLocalRef(g_jni, class);
        return NULL;
    }
    if (modifiers & JVM_ACC_PUBLIC) { modifiers_array[modifiers_count++] = env->intern(env, "public"); }
    if (modifiers & JVM_ACC_FINAL) { modifiers_array[modifiers_count++] = env->intern(env, "final"); }
    if (modifiers & JVM_ACC_SUPER) { modifiers_array[modifiers_count++] = env->intern(env, "super"); }
    if (modifiers & JVM_ACC_INTERFACE) { modifiers_array[modifiers_count++] = env->intern(env, "interface"); }
    if (modifiers & JVM_ACC_ABSTRACT) { modifiers_array[modifiers_count++] = env->intern(env, "abstract"); }
    if (modifiers & JVM_ACC_SYNTHETIC) { modifiers_array[modifiers_count++] = env->intern(env, "synthetic"); }
    if (modifiers & JVM_ACC_ANNOTATION) { modifiers_array[modifiers_count++] = env->intern(env, "annotation"); }
    if (modifiers & JVM_ACC_ENUM) { modifiers_array[modifiers_count++] = env->intern(env, "enum"); }
    modifiers_list = env->funcall(env, env->intern(env, "list"), modifiers_count, modifiers_array);

    /* Superclass */
    superclass = (*g_jni)->GetSuperclass(g_jni, class);
    if (superclass) {
        superclass_sym = jclass_to_symbol(env, superclass);
    } else {
        superclass_sym = env->intern (env, "nil");
    }

    /* Interfaces */
    g_jvmtiError = (*g_jvmti)->GetImplementedInterfaces(g_jvmti, class, &count, &interfaces);
    if (check_jvmti_error(env)) {
        (*g_jni)->DeleteLocalRef(g_jni, class);
        return NULL;
    }
    dynamic_args = malloc(sizeof(emacs_value) * count);
    assert(dynamic_args);
    for (i = 0; i < count; ++i) {
        dynamic_args[i] = jclass_to_symbol(env, interfaces[i]);
    }
    interfaces_list = env->funcall(env, env->intern(env, "list"), count, dynamic_args);
    (*g_jvmti)->Deallocate(g_jvmti, (void *) interfaces);
    free(dynamic_args);

    /* Methods */
    g_jvmtiError = (*g_jvmti)->GetClassMethods(g_jvmti, class, &count, &methods);
    if (check_jvmti_error(env)) {
        (*g_jni)->DeleteLocalRef(g_jni, class);
        return NULL;
    }
    dynamic_args = malloc(sizeof(emacs_value) * count);
    assert(dynamic_args);
    for (i = 0; i < count; ++i) {
        dynamic_args[i] = methodID_to_struct(env, methods[i]);
        if (!dynamic_args[i]) {
            (*g_jni)->DeleteLocalRef(g_jni, class);
            (*g_jvmti)->Deallocate(g_jvmti, (void *) methods);
            return NULL;
        }
    }
    methods_list = env->funcall(env, env->intern(env, "list"), count, dynamic_args);
    (*g_jvmti)->Deallocate(g_jvmti, (void *) methods);
    free(dynamic_args);

    /* Fields */
    g_jvmtiError = (*g_jvmti)->GetClassFields(g_jvmti, class, &count, &fields);
    if (check_jvmti_error(env)) {
        (*g_jni)->DeleteLocalRef(g_jni, class);
        return NULL;
    }
    dynamic_args = malloc(sizeof(emacs_value) * count);
    assert(dynamic_args);
    for (i = 0; i < count; ++i) {
        dynamic_args[i] = fieldID_to_struct(env, class, fields[i]);
        if (!dynamic_args[i]) {
            (*g_jni)->DeleteLocalRef(g_jni, class);
            (*g_jvmti)->Deallocate(g_jvmti, (void *) fields);
            return NULL;
        }
    }
    fields_list = env->funcall(env, env->intern(env, "list"), count, dynamic_args);
    (*g_jvmti)->Deallocate(g_jvmti, (void *) fields);
    free(dynamic_args);

    /* Create result structure */
    list_args[0] = env->intern (env, ":name");
    list_args[1] = args[0];
    list_args[2] = env->intern (env, ":superclass");
    list_args[3] = superclass_sym;
    list_args[4] = env->intern (env, ":interfaces");
    list_args[5] = interfaces_list;
    list_args[6] = env->intern (env, ":methods");
    list_args[7] = methods_list;
    list_args[8] = env->intern (env, ":fields");
    list_args[9] = fields_list;
    list_args[10] = env->intern(env, ":modifiers");
    list_args[11] = modifiers_list;

    (*g_jni)->DeleteLocalRef(g_jni, class);

    return env->funcall(env, env->intern(env, "list"), Fgg_get_class_struct_LIST_ARGS, list_args);
}

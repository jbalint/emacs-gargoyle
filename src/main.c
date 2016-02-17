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
#include <stdio.h>
#include <string.h>

#include <emacs-module.h>

#include "ctrl.h"

/* Emacs won't load the plugin without this: (error "Module /home/jbalint/sw/emacs-gargoyle/gargoyle.so is not GPL compatible") */
int plugin_is_GPL_compatible;

/*
 * Has the VM been started in the current process? We are not allowed
 * to "re-"start the JVM in the same process after it has been
 * stopped. c.f.:
 * 
 * - http://bugs.java.com/bugdatabase/view_bug.do?bug_id=4712793
 *   (JDK-4712793 : JNI : Failure in JNI_CreateJavaVM() after calling DestroyJavaVM())
 * 
 * - Looking in `jni.cpp' the "safe_to_recreate_vm" is never
 *   reset. Manually doing it in GDB causes errors during VM startup
 *   like "Invalid layout of java.lang.String at value" and then a
 *   crash:
 *
 * Stack: [0x00007fffffeff000,0x00007ffffffff000],  sp=0x00007fffffffcef0,  free space=1015k
 * Native frames: (J=compiled Java code, j=interpreted, Vv=VM code, C=native code)
 * V  [libjvm.so+0xa79e0a]  VMError::report_and_die()+0x2ca
 * V  [libjvm.so+0x4e5269]  report_fatal(char const*, int, char const*)+0x59
 * V  [libjvm.so+0x66ddbb]  compute_offset(int&, Klass*, Symbol*, Symbol*, bool)+0x20b
 * V  [libjvm.so+0x66eb59]  java_lang_String::compute_offsets()+0x39
 * V  [libjvm.so+0x9f64c6]  SystemDictionary::initialize_preloaded_classes(Thread*)+0x86
 * V  [libjvm.so+0xa44d6c]  Universe::genesis(Thread*)+0x3cc
 * V  [libjvm.so+0xa4501c]  universe2_init()+0x2c
 * V  [libjvm.so+0x61292d]  init_globals()+0xbd
 * V  [libjvm.so+0xa2bdbd]  Threads::create_vm(JavaVMInitArgs*, bool*)+0x24d
 * V  [libjvm.so+0x6a9b34]  JNI_CreateJavaVM+0x74
 * C  [gargoyle-dm.so+0x919]  ctrl_start_java+0x51
 * C  [gargoyle-dm.so+0xce1]  Fgg_java_start+0xc4
 * C  [emacs+0x181ecc]
 * C  0x0000000000000000
 */
int vm_started;

static emacs_value
Fgg_java_start (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    int ret;
    static char errmsg[50];
    if (g_vm) {
        sprintf(errmsg, "JVM already running");
        env->non_local_exit_signal(env, env->intern(env, "error"),
                                   env->make_string(env, errmsg, strlen(errmsg)));
    } else if (vm_started) {
        sprintf(errmsg, "JVM may not be restarted");
        env->non_local_exit_signal(env, env->intern(env, "error"),
                                   env->make_string(env, errmsg, strlen(errmsg)));
    }
    ret = ctrl_start_java(NULL);
    if (ret) {
        sprintf(errmsg, "JVM created failed (%d)", ret);
        env->non_local_exit_signal(env, env->intern(env, "error"),
                                   env->make_string(env, errmsg, strlen(errmsg)));
    }
    vm_started = 1;
    return env->intern (env, "t");
}

static emacs_value
Fgg_java_stop (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    ctrl_stop_java();
    return env->intern (env, "t");
}

static emacs_value
Fgg_java_running (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    return env->intern (env, g_vm ? "t" : "nil");
}

static emacs_value
Fgg_jni_version (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
	const char *version;
	if (!g_vm) {
		return env->intern (env, "nil");
	}
	version = ctrl_jni_version();
	if (version) {
		return env->make_string (env, version, strlen(version));
	} else {
		return env->intern (env, "nil");
	}
}

/* Provide FEATURE to Emacs.  */
static void
provide (emacs_env *env, const char *feature)
{
    emacs_value Qfeat = env->intern (env, feature);
    emacs_value Qprovide = env->intern (env, "provide");
    emacs_value args[] = { Qfeat };

    env->funcall (env, Qprovide, 1, args);
}

/* Bind NAME to FUN.  */
static void
bind_function (emacs_env *env, const char *name, emacs_value Sfun)
{
    emacs_value Qfset = env->intern (env, "fset");
    emacs_value Qsym = env->intern (env, name);
    emacs_value args[] = { Qsym, Sfun };

    env->funcall (env, Qfset, 2, args);
}

/*****************************/
/* Stuff to be moved out of here once organization is clearer */

#include <jni.h>

/*
 * Wrap a jobject pointer to a Lisp "Java object".
 */
emacs_value new_java_object(emacs_env *env, jobject o)
{
    emacs_value args[1];
    emacs_value wrapped;
    args[0] = env->make_user_ptr(env, NULL, o);
    wrapped = env->funcall(env, env->intern(env, "gg--new-object"), 1, args);
    /*
     * TODO  TODO  TODO  TODO  TODO  TODO  TODO 
     * Map the methods available on this object (and static methods if
     * it's a class) into the Lisp environment
     */
    assert(wrapped);
    return wrapped;
}

/*
 * Assert that the JVM is running. If it's not (error "Gargoyle JVM
 * not running") is thrown.
 */
void assert_jvm_running(emacs_env *env)
{
    static const char *err_msg = "Gargoyle JVM not running";
    emacs_value wrapped_err_msg;
    if (g_jni) {
        return;
    }
    wrapped_err_msg = env->make_string(env, err_msg, strlen(err_msg));
    assert(wrapped_err_msg);
    env->non_local_exit_signal(env, env->intern(env, "error"), wrapped_err_msg);
}

void handle_exception(emacs_env *env)
{
    jthrowable exception;
    exception = (*g_jni)->ExceptionOccurred(g_jni);
    if (exception) {
        if /* should I print exceptions? */ (1) {
            (*g_jni)->ExceptionDescribe(g_jni);
        }
        (*g_jni)->ExceptionClear(g_jni);
        env->non_local_exit_signal(env, env->intern(env, "java-exception"),
                                   new_java_object(env, exception));
    }
}

#define MAX_CLASS_NAME_SIZE 128
static emacs_value
Fgg_find_class (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    static char class_name[MAX_CLASS_NAME_SIZE];
    ptrdiff_t size;
    bool ok;
    jclass class;
    int i;
    /* Are we past the class when transforming class name string? If
     * so, use "$" instead of "/". Eg. com.x.Class.Other ->
     * com/x/Class$Other
     */
    int past_class = 0;

    assert_jvm_running(env);

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
        }
    }
    class = (*g_jni)->FindClass(g_jni, class_name);
    if (class) {
        return new_java_object(env, class);
    } else {
        /* this will longjmp */
        handle_exception(env);
        return NULL;
    }
}
/*****************************/

/* Module init function */
int
emacs_module_init(struct emacs_runtime *ert)
{
    emacs_env *env = ert->get_environment(ert);

    fprintf(stderr, "Initializing Gargoyle dynamic module\n");
    bind_function(env, "gg-java-start", env->make_function(env, 0, 0, Fgg_java_start, "Start the JVM", NULL));
    bind_function(env, "gg-java-stop", env->make_function(env, 0, 0, Fgg_java_stop, "Stop the JVM", NULL));
    bind_function(env, "gg-java-running", env->make_function(env, 0, 0, Fgg_java_running, "Is the JVM running?", NULL));
    bind_function(env, "gg-jni-version", env->make_function(env, 0, 0, Fgg_jni_version, "JNI version", NULL));

    bind_function(env, "gg-find-class", env->make_function(env, 1, 1, Fgg_find_class, "Find/load a Java class", NULL));

    provide(env, "gargoyle-dm");

    return 0;
}

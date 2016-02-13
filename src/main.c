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

#include <stdio.h>
#include <string.h>

#include <emacs-module.h>

#include "ctrl.h"

/* Emacs won't load the plugin without this: (error "Module /home/jbalint/sw/emacs-gargoyle/gargoyle.so is not GPL compatible") */
int plugin_is_GPL_compatible;

static emacs_value
Fgg_java_start (emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
    ctrl_start_java(NULL);
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

/* Module init function */
int
emacs_module_init(struct emacs_runtime *ert)
{
    emacs_env *env = ert->get_environment(ert);

    fprintf(stderr, "Initializing Gargoyle\n");
    bind_function(env, "gg-java-start", env->make_function(env, 0, 0, Fgg_java_start, "Start the JVM", NULL));
    bind_function(env, "gg-java-stop", env->make_function(env, 0, 0, Fgg_java_stop, "Stop the JVM", NULL));
    bind_function(env, "gg-java-running", env->make_function(env, 0, 0, Fgg_java_running, "Is the JVM running?", NULL));
    bind_function(env, "gg-jni-version", env->make_function(env, 0, 0, Fgg_jni_version, "JNI version", NULL));

    provide(env, "gargoyle");

    return 0;
}

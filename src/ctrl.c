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
#include <stdlib.h>

#include <jni.h>

/*
 * Global pointers to the vm under control (if this pointer is NULL, there is no running vm)
 * and
 */
JavaVM *g_vm;
/*
 * the JNI env for the Emacs thread
 */
JNIEnv *g_jni;

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
    return JNI_VERSION_1_8;
}

void JNI_OnUnload(JavaVM *vm, void *reserved)
{
}

int gg_vfprintf(FILE *stream, const char *format, va_list ap)
{
    return vfprintf(stderr, format, ap);
}

/*
 * Start the JVM.
 *
 * @return 0 for OK or a <0 JNI error code on failure
 */
int ctrl_start_java(char **err_msg)
{
    jint ret;
    JavaVMInitArgs vm_args;
    JavaVMOption options[] = {
        /* {.optionString = "-verbose:class"}, */
        /* {.optionString = "-verbose:jni"}, */
        {.optionString = "-Xcheck:jni"}
    };

    /* options[0].optionString = "-Xcheck:jni"; */
    /* options[1].optionString = "vfprintf"; */
    /* options[1].extraInfo = gg_vfprintf; */
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = 0;

    ret = JNI_CreateJavaVM(&g_vm, (void**) &g_jni, &vm_args);

    if (ret == JNI_OK) {
        ret = (*g_jni)->EnsureLocalCapacity(g_jni, 1000);
        assert((ret == JNI_OK) && "Workaround to have enough stack space until we manage this properly");
    }

    return ret;
}

int ctrl_stop_java()
{
    int ret;
    assert(g_vm);
    ret = (*g_vm)->DestroyJavaVM(g_vm);
    assert(ret == JNI_OK);
    g_vm = NULL;
    g_jni = NULL;
    return ret;
}

const char *ctrl_jni_version()
{
    jint version;
    assert(g_jni);
    version = (*g_jni)->GetVersion(g_jni);
    if (version < 0) {
        return NULL;
    }
    switch (version) {
    case JNI_VERSION_1_1:
        return "1.1";
    case JNI_VERSION_1_2:
        return "1.2";
    case JNI_VERSION_1_4:
        return "1.4";
    case JNI_VERSION_1_6:
        return "1.6";
    case JNI_VERSION_1_8:
        return "1.8";
    }
    return "unknown";
}

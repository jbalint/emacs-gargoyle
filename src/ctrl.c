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

#include <jni.h>

/*
 * Global pointers to the vm under control (if this pointer is NULL, there is no running vm)
 * and
 */
JavaVM *g_vm;
/*
 * the JNI env for the Emacs thread
 */
JNIEnv *g_jni_env;

jint JNI_OnLoad(JavaVM *vm, void *reserved)
{
	return JNI_VERSION_1_8;
}

void JNI_OnUnload(JavaVM *vm, void *reserved)
{
}

int ctrl_start_java(char **err_msg)
{
    JavaVMInitArgs vm_args;
    JavaVMOption options[1];
    options[0].optionString = "-Xcheck:jni";
    vm_args.version = JNI_VERSION_1_8;
    vm_args.nOptions = 1;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = 0;
    JNI_CreateJavaVM(&g_vm, (void**) &g_jni_env, &vm_args);
	return g_vm != NULL;
}

int ctrl_stop_java() {
    int ret = (*g_vm)->DestroyJavaVM(g_vm);
	g_vm = NULL;
	return ret;
}

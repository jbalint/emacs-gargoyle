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

#include <jni.h>
#include <jvmti.h>

/*
 * Global pointers to the vm under control (if this pointer is NULL, there is no running vm)
 * and
 */
extern JavaVM *g_vm;
/*
 * the JNI env for the Emacs thread
 */
extern JNIEnv *g_jni;
/*
 * the JVMTI env
 */
extern jvmtiEnv *g_jvmti;
/*
 * the JVMTI error
 */
extern jvmtiError g_jvmtiError;

/*
 * Some global stuff for convenience.
 */
extern jclass g_java_lang_Class;
extern jclass g_java_lang_String;

int ctrl_start_java(char **err_msg);

void ctrl_stop_java();

const char *ctrl_jni_version();

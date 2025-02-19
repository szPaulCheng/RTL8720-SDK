/**
 * copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BAIDU_DUER_LIGHTDUER_JNI_H_
#define BAIDU_DUER_LIGHTDUER_JNI_H_

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PACKAGE "com/baidu/lightduer/lib/jni"

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved);

JNIEnv *duer_jni_get_JNIEnv(int *neesDetach);

void duer_jni_detach_current_thread();

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIGHTDUER_JNI_H_

#
# Copyright (2017) Baidu Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := framework

LOCAL_STATIC_LIBRARIES := cjson mbedtls

MY_SRC_FILES := \
    $(wildcard $(LOCAL_PATH)/core/*.c) \
    $(wildcard $(LOCAL_PATH)/utils/*.c)

LOCAL_SRC_FILES := $(MY_SRC_FILES:$(LOCAL_PATH)/%=%)

#$(warning $(LOCAL_SRC_FILES))
LOCAL_CFLAGS := -DMBEDTLS_CONFIG_FILE=\"baidu_ca_mbedtls_config.h\"

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/core \
    $(LOCAL_PATH)/utils

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/include \
    $(LOCAL_PATH)/core \
    $(LOCAL_PATH)/utils

include $(BUILD_STATIC_LIBRARY)



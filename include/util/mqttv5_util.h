
/*
 * Copyright (c) 2020-2023 Huawei Cloud Computing Technology Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MQTTV5_UTIL_H
#define MQTTV5_UTIL_H

// #define MQTTV5  // If not defined, use MQTT3, else MQTT5.0

// ------------------------ Create Connection -------------------------------------
#define TOPIC_ALIAS_MAX 20 // MQTT v5 Maximum number of subject aliases

// ------------------------ Mqttv5 Publish ----------------------------------------
typedef struct {
    char *key;
    char *Value;
    void *nex;
} MQTTV5_USER_PRO; // user propeties

typedef struct {
    /*
     * It consists of a user-defined UTF-8 key/value pair array.
     * You can use user attributes to add metadata to mqtt
     * messages and transfer information between publishers,
     * mqtt servers and subscribers.
    */
    MQTTV5_USER_PRO *properties;
    /*
     * The content type contained in the mqtt5.0 variable header
     * is a UTF-8 encoded string, which is used to describe the
     * contents of a will message or a publish message.
     * A typical application of content type is to store MIME types,
     * such as text/plain for text files and audio/AAC for audio files.
    */
    char *contnt_type;
    /*
     * The responder will take appropriate actions according to the
     * request message, and then publish the response message to the
     * subject specified by the response subject attribute.
    */
    char *response_topic;
    /*
     * The request response of mqtt is asynchronous.
     * comparison_data can be used to correlate
     * the response message with the request message
    */
    char *correlation_data;
    /*
     * It allows users to reduce the bandwidth consumption when publishing 
     * messages by reducing long and commonly used topic names to a double byte integer.
     * 
    */
    unsigned int topic_alias;
} MQTTV5_DATA;

#ifdef MQTTV5
void mqttV5_listFree(MQTTV5_USER_PRO *mqttData);

#define mqttv5_initializer {NULL, NULL, NULL, NULL, -1}
#endif
#endif

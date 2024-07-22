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

#include <string.h>
#include "openssl/ossl_typ.h"
#include "openssl/buffer.h"
#include "openssl/hmac.h"
#include "log_util.h"
#include "securec.h"
#include "string_util.h"
#include "iota_error_type.h"
#include "iota_cfg.h"
#include "hmac_sha256.h"

int EncryptWithHMac(const char *inputData, char **inputKey, int inEncryDataLen, char *outData, int checkTimestamp)
{
    if ((inputData == NULL) || ((*inputKey) == NULL)) {
        PrintfLog(EN_LOG_LEVEL_ERROR, "EncryptWithHMac(): the input is invalid.\n");
        return IOTA_FAILURE;
    }
    char *end = NULL;
    unsigned int macLength = 0;
    unsigned int tryTime = 1;
    size_t lenData = strlen(inputData);
    long timeTmp = strtol(*inputKey, &end, 10);
    unsigned char *temp = NULL;
    if (checkTimestamp <= EN_IOTA_CFG_CHECK_STAMP_SHA256) {
        temp = HMAC(EVP_sha256(), *inputKey, strlen(*inputKey), (const unsigned char *)inputData, lenData,
            NULL, &macLength);
    } else {
        temp = HMAC(EVP_sm3(), *inputKey, TIME_STAMP_LENGTH, (const unsigned char *)inputData, lenData,
            NULL, &macLength);
    }

    while (strlen(temp) != SHA256_ENCRYPTION_LENGRH) {
        tryTime++;
        if (tryTime > TRY_MAX_TIME) {
            PrintfLog(EN_LOG_LEVEL_ERROR, "EncryptWithHMac(): Encryption failed after max times attempts.\n");
            return IOTA_FAILURE;
        }

        timeTmp++;
        (void)snprintf_s(*inputKey, TIME_STAMP_LENGTH + 1, TIME_STAMP_LENGTH, "%ld", timeTmp);
        if (checkTimestamp <= EN_IOTA_CFG_CHECK_STAMP_SHA256) {
            temp = HMAC(EVP_sha256(), *inputKey, TIME_STAMP_LENGTH, (const unsigned char *)inputData, lenData,
                NULL, &macLength);
        } else {
            temp = HMAC(EVP_sm3(), *inputKey, TIME_STAMP_LENGTH, (const unsigned char *)inputData, lenData,
                NULL, &macLength);
        }
    }

    int index;
    int loop;
    char hexChar;

    for (index = 0, loop = 0; loop < inEncryDataLen; loop++) {
        hexChar = (temp[loop] >> 4) & 0x0F;
        outData[index++] = (hexChar <= 9) ? (hexChar + '0') : (hexChar + 'a' - 10);

        hexChar = temp[loop] & 0x0F;
        outData[index++] = (hexChar <= 9) ? (hexChar + '0') : (hexChar + 'a' - 10);
    }

    outData[index] = '\0';

    return IOTA_SUCCESS;
}

unsigned char *base64_encode(unsigned char *str)  
{  
    long len;  
    long str_len;  
    unsigned char *res;  
    int i,j;  
    //定义base64编码表  
    unsigned char *base64_table="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";  
  
    //计算经过base64编码后的字符串长度  
    str_len=strlen(str);  
    if(str_len % 3 == 0)  
        len=str_len/3*4;  
    else  
        len=(str_len/3+1)*4;  
  
    res=malloc(sizeof(unsigned char)*len+1);  
    res[len]='\0';  
  
    //以3个8位字符为一组进行编码  
    for(i=0,j=0;i<len-2;j+=3,i+=4)  
    {  
        res[i]=base64_table[str[j]>>2]; //取出第一个字符的前6位并找出对应的结果字符  
        res[i+1]=base64_table[(str[j]&0x3)<<4 | (str[j+1]>>4)]; //将第一个字符的后位与第二个字符的前4位进行组合并找到对应的结果字符  
        res[i+2]=base64_table[(str[j+1]&0xf)<<2 | (str[j+2]>>6)]; //将第二个字符的后4位与第三个字符的前2位组合并找出对应的结果字符  
        res[i+3]=base64_table[str[j+2]&0x3f]; //取出第三个字符的后6位并找出结果字符  
    }  

    switch(str_len % 3)  
    {  
        case 1:  
            if (i >= 2) {
                res[i-2]='=';  
                res[i-1]='=';  
            }
            break;  
        case 2:  
            if (i >= 1) {
                res[i-1]='=';  
            }
            break;  
        default:
            break;
    }  
  
    return res;  
}  
  
unsigned char *base64_decode(unsigned char *code)  
{  
    //根据base64表，以字符找到对应的十进制数据  
    int table[]={0,0,0,0,0,0,0,0,0,0,0,0,
    		 0,0,0,0,0,0,0,0,0,0,0,0,
    		 0,0,0,0,0,0,0,0,0,0,0,0,
    		 0,0,0,0,0,0,0,62,0,0,0,
    		 63,52,53,54,55,56,57,58,
    		 59,60,61,0,0,0,0,0,0,0,0,
    		 1,2,3,4,5,6,7,8,9,10,11,12,
    		 13,14,15,16,17,18,19,20,21,
    		 22,23,24,25,0,0,0,0,0,0,26,
    		 27,28,29,30,31,32,33,34,35,
    		 36,37,38,39,40,41,42,43,44,
    		 45,46,47,48,49,50,51
    	       };  
    long len;  
    long str_len;  
    unsigned char *res;  
    int i,j;  
  
    //计算解码后的字符串长度  
    len=strlen(code);  
    //判断编码后的字符串后是否有=  
    if(strstr(code,"=="))  
        str_len=len/4*3-2;  
    else if(strstr(code,"="))  
        str_len=len/4*3-1;  
    else  
        str_len=len/4*3;  
  
    res=malloc(sizeof(unsigned char)*str_len+1);  
    res[str_len]='\0';  
  
    //以4个字符为一位进行解码  
    for(i=0,j=0;i < len-2;j+=3,i+=4)  
    {  
        res[j]=((unsigned char)table[code[i]])<<2 | (((unsigned char)table[code[i+1]])>>4); //取出第一个字符对应base64表的十进制数的前6位与第二个字符对应base64表的十进制数的后2位进行组合  
        res[j+1]=(((unsigned char)table[code[i+1]])<<4) | (((unsigned char)table[code[i+2]])>>2); //取出第二个字符对应base64表的十进制数的后4位与第三个字符对应bas464表的十进制数的后4位进行组合  
        res[j+2]=(((unsigned char)table[code[i+2]])<<6) | ((unsigned char)table[code[i+3]]); //取出第三个字符对应base64表的十进制数的后2位与第4个字符进行组合  
    }  
    return res;  
}
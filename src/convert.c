/*************************************************************************
    > File Name: convert.c
    > Author: Mr Wei
    > Description:
    > Created Time: 2022-08-17
 ************************************************************************/
#include "convert.h"
#include "Log_Message.h"

static int charset_convert(const char *from_charset, const char *to_charset,
                           char *in_buf, size_t in_left, char *out_buf, size_t out_left)
{
    iconv_t icd = (iconv_t)-1;
    size_t sRet = -1;
    char *pIn = in_buf;
    char *pOut = out_buf;
    size_t outLen = out_left;

    if (NULL == from_charset || NULL == to_charset || NULL == in_buf || 0 >= in_left || NULL == out_buf || 0 >= out_left) {
        return -1;
    }

    icd = iconv_open(to_charset, from_charset);
    if ((iconv_t)-1 == icd) {
        log_message(ERROR, "Open encoding format error!");
        return -1;
    }

    sRet = iconv(icd, &pIn, &in_left, &pOut, &out_left);
    if ((size_t)-1 == sRet) {
        log_message(ERROR, "Encoding conversion failed!");
        iconv_close(icd);
        return -1;
    }

    out_buf[outLen - out_left] = 0;
    iconv_close(icd);
    return (int)(outLen - out_left);
}

int charset_convert_UTF8_TO_GB2312(char *in_buf, size_t in_left, char *out_buf, size_t out_left)
{
    return charset_convert("UTF-8", "GB2312", in_buf, in_left, out_buf, out_left);
}

int charset_convert_GB2312_TO_UTF8(char *in_buf, size_t in_left, char *out_buf, size_t out_left)
{
    return charset_convert("GB2312", "UTF-8", in_buf, in_left, out_buf, out_left);
}

int charset_convert_UTF8_TO_UTF16(char *in_buf, size_t in_left, char *out_buf, size_t out_left)
{
    return charset_convert("UTF-8", "UTF-16LE", in_buf, in_left, out_buf, out_left);
}

int charset_convert_GBK_TO_UTF16_LE(char *in_buf, size_t in_left, char *out_buf, size_t out_left)
{
    return charset_convert("GBK", "UTF-16LE", in_buf, in_left, out_buf, out_left);
}

int charset_convert_GBK_TO_UTF8(char *in_buf, size_t in_left, char *out_buf, size_t out_left)
{
    return charset_convert("GBK", "UTF-8", in_buf, in_left, out_buf, out_left);
}

int charset_convert_UTF8_TO_GBK(char *in_buf, size_t in_left, char *out_buf, size_t out_left)
{
    return charset_convert("UTF-8", "GBK", in_buf, in_left, out_buf, out_left);
}

int convert_test(void)
{
    const char *pIn = "hello 你好世界";
    char pOut[100] = {0};
    int outLen = 0;
    int inLen = 0;
    int iRet = -1;

    inLen = strlen(pIn);
    outLen = inLen * 10;

    iRet = charset_convert_GBK_TO_UTF8((char *)pIn, inLen, pOut, outLen);
    if (-1 == iRet) {
        return -1;
    }

    printf("out = %s;\touLen = %d\n", pOut, iRet);

    return 0;
}

static const char *base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* base64_encode(char* binData, char* base64, int binLength)
{
    int i = 0;
    int j = 0;
    int current = 0;
    for (i = 0; i < binLength; i += 3) {
        //获取第一个6位
        current = (*(binData + i) >> 2) & 0x3F;
        *(base64 + j++) = base64char[current];
        //获取第二个6位的前两位
        current = (*(binData + i) << 4) & 0x30;
        //如果只有一个字符，那么需要做特殊处理
        if (binLength <= (i + 1)) {
            *(base64 + j++) = base64char[current];
            *(base64 + j++) = '=';
            *(base64 + j++) = '=';
            break;
        }
        //获取第二个6位的后四位
        current |= (*(binData + i + 1) >> 4) & 0xf;
        *(base64 + j++) = base64char[current];
        //获取第三个6位的前四位
        current = (*(binData + i + 1) << 2) & 0x3c;
        if (binLength <= (i + 2)) {
            *(base64 + j++) = base64char[current];
            *(base64 + j++) = '=';
            break;
        }
        //获取第三个6位的后两位
        current |= (*(binData + i + 2) >> 6) & 0x03;
        *(base64 + j++) = base64char[current];
        //获取第四个6位
        current = *(binData + i + 2) & 0x3F;
        *(base64 + j++) = base64char[current];
    }
    *(base64 + j) = '\0';
    return base64;
}

char* base64_decode(char const* base64Str, char* debase64Str, int encodeStrLen)
{
    int i = 0;
    int j = 0;
    int k = 0;
    char temp[4] = "";

    for (i = 0; i < encodeStrLen; i += 4) {
        for (j = 0; j < 64; j++) {
            if (*(base64Str + i) == base64char[j]) {
                temp[0] = j;
            }
        }

        for (j = 0; j < 64; j++) {
            if (*(base64Str + i + 1) == base64char[j]) {
                temp[1] = j;
            }
        }

        for (j = 0; j < 64; j++) {
            if (*(base64Str + i + 2) == base64char[j]) {
                temp[2] = j;
            }
        }

        for (j = 0; j < 64; j++) {
            if (*(base64Str + i + 3) == base64char[j]) {
                temp[3] = j;
            }
        }

        *(debase64Str + k++) = ((temp[0] << 2) & 0xFC) | ((temp[1] >> 4) & 0x03);
        if (*(base64Str + i + 2) == '=')
            break;

        *(debase64Str + k++) = ((temp[1] << 4) & 0xF0) | ((temp[2] >> 2) & 0x0F);
        if (*(base64Str + i + 3) == '=')
            break;

        *(debase64Str + k++) = ((temp[2] << 6) & 0xF0) | (temp[3] & 0x3F);
    }
    return debase64Str;
}

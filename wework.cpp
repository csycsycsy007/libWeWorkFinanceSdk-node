#include <node.h>
#include <v8.h>
#include "WeWorkFinanceSdk_C.h"
#include <iostream>

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using std::string;

using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Object;
using v8::String;
using v8::Value;
using v8::Int32;

typedef WeWorkFinanceSdk_t * newsdk_t();
typedef int Init_t(WeWorkFinanceSdk_t * ,const char * ,const char * );
typedef void DestroySdk_t(WeWorkFinanceSdk_t * );

typedef int GetChatData_t(WeWorkFinanceSdk_t*, unsigned long long, unsigned int, const char*, const char*, int, Slice_t*);
typedef Slice_t* NewSlice_t();
typedef void FreeSlice_t(Slice_t*);

typedef int DecryptData_t(const char*, const char*, Slice_t*);

typedef int GetMediaData_t(WeWorkFinanceSdk_t*, const char*, const char*, const char*, const char*, int, MediaData_t*);
typedef MediaData_t* NewMediaData_t();
typedef void FreeMediaData_t(MediaData_t*);

WeWorkFinanceSdk_t * sdk;
void * so_handle;

void Init(const v8::FunctionCallbackInfo<v8::Value>& args) {

    int ret = 0;

    Isolate *isolate = args.GetIsolate();
    String::Utf8Value str0(isolate, args[0]);
    String::Utf8Value str1(isolate, args[1]);

    const char* arg0 = *str0;
    const char* arg1 = *str1;

    so_handle = dlopen("./lib/libWeWorkFinanceSdk_C.so", RTLD_LAZY);
    if (!so_handle) {
        printf("load sdk so fail:%s\n", dlerror());
    }
    newsdk_t * newsdk_fn = (newsdk_t *) dlsym(so_handle, "NewSdk");
    sdk = newsdk_fn();

    Init_t * init_fn = (Init_t * ) dlsym(so_handle, "Init");
    DestroySdk_t * destroysdk_fn = (DestroySdk_t * ) dlsym(so_handle, "DestroySdk");
    ret = init_fn(sdk, arg0, arg1);
    if (ret == 0) {
        args.GetReturnValue().Set(ret);
    } else {
        //sdk需要主动释放
        destroysdk_fn(sdk);
        args.GetReturnValue().Set(ret);
        printf("init sdk err ret:%d\n", ret);
    }
}

void GetChatData(const v8::FunctionCallbackInfo<v8::Value>& args) {
    int ret = 0;

    Isolate *isolate = args.GetIsolate();

    unsigned long long seq = args[0].As<Int32>()->Value();
    int limit = args[1].As<Int32>()->Value();
    int timeout = args[4].As<Int32>()->Value();

    String::Utf8Value str2(isolate, args[2]);
    String::Utf8Value str3(isolate, args[3]);
    const char* proxy = *str2;
    const char* passwd = *str3;

    NewSlice_t * newslice_fn = (NewSlice_t * ) dlsym(so_handle, "NewSlice");
    FreeSlice_t * freeslice_fn = (FreeSlice_t * ) dlsym(so_handle, "FreeSlice");

    // 每次使用GetChatData拉取存档前需要调用NewSlice获取一个chatDatas，在使用完chatDatas中数据后，还需要调用FreeSlice释放。
    Slice_t * chatDatas = newslice_fn();
    GetChatData_t * getchatdata_fn = (GetChatData_t * ) dlsym(so_handle, "GetChatData");
    ret = getchatdata_fn(sdk, seq, limit, proxy, passwd, timeout, chatDatas);
    if (ret != 0) {
        freeslice_fn(chatDatas);
    }
    
    if (ret == 0) {
        Local<String> result = String::NewFromUtf8(isolate, chatDatas->buf).ToLocalChecked();

        args.GetReturnValue().Set(result);
    } else {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Failed to get chat data.").ToLocalChecked());
    }

    freeslice_fn(chatDatas);
}


void Decryptdata(const v8::FunctionCallbackInfo<v8::Value>& args) {
    int ret = 0;

    Isolate *isolate = args.GetIsolate();
    String::Utf8Value str(isolate, args[0]);
    String::Utf8Value str1(isolate, args[1]);

    const char* encrypt_key = *str;
    const char* encrypt_msg = *str1;

    NewSlice_t* newslice_fn = (NewSlice_t*)dlsym(so_handle, "NewSlice");
    FreeSlice_t* freeslice_fn = (FreeSlice_t*)dlsym(so_handle, "FreeSlice");

    Slice_t* Msgs = newslice_fn();
    // decryptdata api
    DecryptData_t* decryptdata_fn = (DecryptData_t*)dlsym(so_handle, "DecryptData");
    ret = decryptdata_fn(encrypt_key, encrypt_msg, Msgs);

    if (ret == 0) {
        Local<String> result = String::NewFromUtf8(isolate, Msgs->buf).ToLocalChecked();

        args.GetReturnValue().Set(result);
    } else {
        args.GetReturnValue().Set(String::NewFromUtf8(isolate, "Failed to decrypt data.").ToLocalChecked());
    }

    freeslice_fn(Msgs);
}


void GetMediaData(const v8::FunctionCallbackInfo<v8::Value>& args) {

    int ret = 0;

    Isolate *isolate = args.GetIsolate();

    String::Utf8Value str0(isolate, args[0]);
    String::Utf8Value str1(isolate, args[1]);
    String::Utf8Value str2(isolate, args[2]);
    String::Utf8Value str4(isolate, args[4]);
    const char* sdkFileid = *str0;
    const char* proxy = *str1;
    const char* passwd = *str2;
    const char* saveFile = *str4;

    int timeout = args[3].As<Int32>()->Value();

    //拉取媒体文件
    std::string index;
    int isfinish = 0;

    GetMediaData_t* getmediadata_fn = (GetMediaData_t*)dlsym(so_handle, "GetMediaData");
    NewMediaData_t* newmediadata_fn = (NewMediaData_t*)dlsym(so_handle, "NewMediaData");
    FreeMediaData_t* freemediadata_fn = (FreeMediaData_t*)dlsym(so_handle, "FreeMediaData");

    //媒体文件每次拉取的最大size为512k，因此超过512k的文件需要分片拉取。若该文件未拉取完整，mediaData中的is_finish会返回0，同时mediaData中的outindexbuf会返回下次拉取需要传入GetMediaData的indexbuf。
    //indexbuf一般格式如右侧所示，”Range:bytes=524288-1048575“，表示这次拉取的是从524288到1048575的分片。单个文件首次拉取填写的indexbuf为空字符串，拉取后续分片时直接填入上次返回的indexbuf即可。
    while (isfinish == 0) {
        //每次使用GetMediaData拉取存档前需要调用NewMediaData获取一个mediaData，在使用完mediaData中数据后，还需要调用FreeMediaData释放。
        printf("index:%s\n", index.c_str());
        MediaData_t* mediaData = newmediadata_fn();
        ret = getmediadata_fn(sdk, index.c_str(), sdkFileid, proxy, passwd, timeout, mediaData);
        if (ret != 0) {
            //单个分片拉取失败建议重试拉取该分片，避免从头开始拉取。
            freemediadata_fn(mediaData);
            printf("GetMediaData err ret:%d\n", ret);
            args.GetReturnValue().Set(ret);
        }
        printf("content size:%d isfin:%d outindex:%s\n", mediaData->data_len, mediaData->is_finish, mediaData->outindexbuf);

        //大于512k的文件会分片拉取，此处需要使用追加写，避免后面的分片覆盖之前的数据。
        char file[200];
        snprintf(file, sizeof(file), "%s", saveFile);
        FILE* fp = fopen(file, "ab+");
        printf("filename:%s \n", file);
        if (NULL == fp) {
            freemediadata_fn(mediaData);
            printf("open file err\n");
            args.GetReturnValue().Set(ret);
        }

        fwrite(mediaData->data, mediaData->data_len, 1, fp);
        fclose(fp);

        //获取下次拉取需要使用的indexbuf
        index.assign(string(mediaData->outindexbuf));
        isfinish = mediaData->is_finish;
        freemediadata_fn(mediaData);
    }

    args.GetReturnValue().Set(ret);
}


void init(Local<Object> exports) {
  NODE_SET_METHOD(exports, "init", Init);
  NODE_SET_METHOD(exports, "getChatData", GetChatData);
  NODE_SET_METHOD(exports, "decryptdata", Decryptdata);
  NODE_SET_METHOD(exports, "getMediaData", GetMediaData);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, init)

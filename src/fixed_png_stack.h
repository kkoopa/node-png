#ifndef FIXED_PNG_STACK_H
#define FIXED_PNG_STACK_H

#include <node.h>
#include <node_buffer.h>

#include "common.h"

class FixedPngStack : public node::ObjectWrap {
    int width, height;
    unsigned char *data;
    buffer_type buf_type;

    static void UV_PngEncode(uv_work_t *req);
    static void UV_PngEncodeAfter(uv_work_t *req);

public:
    static void Initialize(v8::Handle<v8::Object> target);
    FixedPngStack(int wwidth, int hheight, buffer_type bbuf_type);
    ~FixedPngStack();

    class FixedPngEncodeWorker : public PngEncoder::EncodeWorker {
    public:
        FixedPngEncodeWorker(NanCallback *callback, FixedPngStack *png) : PngEncoder::EncodeWorker(callback), png_obj(png) {
        };

        void Execute();
        void HandleOKCallback();
        void HandleErrorCallback();

    private:
        FixedPngStack *png_obj;
    };

    void Push(unsigned char *buf_data, int x, int y, int w, int h);
    v8::Handle<v8::Value> PngEncodeSync();

    static NAN_METHOD(New);
    static NAN_METHOD(Push);
    static NAN_METHOD(PngEncodeSync);
    static NAN_METHOD(PngEncodeAsync);
};
#endif


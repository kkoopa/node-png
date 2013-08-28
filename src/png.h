#ifndef NODE_PNG_H
#define NODE_PNG_H

#include <node.h>
#include <node_buffer.h>

#include "common.h"

#include "png_encoder.h"

class Png : public node::ObjectWrap {
    int width;
    int height;
    buffer_type buf_type;

public:
    static void Initialize(v8::Handle<v8::Object> target);
    Png(int wwidth, int hheight, buffer_type bbuf_type);
    v8::Handle<v8::Value> PngEncodeSync();

    class PngEncodeWorker : public PngEncoder::EncodeWorker {
    public:
        PngEncodeWorker(NanCallback *callback, Png *png, char *buf_data) : PngEncoder::EncodeWorker(callback, buf_data), png_obj(png) {
        };

        void Execute();
        void HandleOKCallback();
        void HandleErrorCallback();

    private:
        Png *png_obj;
    };

    static NAN_METHOD(New);
    static NAN_METHOD(PngEncodeSync);
    static NAN_METHOD(PngEncodeAsync);
};

#endif


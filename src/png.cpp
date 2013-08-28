#include <cstring>
#include <cstdlib>
#include "common.h"
#include "png_encoder.h"
#include "png.h"

using namespace v8;
using namespace node;

void
Png::Initialize(Handle<Object> target)
{
    NanScope();

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", PngEncodeSync);
    target->Set(String::NewSymbol("Png"), t->GetFunction());
}

Png::Png(int wwidth, int hheight, buffer_type bbuf_type) :
    width(wwidth), height(hheight), buf_type(bbuf_type) {}

Handle<Value>
Png::PngEncodeSync()
{
    NanScope();

    Local<Value> buf_val = NanObjectWrapHandle(this)->GetHiddenValue(String::New("buffer"));

    char *buf_data = Buffer::Data(buf_val->ToObject());

    try {
        PngEncoder encoder((unsigned char*)buf_data, width, height, buf_type);
        encoder.encode();
        int png_len = encoder.get_png_len();
        Local<Object> retbuf = NanNewBufferHandle(png_len);
        memcpy(Buffer::Data(retbuf), encoder.get_png(), png_len);
        return scope.Close(retbuf);
    }
    catch (const char *err) {
        return ThrowException(Exception::Error(String::New(err)));
    }
}

NAN_METHOD(Png::New)
{
    NanScope();

    if (args.Length() < 3)
        return NanThrowError("At least three arguments required - data buffer, width, height, [and input buffer type]");
    if (!Buffer::HasInstance(args[0]))
        return NanThrowTypeError("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return NanThrowTypeError("Second argument must be integer width.");
    if (!args[2]->IsInt32())
        return NanThrowTypeError("Third argument must be integer height.");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 4) {
        if (!args[3]->IsString())
            return NanThrowTypeError("Fourth argument must be 'gray', 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[3]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
            str_eq(*bts, "rgba") || str_eq(*bts, "bgra") ||
            str_eq(*bts, "gray")))
        {
            return NanThrowTypeError("Fourth argument must be 'gray', 'rgb', 'bgr', 'rgba' or 'bgra'.");
        }

        if (str_eq(*bts, "rgb"))
            buf_type = BUF_RGB;
        else if (str_eq(*bts, "bgr"))
            buf_type = BUF_BGR;
        else if (str_eq(*bts, "rgba"))
            buf_type = BUF_RGBA;
        else if (str_eq(*bts, "bgra"))
            buf_type = BUF_BGRA;
        else if (str_eq(*bts, "gray"))
            buf_type = BUF_GRAY;
        else
            return NanThrowTypeError("Fourth argument wasn't 'gray', 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }

    int w = args[1]->Int32Value();
    int h = args[2]->Int32Value();

    if (w < 0)
        return NanThrowRangeError("Width smaller than 0.");
    if (h < 0)
        return NanThrowRangeError("Height smaller than 0.");

    Png *png = new Png(w, h, buf_type);
    png->Wrap(args.This());

    // Save buffer.
    NanObjectWrapHandle(png)->SetHiddenValue(String::New("buffer"), args[0]);

    NanReturnValue(args.This());
}

NAN_METHOD(Png::PngEncodeSync)
{
    NanScope();
    Png *png = ObjectWrap::Unwrap<Png>(args.This());
    NanReturnValue(png->PngEncodeSync());
}

void Png::PngEncodeWorker::Execute() {
    try {
        PngEncoder encoder((unsigned char *)buf_data, png_obj->width, png_obj->height, png_obj->buf_type);
        encoder.encode();
        png_len = encoder.get_png_len();
        png = (char *)malloc(sizeof(*png)*png_len);
        if (!png) {
            errmsg = strdup("malloc in Png::UV_PngEncode failed.");
        } else {
            memcpy(png, encoder.get_png(), png_len);
        }
    }
    catch (const char *err) {
        errmsg = strdup(err);
    }
}

void Png::PngEncodeWorker::HandleOKCallback() {
    NanScope();

    Local<Object> buf = NanNewBufferHandle(png_len);
    memcpy(Buffer::Data(buf), png, png_len);
    Local<Value> argv[2] = {buf, Undefined()};

    TryCatch try_catch; // don't quite see the necessity of this

    callback->Call(2, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    free(png);
    png = NULL;

    png_obj->Unref();
}

void Png::PngEncodeWorker::HandleErrorCallback() {
    NanScope();

    Local<Value> argv[2] = {Undefined(), Exception::Error(String::New(errmsg))};

    TryCatch try_catch; // don't quite see the necessity of this

    callback->Call(2, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    if (png) {
        free(png);
        png = NULL;
    }

    png_obj->Unref();
}

NAN_METHOD(Png::PngEncodeAsync)
{
    NanScope();

    if (args.Length() != 1)
        return NanThrowError("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return NanThrowTypeError("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    Png *png = ObjectWrap::Unwrap<Png>(args.This());

    // We need to pull out the buffer data before
    // we go to the thread pool.
    Local<Value> buf_val = NanObjectWrapHandle(png)->GetHiddenValue(String::New("buffer"));

    NanAsyncQueueWorker(new Png::PngEncodeWorker(new NanCallback(callback), png, Buffer::Data(buf_val->ToObject())));

    png->Ref();

    NanReturnUndefined();
}

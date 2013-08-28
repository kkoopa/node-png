#include <cstdlib>

#include "png_encoder.h"
#include "fixed_png_stack.h"

using namespace v8;
using namespace node;

void
FixedPngStack::Initialize(Handle<Object> target)
{
    NanScope();

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", PngEncodeSync);
    target->Set(String::NewSymbol("FixedPngStack"), t->GetFunction());
}

FixedPngStack::FixedPngStack(int wwidth, int hheight, buffer_type bbuf_type) :
    width(wwidth), height(hheight), buf_type(bbuf_type)
{
    data = (unsigned char *)malloc(sizeof(*data) * width * height * 4);
    if (!data) throw "malloc failed in node-png (FixedPngStack ctor)";
    memset(data, 0xFF, width*height*4);
}

FixedPngStack::~FixedPngStack()
{
    free(data);
}

void
FixedPngStack::Push(unsigned char *buf_data, int x, int y, int w, int h)
{
    int start = y*width*4 + x*4;
    for (int i = 0; i < h; i++) {
        unsigned char *datap = &data[start + i*width*4];
        for (int j = 0; j < w; j++) {
            *datap++ = *buf_data++;
            *datap++ = *buf_data++;
            *datap++ = *buf_data++;
            *datap++ = (buf_type == BUF_RGB || buf_type == BUF_BGR) ? 0x00 : *buf_data++;
        }
    }
}

Handle<Value>
FixedPngStack::PngEncodeSync()
{
    NanScope();

    buffer_type pbt = (buf_type == BUF_BGR || buf_type == BUF_BGRA) ? BUF_BGRA : BUF_RGBA;

    try {
        PngEncoder encoder(data, width, height, pbt);
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

NAN_METHOD(FixedPngStack::New)
{
    NanScope();

    if (args.Length() < 2)
        return NanThrowError("At least two arguments required - width and height [and input buffer type].");
    if (!args[0]->IsInt32())
        return NanThrowTypeError("First argument must be integer width.");
    if (!args[1]->IsInt32())
        return NanThrowTypeError("Second argument must be integer height.");

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 3) {
        if (!args[2]->IsString())
            return NanThrowTypeError("Third argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[2]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
                    str_eq(*bts, "rgba") || str_eq(*bts, "bgra")))
        {
            return NanThrowTypeError("Third argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
        }

        if (str_eq(*bts, "rgb"))
            buf_type = BUF_RGB;
        else if (str_eq(*bts, "bgr"))
            buf_type = BUF_BGR;
        else if (str_eq(*bts, "rgba"))
            buf_type = BUF_RGBA;
        else if (str_eq(*bts, "bgra"))
            buf_type = BUF_BGRA;
        else
            return NanThrowTypeError("Third argument wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }

    int width = args[0]->Int32Value();
    int height = args[1]->Int32Value();

    try {
        FixedPngStack *png_stack = new FixedPngStack(width, height, buf_type);
        png_stack->Wrap(args.This());
        NanReturnValue(args.This());
    }
    catch (const char *e) {
        return NanThrowError(e);
    }
}

NAN_METHOD(FixedPngStack::Push)
{
    NanScope();

    if (!Buffer::HasInstance(args[0]))
        return NanThrowTypeError("First argument must be Buffer.");
    if (!args[1]->IsInt32())
        return NanThrowTypeError("Second argument must be integer x.");
    if (!args[2]->IsInt32())
        return NanThrowTypeError("Third argument must be integer y.");
    if (!args[3]->IsInt32())
        return NanThrowTypeError("Fourth argument must be integer w.");
    if (!args[4]->IsInt32())
        return NanThrowTypeError("Fifth argument must be integer h.");

    FixedPngStack *png_stack = ObjectWrap::Unwrap<FixedPngStack>(args.This());
    int x = args[1]->Int32Value();
    int y = args[2]->Int32Value();
    int w = args[3]->Int32Value();
    int h = args[4]->Int32Value();

    if (x < 0)
        return NanThrowRangeError("Coordinate x smaller than 0.");
    if (y < 0)
        return NanThrowRangeError("Coordinate y smaller than 0.");
    if (w < 0)
        return NanThrowRangeError("Width smaller than 0.");
    if (h < 0)
        return NanThrowRangeError("Height smaller than 0.");
    if (x >= png_stack->width)
        return NanThrowRangeError("Coordinate x exceeds FixedPngStack's dimensions.");
    if (y >= png_stack->height)
        return NanThrowRangeError("Coordinate y exceeds FixedPngStack's dimensions.");
    if (x+w > png_stack->width)
        return NanThrowRangeError("Pushed PNG exceeds FixedPngStack's width.");
    if (y+h > png_stack->height)
        return NanThrowRangeError("Pushed PNG exceeds FixedPngStack's height.");

    char *buf_data = Buffer::Data(args[0]->ToObject());

    png_stack->Push((unsigned char*)buf_data, x, y, w, h);

    NanReturnUndefined();
}

NAN_METHOD(FixedPngStack::PngEncodeSync)
{
    NanScope();

    FixedPngStack *png_stack = ObjectWrap::Unwrap<FixedPngStack>(args.This());
    NanReturnValue(png_stack->PngEncodeSync());
}

void FixedPngStack::FixedPngEncodeWorker::Execute() {
    try {
        PngEncoder encoder(png_obj->data, png_obj->width, png_obj->height, png_obj->buf_type);
        encoder.encode();
        png_len =encoder.get_png_len();
        png = (char *)malloc(sizeof(*png)*png_len);
        if (!png) {
            errmsg = strdup("malloc in FixedPngStack::UV_PngEncode failed.");
        }
        else {
            memcpy(png,encoder.get_png(), png_len);
        }
    }
    catch (const char *err) {
        errmsg = strdup(err);
    }
}

void FixedPngStack::FixedPngEncodeWorker::HandleOKCallback() {
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

void FixedPngStack::FixedPngEncodeWorker::HandleErrorCallback() {
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

NAN_METHOD(FixedPngStack::PngEncodeAsync)
{
    NanScope();

    if (args.Length() != 1)
        return NanThrowError("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return NanThrowTypeError("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    FixedPngStack *png = ObjectWrap::Unwrap<FixedPngStack>(args.This());

    NanAsyncQueueWorker(new FixedPngStack::FixedPngEncodeWorker(new NanCallback(callback), png));

    png->Ref();

    NanReturnUndefined();
}


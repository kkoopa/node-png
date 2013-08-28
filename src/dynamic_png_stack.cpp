#include "png_encoder.h"
#include "dynamic_png_stack.h"

using namespace v8;
using namespace node;

std::pair<Point, Point>
DynamicPngStack::optimal_dimension()
{
    Point top(-1, -1), bottom(-1, -1);
    for (vPngi it = png_stack.begin(); it != png_stack.end(); ++it) {
        Png *png = *it;
        if (top.x == -1 || png->x < top.x)
            top.x = png->x;
        if (top.y == -1 || png->y < top.y)
            top.y = png->y;
        if (bottom.x == -1 || png->x + png->w > bottom.x)
            bottom.x = png->x + png->w;
        if (bottom.y == -1 || png->y + png->h > bottom.y)
            bottom.y = png->y + png->h;
    }

    return std::make_pair(top, bottom);
}

void
DynamicPngStack::construct_png_data(unsigned char *data, Point &top)
{
    for (vPngi it = png_stack.begin(); it != png_stack.end(); ++it) {
        Png *png = *it;
        int start = (png->y - top.y)*width*4 + (png->x - top.x)*4;
        unsigned char *pngdatap = png->data;
        for (int i = 0; i < png->h; i++) {
            unsigned char *datap = &data[start + i*width*4];
            for (int j = 0; j < png->w; j++) {
                *datap++ = *pngdatap++;
                *datap++ = *pngdatap++;
                *datap++ = *pngdatap++;
                *datap++ = (buf_type == BUF_RGB || buf_type == BUF_BGR) ? 0x00 : *pngdatap++;
            }
        }
    }
}

void
DynamicPngStack::Initialize(Handle<Object> target)
{
    NanScope();

    Local<FunctionTemplate> t = FunctionTemplate::New(New);
    t->InstanceTemplate()->SetInternalFieldCount(1);
    NODE_SET_PROTOTYPE_METHOD(t, "push", Push);
    NODE_SET_PROTOTYPE_METHOD(t, "encode", PngEncodeAsync);
    NODE_SET_PROTOTYPE_METHOD(t, "encodeSync", PngEncodeSync);
    NODE_SET_PROTOTYPE_METHOD(t, "dimensions", Dimensions);
    target->Set(String::NewSymbol("DynamicPngStack"), t->GetFunction());
}

DynamicPngStack::DynamicPngStack(buffer_type bbuf_type) :
    buf_type(bbuf_type) {}

DynamicPngStack::~DynamicPngStack()
{
    for (vPngi it = png_stack.begin(); it != png_stack.end(); ++it)
        delete *it;
}

Handle<Value>
DynamicPngStack::Push(unsigned char *buf_data, size_t buf_len, int x, int y, int w, int h)
{
    NanScope();

    try {
        Png *png = new Png(buf_data, buf_len, x, y, w, h);
        png_stack.push_back(png);
        return scope.Close(Undefined());
    }
    catch (const char *e) {
        return ThrowException(Exception::Error(String::New(e)));
    }
}

Handle<Value>
DynamicPngStack::PngEncodeSync()
{
    NanScope();

    std::pair<Point, Point> optimal = optimal_dimension();
    Point top = optimal.first, bot = optimal.second;

    offset = top;
    width = bot.x - top.x;
    height = bot.y - top.y;

    unsigned char *data = (unsigned char*)malloc(sizeof(*data) * width * height * 4);
    if (!data) return ThrowException(Exception::Error(String::New("malloc failed in DynamicPngStack::PngEncode")));
    memset(data, 0xFF, width*height*4);

    construct_png_data(data, top);

    buffer_type pbt = (buf_type == BUF_BGR || buf_type == BUF_BGRA) ? BUF_BGRA : BUF_RGBA;

    try {
        PngEncoder encoder(data, width, height, pbt);
        encoder.encode();
        free(data);
        int png_len = encoder.get_png_len();
        Local<Object> retbuf = NanNewBufferHandle(png_len);
        memcpy(Buffer::Data(retbuf), encoder.get_png(), png_len);
        return scope.Close(retbuf);
    }
    catch (const char *err) {
        return ThrowException(Exception::Error(String::New(err)));
    }
}

Handle<Value>
DynamicPngStack::Dimensions()
{
    NanScope();

    Local<Object> dim = Object::New();
    dim->Set(String::NewSymbol("x"), Integer::New(offset.x));
    dim->Set(String::NewSymbol("y"), Integer::New(offset.y));
    dim->Set(String::NewSymbol("width"), Integer::New(width));
    dim->Set(String::NewSymbol("height"), Integer::New(height));

    return scope.Close(dim);
}

NAN_METHOD(DynamicPngStack::New)
{
    NanScope();

    buffer_type buf_type = BUF_RGB;
    if (args.Length() == 1) {
        if (!args[0]->IsString())
            return NanThrowTypeError("First argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");

        String::AsciiValue bts(args[0]->ToString());
        if (!(str_eq(*bts, "rgb") || str_eq(*bts, "bgr") ||
            str_eq(*bts, "rgba") || str_eq(*bts, "bgra")))
        {
            return NanThrowTypeError("First argument must be 'rgb', 'bgr', 'rgba' or 'bgra'.");
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
            return NanThrowTypeError("First argument wasn't 'rgb', 'bgr', 'rgba' or 'bgra'.");
    }

    DynamicPngStack *png_stack = new DynamicPngStack(buf_type);
    png_stack->Wrap(args.This());
    NanReturnValue(args.This());
}

NAN_METHOD(DynamicPngStack::Push)
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

    DynamicPngStack *png_stack = ObjectWrap::Unwrap<DynamicPngStack>(args.This());

    Local<Object> buf_obj = args[0].As<Object>();
    char *buf_data = Buffer::Data(buf_obj);
    size_t buf_len = Buffer::Length(buf_obj);

    NanReturnValue(png_stack->Push((unsigned char*)buf_data, buf_len, x, y, w, h));
}

NAN_METHOD(DynamicPngStack::Dimensions)
{
    NanScope();

    DynamicPngStack *png_stack = ObjectWrap::Unwrap<DynamicPngStack>(args.This());
    NanReturnValue(png_stack->Dimensions());
}

NAN_METHOD(DynamicPngStack::PngEncodeSync)
{
    NanScope();

    DynamicPngStack *png_stack = ObjectWrap::Unwrap<DynamicPngStack>(args.This());
    NanReturnValue(png_stack->PngEncodeSync());
}

void DynamicPngStack::DynamicPngEncodeWorker::Execute() {
    std::pair<Point, Point> optimal = png_obj->optimal_dimension();
    Point top = optimal.first, bot = optimal.second;

    png_obj->offset = top;
    png_obj->width = bot.x - top.x;
    png_obj->height = bot.y - top.y;

    unsigned char *data = (unsigned char*)malloc(sizeof(*data) * png_obj->width * png_obj->height * 4);
    if (!data) {
        errmsg = strdup("malloc failed in DynamicPngStack::UV_PngEncode.");
        return;
    }

    memset(data, 0xFF, png_obj->width*png_obj->height*4);

    png_obj->construct_png_data(data, top);

    buffer_type pbt = (png_obj->buf_type == BUF_BGR || png_obj->buf_type == BUF_BGRA) ?
        BUF_BGRA : BUF_RGBA;

    try {
        PngEncoder encoder(data, png_obj->width, png_obj->height, pbt);
        encoder.encode();
        free(data);
        png_len = encoder.get_png_len();
        png = (char *)malloc(sizeof(*png)*png_len);
        if (!png) {
            errmsg = strdup("malloc in DynamicPngStack::DynamicPngEncodeWorker::Execute() failed.");
            return;
        }
        else {
            memcpy(png, encoder.get_png(), png_len);
        }
    }
    catch (const char *err) {
        if (data) free(data);
        errmsg = strdup(err);
    }
}

void DynamicPngStack::DynamicPngEncodeWorker::HandleOKCallback() {
    NanScope();

    Local<Object> buf = NanNewBufferHandle(png_len);
    memcpy(Buffer::Data(buf), png, png_len);
    Local<Value> argv[3] = {buf, png_obj->Dimensions(), Undefined()};

    TryCatch try_catch; // don't quite see the necessity of this

    callback->Call(3, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    free(png);
    png = NULL;

    png_obj->Unref();
}

void DynamicPngStack::DynamicPngEncodeWorker::HandleErrorCallback() {
    NanScope();

    Local<Value> argv[3] = {Undefined(), Undefined(), Exception::Error(String::New(errmsg))};

    TryCatch try_catch; // don't quite see the necessity of this

    callback->Call(3, argv);

    if (try_catch.HasCaught())
        FatalException(try_catch);

    if (png) {
        free(png);
        png = NULL;
    }

    png_obj->Unref();
}

NAN_METHOD(DynamicPngStack::PngEncodeAsync)
{
    NanScope();

    if (args.Length() != 1)
        return NanThrowError("One argument required - callback function.");

    if (!args[0]->IsFunction())
        return NanThrowTypeError("First argument must be a function.");

    Local<Function> callback = Local<Function>::Cast(args[0]);
    DynamicPngStack *png = ObjectWrap::Unwrap<DynamicPngStack>(args.This());

    NanAsyncQueueWorker(new DynamicPngStack::DynamicPngEncodeWorker(new NanCallback(callback), png));

    png->Ref();

    NanReturnUndefined();
}


#ifndef PNG_ENCODER_H
#define PNG_ENCODER_H

#include <png.h>

#include "common.h"
#include "nan.h"

class PngEncoder {
    int width, height;
    unsigned char *data;
    char *png;
    unsigned int png_len, mem_len;
    buffer_type buf_type;

public:
    PngEncoder(unsigned char *ddata, int width, int hheight, buffer_type bbuf_type);
    ~PngEncoder();

    class EncodeWorker : public NanAsyncWorker {
    public:
        EncodeWorker(NanCallback *callback, char *buf_data=NULL) : NanAsyncWorker(callback), png(NULL), png_len(0), buf_data(buf_data) {
              png = NULL;
              png_len = 0;
        };

    protected:
        char *png;
        int png_len;
        char *buf_data;
    };

    static void png_chunk_producer(png_structp png_ptr, png_bytep data, png_size_t length);
    void encode();
    const char *get_png() const;
    int get_png_len() const;
};

#endif


#pragma once
#include <iostream>
#include <string>
#include <exception>
#include <memory>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class FormatContext
{
private:
    AVFormatContext* fmt_{nullptr};
    AVStream* video_stream_{nullptr};
    AVStream* audio_stream_{nullptr};
public:
    FormatContext(const std::string& fpath);
    ~FormatContext();
    AVFormatContext* self();
    AVStream* video_ID();
    AVStream* audioID();
    double fps();
    AVRational videoTimeBase();
    AVRational audioTimeBase();
    int64_t duration();
};

class CodecContext
{
private:
    AVCodecContext* ctx_{nullptr};
    AVDictionary* opts_{nullptr};
public:
    CodecContext(AVStream* stream);
    ~CodecContext();
    AVCodecContext* self();
    int width();
    int height();
    AVPixelFormat format();
    std::string codecName();
};

class Packet
{
private:
    AVPacket* pkt_{nullptr};
public:
    Packet();
    ~Packet();
    bool getPacket(FormatContext* f);
    bool is_Stream(const int stream);
    bool send(CodecContext* c, int* eof);
    int64_t lenght();
    void unref();
};

class Frame
{
private:
    AVFrame* frame_{nullptr};
public:
    Frame();
    ~Frame();
    unsigned char** _data();
    int* linesize();
    bool receive(CodecContext* c, Packet* p);
    int width();
    int height();
    int64_t timeStamp();
    void unref();
};

class scale_image
{
private:
    SwsContext* sws_{nullptr};
public:
    scale_image(CodecContext* c);
    ~scale_image();
    bool getDataFromFrame(Frame* f, unsigned char* _buffer);
};

class Decoder
{
private:
    std::unique_ptr<FormatContext> fmt;
    std::unique_ptr<CodecContext> ctx;
    std::unique_ptr<Packet> pkt;
    std::unique_ptr<Frame> frame;
    std::unique_ptr<scale_image> si;
public:
    Decoder(const std::string& file_path);
    ~Decoder() = default;
    bool readFrameFromDecoder(unsigned char* frame_buffer, int64_t* pts, int *eof);
    bool readSeekFrameFromDecoder(int64_t pts, unsigned char* frame_buffer, int64_t* _ts, int* eof);
    int width();
    int height();
    double fps();
    int64_t duration();
    AVRational timeBase();
    void seek(int64_t ts);
    int64_t startTime();
};


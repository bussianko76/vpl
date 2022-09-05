#include "Decoder.hpp"
#define EXIT std::exit(EXIT_FAILURE)

static std::string ffmpeg_error_string(const int errnum)
{
    char emsg[1024];
    av_strerror(errnum, emsg, 512);
    return std::string(emsg);
}

Decoder::Decoder(const std::string &file_path):
    fmt{std::make_unique<FormatContext>(file_path)},
    ctx{std::make_unique<CodecContext>(fmt->video_ID())},
    pkt{std::make_unique<Packet>()},
    frame{std::make_unique<Frame>()},
    si{std::make_unique<scale_image>(ctx.get())}
{}

bool Decoder::readFrameFromDecoder(unsigned char *frame_buffer, int64_t *pts, int* eof)
{
    while(true)
    {
        if(!pkt->getPacket(fmt.get())) return false;
        if(!pkt->is_Stream(fmt->video_ID()->index))
        {
            pkt->unref();
            continue;
        }
        if(!pkt->send(ctx.get(), eof)) return false;
        if(!frame->receive(ctx.get(), pkt.get())) continue;
        break;
    }
    *pts = frame->timeStamp();
    return si->getDataFromFrame(frame.get(), frame_buffer);
}

bool Decoder::readSeekFrameFromDecoder(int64_t pts, unsigned char *frame_buffer, int64_t *_ts, int *eof)
{
    seek(pts);
    while(true)
    {
        if(!pkt->getPacket(fmt.get())) return false;
        if(!pkt->is_Stream(fmt->video_ID()->index))
        {
            pkt->unref();
            continue;
        }
        if(!pkt->send(ctx.get(), eof)) return false;
        if(!frame->receive(ctx.get(), pkt.get())) continue;
        if(frame->timeStamp() >= pts) break;
    }
    *_ts = frame->timeStamp();
    return si->getDataFromFrame(frame.get(), frame_buffer);
}

int Decoder::width()
{
    return ctx->width();
}

int Decoder::height()
{
    return ctx->height();
}

double Decoder::fps()
{
    return fmt->fps();
}

int64_t Decoder::duration()
{
    return fmt->duration() / 1000;
}

AVRational Decoder::timeBase()
{
    return fmt->videoTimeBase();
}

void Decoder::seek(int64_t ts)
{
    avcodec_flush_buffers(ctx->self());
    av_seek_frame(fmt->self(), fmt->video_ID()->index, ts, AVSEEK_FLAG_BACKWARD);
}

int64_t Decoder::startTime()
{
    return fmt->video_ID()->start_time;
}

FormatContext::FormatContext(const std::string &fpath)
{
    fmt_ = avformat_alloc_context();
    int ret = avformat_open_input(&fmt_, fpath.c_str(), nullptr, nullptr);
    if(ret != 0)
    {
        std::cerr << "Couldn't open input file. " << ffmpeg_error_string(ret) << "\n";
        EXIT;
    }

    ret = avformat_find_stream_info(fmt_, nullptr);
    if(ret < 0)
    {
        std::cerr << "Couldn't find any stream into file." << std::endl;
        EXIT;
    }

    for(int i{0}; i < fmt_->nb_streams; i++)
    {
        if(fmt_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) video_stream_ = fmt_->streams[i];
        else if(fmt_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) audio_stream_ = fmt_->streams[i];
    }
}

FormatContext::~FormatContext()
{
    if(fmt_)
    {
        avformat_close_input(&fmt_);
        avformat_free_context(fmt_);
        fmt_ = nullptr;
        video_stream_ = nullptr;
        audio_stream_ = nullptr;
    }
}

AVFormatContext *FormatContext::self()
{
    if(fmt_) return fmt_;
    return nullptr;
}

AVStream *FormatContext::video_ID()
{
    if(fmt_) return video_stream_;
    return nullptr;
}

AVStream *FormatContext::audioID()
{
    if(fmt_) return audio_stream_;
    return nullptr;
}

double FormatContext::fps()
{
    if(video_stream_) return av_q2d(video_stream_->avg_frame_rate);
    return 0.0;
}

AVRational FormatContext::videoTimeBase()
{
    if(video_stream_) return video_stream_->time_base;
    return AVRational{0, 0};
}

AVRational FormatContext::audioTimeBase()
{
    if(audio_stream_) return audio_stream_->time_base;
    return AVRational{0, 0};
}

int64_t FormatContext::duration()
{
    return AV_NOPTS_VALUE ? fmt_->duration : video_stream_->duration;
}

CodecContext::CodecContext(AVStream *stream)
{
    const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if(!decoder)
    {
        std::cerr << "Couldn't find decoder. " <<"\n";
        EXIT;
    }

    ctx_ = avcodec_alloc_context3(decoder);
    if(!ctx_)
    {
        std::cerr << "Couldn't create codec context." <<"\n";
        EXIT;
    }

    int ret = avcodec_parameters_to_context(ctx_, stream->codecpar);
    if(ret < 0)
    {
        std::cerr << "Couldn't copy codec parameters. " << ffmpeg_error_string(ret) << std::endl;
        EXIT;
    }

    ret = av_dict_set(&opts_, "threads", "auto", 0);
    if(ret < 0)
    {
        std::cerr << "Couldn't set threads count. " << ffmpeg_error_string(ret) <<"\n";
        EXIT;
    }

    ret = avcodec_open2(ctx_, decoder, &opts_);
    if(ret != 0)
    {
        std::cerr << "Couldn't open codec: " << ffmpeg_error_string(ret) <<"\n";
        EXIT;
    }
}

CodecContext::~CodecContext()
{
    if(ctx_)
    {
        avcodec_close(ctx_);
        avcodec_free_context(&ctx_);
        ctx_ = nullptr;
    }

    if(opts_)
    {
        av_dict_free(&opts_);
        opts_ = nullptr;
    }
}

AVCodecContext *CodecContext::self()
{
    if(ctx_) return ctx_;
    return nullptr;
}

int CodecContext::width()
{
    if(ctx_) return ctx_->width;
    return 0;
}

int CodecContext::height()
{
    if(ctx_) return ctx_->height;
    return 0;
}

AVPixelFormat CodecContext::format()
{
    if(ctx_) return ctx_->pix_fmt;
    return AV_PIX_FMT_NONE;
}

std::string CodecContext::codecName()
{
    if(ctx_) return std::string(ctx_->codec->name);
    return "NONE";
}

Packet::Packet()
{
    pkt_ = av_packet_alloc();
    if(!pkt_)
    {
        std::cerr << "Couldn't create media packet." <<"\n";
        EXIT;
    }
}

Packet::~Packet()
{
    if(pkt_)
    {
        unref();
        av_packet_free(&pkt_);
        pkt_ = nullptr;
    }
}

bool Packet::getPacket(FormatContext *f)
{
    return av_read_frame(f->self(), pkt_) >= 0;
}

bool Packet::is_Stream(const int stream)
{
    return pkt_->stream_index == stream;
}

bool Packet::send(CodecContext *c, int *eof)
{
    int ret = avcodec_send_packet(c->self(), pkt_);
    if(ret == AVERROR_EOF)
    {
        avcodec_send_packet(c->self(), nullptr);
        *eof = 1;
    }
    else if(ret < 0)
    {
        std::cerr << "Failed to decode packet: " << ffmpeg_error_string(ret) << std::endl;
        return false;
    }
    return true;
}

int64_t Packet::lenght()
{
    return pkt_->duration;
}

void Packet::unref()
{
    if(pkt_) av_packet_unref(pkt_);
}

Frame::Frame()
{
    frame_ = av_frame_alloc();
    if(!frame_)
    {
        std::cerr << "Couldn't create media frame." <<"\n";
        EXIT;
    }
}

Frame::~Frame()
{
    if(frame_)
    {
        unref();
        av_frame_free(&frame_);
        frame_ = nullptr;
    }
}

unsigned char **Frame::_data()
{
    if(frame_) return  frame_->data;
    return nullptr;
}

int *Frame::linesize()
{
    if(frame_) return frame_->linesize;
    return nullptr;
}

bool Frame::receive(CodecContext *c, Packet *p)
{
    int ret = avcodec_receive_frame(c->self(), frame_);
    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
    {
        p->unref();
        return false;
    }
    else if(ret < 0)
    {
        std::cerr << "Failed to decode frame: " << ffmpeg_error_string(ret) << "\n";
        return false;
    }
    return true;
}

int Frame::width()
{
    if(frame_) return frame_->width;
    return 0;
}

int Frame::height()
{
    if(frame_) return frame_->height;
    return 0;
}

int64_t Frame::timeStamp()
{
    if(frame_) return frame_->pts;
    return 0;
}

void Frame::unref()
{
    if(frame_) av_frame_unref(frame_);
}

scale_image::scale_image(CodecContext *c)
{
    sws_ = sws_getContext(c->width(), c->height(), c->format(), c->width(), c->height(), AV_PIX_FMT_RGB0, SWS_BILINEAR, nullptr,
                          nullptr, nullptr);
}

scale_image::~scale_image()
{
    if(sws_)
    {
        sws_freeContext(sws_);
        sws_ = nullptr;
    }
}

bool scale_image::getDataFromFrame(Frame *f, unsigned char *_buffer)
{
    if(sws_)
    {
        unsigned char* dst[4] = {_buffer, nullptr, nullptr, nullptr};
        int ln[4] = {f->width()*4, 0, 0, 0};
        return sws_scale(sws_, f->_data(), f->linesize(), 0, f->height(), dst, ln) >= 0;
    }
    return false;
}

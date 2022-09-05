#pragma once
#include "window/VPLRender.hpp"
#include "ffmpeg/Decoder.hpp"
#include <memory>

class Player
{
private:
    std::unique_ptr<VPLRender> rnd;
    std::unique_ptr<Decoder> dec;
    std::string video_dur;
    double speed{1.0};
    void updateCounter(int id);
public:
    Player(const std::string& file_path);
    ~Player() = default;
    void operator()();
};


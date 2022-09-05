#include "Player.hpp"
#include <vector>
#include <chrono>

bool b_pause_play{false};
bool b_seekable{false};
std::size_t idx{0};
double frame_per_sec{0.0};

void Player::updateCounter(int id)
{
    int s{0}, sec{0}, min{0}, hr{0};
    int cor{1};
    s = id / dec->fps();
    hr = s / 3600;
    min = s / 60;
    if(min > 60) min -= 60;
    sec = s % 60;
    std::string shr, smin, ssec;
    if(sec < 10) ssec = "0"+std::to_string(sec);
    else ssec = std::to_string(sec);
    if(min < 10) smin = "0"+std::to_string(min);
    else smin = std::to_string(min);
    if(hr < 10) shr = "0"+std::to_string(hr);
    else shr = std::to_string(hr);
    std::string newTitle = "VPL   " + shr+":"+smin+":"+ssec+" - " + video_dur;
    glfwSetWindowTitle(rnd->window(), newTitle.c_str());
}

Player::Player(const std::string &file_path):
    rnd{std::make_unique<VPLRender>()},
    dec{std::make_unique<Decoder>(file_path)}
{
    int sec = dec->duration() / 1000;
    int hour = sec / 3600;
    int min = sec / 60;
    if(min > 60) min = min - 60;
    int second = sec % 60;

    std::string sh;
    if(hour < 10) sh = "0" + std::to_string(hour);
    else sh = std::to_string(hour);
    std::string sm;
    if(min < 10) sm = "0" + std::to_string(min);
    else sm = std::to_string(min);
    std::string ssec;
    if(second < 10) ssec = "0" + std::to_string(second);
    else ssec = std::to_string(second);
    video_dur = sh+":"+sm+":"+ssec;
    frame_per_sec = dec->fps();
}

void Player::operator()()
{
    std::vector<unsigned char> pic(dec->width()*dec->height()*4);
    int64_t ts;
    int eof{0};
    while(!glfwWindowShouldClose(rnd->window()))
    {
        glfwPollEvents();
        if(b_seekable)
        {
            int64_t one_f = dec->timeBase().den / dec->fps();
            int64_t seek_ts = idx * one_f;
            dec->seek(seek_ts);
            if(!dec->readSeekFrameFromDecoder(seek_ts, &pic[0], &ts, &eof))
            {
                if(eof == 1) break;
                std::cerr << "Couldn't find seeking frame." <<"\n";
                break;
            }
            rnd->paint(pic.data(), dec->width(), dec->height());
            glfwSwapBuffers(rnd->window());
            double nt = ts * (double)dec->timeBase().num / (double)dec->timeBase().den;
            glfwSetTime(nt);
            b_seekable = false;
        }
        if(b_pause_play)
        {
            double oldt = glfwGetTime();
            while(b_pause_play)
            {
                glfwPollEvents();
            }
            glfwSetTime(oldt);
        }
        if(!dec->readFrameFromDecoder(&pic[0], &ts, &eof))
        {
            if(eof == 1) break;
            std::cerr << "Couldn't load video frame." <<"\n";
            break;
        }

        static bool first = true;
        if(first)
        {
            glfwSetTime(0.0);
            first = false;
        }

        double sec = (ts * (double)dec->timeBase().num / (double)dec->timeBase().den) * speed;
        while(sec > glfwGetTime())
        {
            glfwWaitEventsTimeout(sec - glfwGetTime());
        }
        rnd->paint(pic.data(), dec->width(), dec->height());
        glfwSwapBuffers(rnd->window());
        updateCounter(idx);
        idx++;
    }
}

#include "Player.hpp"

int main(int argc, const char** argv)
{
    Player play{argv[1]};
    play();
    return 0;
}

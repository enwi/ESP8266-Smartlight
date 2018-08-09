#include <cmath>        // fmod
#include <iostream>
#include <random>
#include <signal.h>     // needed for SignalHandler
#include <thread>

#include "Color.hpp"
#include "RemoteRGBWStrip.hpp"
#include "RemoteRGBWStrip2.hpp"



const std::string IP = "192.168.2.143";   // The ip to which to send data to
const uint32_t PORT = 1996;               // The port on which to send data

//const uint16_t LEDS = 120;
const uint16_t LEDS = 56;

bool stop_all_threads = false;
struct sigaction sig_int_handler;

void SignalHandler(int signal)
{
    // end program
    std::cout << std::endl;
    std::cout << "Shutting down strip sender!" << std::endl;
    //ShutdownAllThreads();
    std::cout << "Shutdown complete!" << std::endl;
    exit(0);
}

void SetupSignalHandler()
{
    sig_int_handler.sa_handler = SignalHandler;
    sigfillset(&sig_int_handler.sa_mask);
    sig_int_handler.sa_flags = 0;

    sigaction(SIGINT, &sig_int_handler, NULL);
    //sigaction(SIGSEGV, &sig_int_handler, NULL);
    sigaction(SIGILL, &sig_int_handler, NULL);
    sigaction(SIGFPE, &sig_int_handler, NULL);
    sigaction(SIGABRT, &sig_int_handler, NULL);
    sigaction(SIGTERM, &sig_int_handler, NULL);
    stop_all_threads = false;
}

namespace utility
{
    inline std::minstd_rand & GetGenerator()
    {
        thread_local std::minstd_rand gen(std::random_device{}());
        return gen;
    }

    double GetRandomNumber(double min, double max)
    {
        std::uniform_real_distribution<double> dis(min, max);
        return dis(GetGenerator());
    }

    float GetRandomNumber(float min, float max)
    {
        std::uniform_real_distribution<float> dis(min, max);
        return dis(GetGenerator());
    }

    template<typename T>
    T GetRandomNumber(T min, T max)
    {
        std::uniform_int_distribution<T> dis(min, max);
        return dis(GetGenerator());
    }

    template<typename Iter, typename RandomGenerator>
    Iter GetRandomElement(Iter start, Iter end, RandomGenerator& g)
    {
        std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
        std::advance(start, dis(g));
        return start;
    }

    template<typename Iter>
    Iter GetRandomElement(Iter start, Iter end)
    {
        return GetRandomElement(start, end, GetGenerator());
    }
}
std::vector<std::pair<uint16_t,Color>> stars = {};
void twinkle(RemoteRGBWStrip &strip)
{
    // create random stars
    uint16_t create_star = utility::GetRandomNumber(uint16_t(0), uint16_t(0));
    std::vector<std::pair<uint16_t,Color>>::iterator star_iter = stars.end();
    if(!create_star)
    {
        uint16_t rand_led = utility::GetRandomNumber(uint16_t(0), uint16_t(LEDS-1));
        star_iter = find_if(stars.begin(), stars.end(), [&rand_led](const std::pair<uint16_t,Color>& star) { return star.first == rand_led; } );
        if(star_iter == stars.end())
        {
            stars.push_back({rand_led,{Color::HSV{0.0, 0.0, 1.0}}});
            //std::cout << "creating" << std::endl;
        }
    }

    // show
    strip = stars;
    strip.show();

    // decay
    star_iter = stars.begin();
    while (star_iter != stars.end())
    {
        Color::HSV star_color = star_iter->second.getHSV();
        if(star_color.v > 0.0)
        {
            star_color.v -= 0.005;
            star_iter->second = star_color;
            ++star_iter;
        }
        else
        {
            star_iter = stars.erase(star_iter);
        }
    }
}

Color::HSV color = {0.0, 1.0, 1.0};
void colorLoop(RemoteRGBWStrip &strip)
{
    Color::HSV offset_color = color;
    const double COLOR_OFFSET = (1.0/LEDS)*360.0;
    //std::cout << "HSV: " << offset_color.h << ", " << offset_color.s << ", " << offset_color.v << std::endl;
    for(uint16_t i = 0; i < LEDS; ++i)
    {
        strip.setLedColor(i, offset_color);
        offset_color.h = std::fmod(offset_color.h + COLOR_OFFSET, 360.0);
    }
    strip.show();
    // strip.setLedColor(led_num, color);
    // strip.show();
    // ++led_num;
    // if(led_num > LEDS)
    // {
    //     led_num = 0;
    // }
    color.h += 1.0;
    if(color.h > 360)
    {
        color.h = 0;
    }
}

uint8_t animation_type = 0;
std::chrono::system_clock::time_point last_animation_change = std::chrono::system_clock::now();
int main(int argc, char *argv[])
{
    std::srand(std::time(nullptr));     // init std::rand()
    RemoteRGBWStrip strip1(LEDS, IP, PORT);
    strip1.clear();
    strip1.setBrightness(150);
    strip1.show();
    uint16_t led_num = 0;
    while(1)
    {
        switch (animation_type)
        {
            case 0:
                colorLoop(strip1);
            break;
            case 1:
                twinkle(strip1);
            break;
        }
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        if( (now - last_animation_change) >= std::chrono::seconds(10))
        {
            ++animation_type;
            if(animation_type > 1)
            {
                animation_type = 0;
            }
            last_animation_change = now;
        }
    }

    return 0;
}

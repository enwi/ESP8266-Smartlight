
#pragma once

#include <chrono>           // time_point, duration
#include <arpa/inet.h>      // sockaddr_in
#include <string>
#include <vector>

#include "Color.hpp"        // colors

extern "C"
{
#include "../pb_common.h"
#include "../pb_encode.h"   // protobuf message encoder
#include "../strip.pb.h"    // protobuf message definition
}

class RemoteRGBWStrip
{
friend class RemoteRGBWStrip2;
public:
    RemoteRGBWStrip(uint16_t led_count, std::string ip, uint32_t port);
    virtual ~RemoteRGBWStrip();
    virtual void operator=(const std::vector<std::pair<uint16_t,Color>> &rhs);
    virtual void operator=(const std::vector<Color> &rhs);
    virtual void setLedColor(uint16_t led_index, Color col);
    virtual void setBrightness(uint8_t brt);
    virtual void clear();
    virtual void show();

protected:

private:
    virtual Color& operator[](uint16_t led_index);
    static bool NanopbLedEncoding(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);

private:
    std::vector<Color> leds;
    std::vector<std::pair<uint16_t,Color>> led_queue;
    bool brt_changed;
    uint8_t brt;
    Strip message;
    std::chrono::system_clock::time_point last_send_time;
    int udp_socket_fd;
    struct sockaddr_in si_other;
    int si_len;
    uint8_t UDP_buffer[1460];
    const uint32_t USEC_SEND_EVERY = 6000;  // a value of 6000 - 8000 works best (6-8 milliseconds)
};

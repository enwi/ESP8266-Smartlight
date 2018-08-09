#include "RemoteRGBWStrip.hpp"

//#include <sys/socket.h>
#include <iostream>         // cout
#include <iterator>         // iterator
#include <thread>           // sleep_for

#ifdef __APPLE__
#include <unistd.h>         // close
#define SOL_IP IPPROTO_IP
#endif


RemoteRGBWStrip::RemoteRGBWStrip(uint16_t led_count, std::string ip, uint32_t port=1996) :
    leds{},
    led_queue{},
    brt_changed{false},
    brt{0},
    message{},
    last_send_time{std::chrono::system_clock::now()},
    udp_socket_fd{-1},
    si_other{},
    si_len{sizeof(si_other)},
    UDP_buffer{}
{
    if ( (udp_socket_fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        std::cout << "Error during socket creation: " << std::strerror(errno) << std::endl;
        exit(1);
    }

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(port);

    if (inet_aton(ip.c_str(), &si_other.sin_addr) == 0)
    {
        std::cout << "inet_aton() failed: " << std::strerror(errno) << std::endl;
        exit(1);
    }

    leds.reserve(led_count);    // reserver space for maximum led count
    led_queue.reserve(led_count);
    while(led_count)
    {
        leds.push_back({});
        --led_count;
    }
}

RemoteRGBWStrip::~RemoteRGBWStrip()
{
    close(udp_socket_fd);
}

void RemoteRGBWStrip::operator=(const std::vector<std::pair<uint16_t,Color>> &rhs)
{
    for(std::pair<uint16_t,Color> led : rhs)
    {
        if(led.first < leds.size())
        {
            if((*this)[led.first] != led.second)
            {
                (*this)[led.first] = led.second;
                // should we check if there is already the same led in the queue?
                led_queue.push_back(led);
            }
        }
    }
}

void RemoteRGBWStrip::operator=(const std::vector<Color> &rhs)
{
    std::vector<Color>::iterator itlhs = leds.begin();
    std::vector<Color>::const_iterator itrhs = rhs.begin();
    uint16_t led_index = 0;
    while(itlhs != leds.end() || itrhs != rhs.end())
    {
        if(*itlhs != *itrhs)
        {
            *itlhs = *itlhs;
            led_queue.push_back({led_index, *itlhs});
        }
        ++itlhs;
        ++itrhs;
        ++led_index;
    }
}

void RemoteRGBWStrip::setLedColor(uint16_t led_index, Color col)
{
    if(led_index < leds.size())
    {
        if((*this)[led_index] != col)
        {
            (*this)[led_index] = col;
            // should we check if there is already the same led in the queue?
            led_queue.push_back({led_index, col});
        }
    }
}

void RemoteRGBWStrip::setBrightness(uint8_t brt)
{
    if(this->brt != brt)
    {
        this->brt = brt;
        brt_changed = true;
    }
}

// better use whole strip command for sending
void RemoteRGBWStrip::clear()
{
    led_queue.clear();
    for(uint16_t i = 0; i < leds.size(); ++i)
    {
        led_queue.push_back({i,{}});
    }
}

// maybe move execution to separate thread?
void RemoteRGBWStrip::show()
{
    if(led_queue.size())
    {
        pb_ostream_t stream = pb_ostream_from_buffer(UDP_buffer, sizeof(UDP_buffer));

        message.has_brt = brt_changed;
        if(brt_changed)
        {
            message.brt = brt;
            brt_changed = false;
        }

        // how to know if whole strip is changed to 1 color?
        message.has_whole = false;
        // message.has_whole = true;
        // message.whole.red = col.r*255;
        // message.whole.gre = col.g*255;
        // message.whole.blu = col.b*255;
        // message.whole.whi = col.w*255;

        // add all leds that changed
        message.leds.arg = static_cast<void*>(& led_queue);
        message.leds.funcs.encode = &RemoteRGBWStrip::NanopbLedEncoding;

        if (!pb_encode_delimited(&stream, Strip_fields, &message))
        {
            std::cout << "Encoding failed:" << PB_GET_ERROR(&stream) << std::endl;
            return;
        }

        // make sure to not send to often
        std::chrono::microseconds time_since_last_send = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - last_send_time);
        if( time_since_last_send < std::chrono::microseconds(USEC_SEND_EVERY))
        {
            std::chrono::microseconds sleep_time = std::chrono::microseconds(USEC_SEND_EVERY) - time_since_last_send;
            std::this_thread::sleep_for(sleep_time);
        }

        last_send_time = std::chrono::system_clock::now();
        //send the message
        if (sendto(udp_socket_fd, UDP_buffer, stream.bytes_written, 0 , (struct sockaddr *) &si_other, si_len) == -1)
        {
            std::cout << "Error during sendto: " << std::strerror(errno) << std::endl;
            return;
        }
        //std::cout << "Sent " << stream.bytes_written << " Bytes" << std::endl;

        led_queue.clear();

        //std::cout << "Size: " << stream.bytes_written << std::endl;
        //std::this_thread::sleep_for(std::chrono::milliseconds(8));
        //std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

/**** private ****/
// how to check for changes? Own Vector implementation, anyone?
Color& RemoteRGBWStrip::operator[](uint16_t led_index)
{
    if(led_index < leds.size())
    {
        return leds.at(led_index);
    }
    else
    {
        return leds.back();
    }
}

bool RemoteRGBWStrip::NanopbLedEncoding(pb_ostream_t *stream, const pb_field_t *field, void * const *arg)
{
    Strip_Led led = {};
    uint16_t led_index = 0;
    const std::vector<std::pair<uint16_t,Color>> led_queue_arg = *static_cast<const std::vector<std::pair<uint16_t,Color>> *>( *arg );
    for(const std::pair<uint16_t,Color> &queued_led : led_queue_arg)
    {
        Color::RGBW col_rgbw = queued_led.second.getRGBW();
        led.num = queued_led.first;
        led.rgbw = (static_cast<uint8_t>(col_rgbw.r*255) << 24) |
                   (static_cast<uint8_t>(col_rgbw.g*255) << 16) |
                   (static_cast<uint8_t>(col_rgbw.b*255) <<  8) |
                    static_cast<uint8_t>(col_rgbw.w*255);

        // This encodes the header for the field, based on the constant info from pb_field_t.
        if (!pb_encode_tag_for_field(stream, field))
        {
            std::cout << "pb_encode_tag_for_field failed" << std::endl;
            return false;
        }

        // This encodes the data for the field, based on our led structure.
        if (!pb_encode_submessage(stream, Strip_Led_fields, &led))
        {
            std::cout << "pb_encode_submessage failed" << std::endl;
            return false;
        }
        ++led_index;
    }

    return true;
}

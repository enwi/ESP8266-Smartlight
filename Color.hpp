
#pragma once

#include <stdint.h>     // uintx_t, intx_t
#include <cmath>        // fmod

class Color
{
public:
    struct RGB
    {
    public:
        bool operator==(const RGB &rhs)
        {
            return (this->r == rhs.r && this->g == rhs.g && this->b == rhs.b);
        };

    public:
        double r;   //!< a value between 0 and 1 as red channel
        double g;   //!< a value between 0 and 1 as green channel
        double b;   //!< a value between 0 and 1 as blue channel
    };

    struct RGBW
    {
    public:
        bool operator==(const RGBW &rhs)
        {
            return (this->r == rhs.r && this->g == rhs.g && this->b == rhs.b && this->w == rhs.w);
        };

    public:
        double r;   //!< a value between 0 and 1 as red channel
        double g;   //!< a value between 0 and 1 as green channel
        double b;   //!< a value between 0 and 1 as blue channel
        double w;   //!< a value between 0 and 1 as white channel
    };

    struct HSV
    {
    public:
        HSV() : h{0.0}, s{0.0}, v{0.0} {};
        HSV(double h, double s, double v)
        {
            this->h = std::fmod(h,360.0);
            this->s = (s > 1.0 ? 1.0 : s);
            this->v = (v > 1.0 ? 1.0 : v);
        };
        bool operator==(const HSV &rhs)
        {
            return (this->h == rhs.h && this->s == rhs.s && this->v == rhs.v);
        };

    public:
        double h;   // angle in degrees between 0 and 360
        double s;   // a fraction between 0 and 1
        double v;   // a fraction between 0 and 1
    };

public:
    Color() : color{0.0, 0.0, 0.0} {};
    Color(Color::RGB rgb) : color{RGBToHSV(rgb)} {};
    Color(Color::RGBW rgbw) : color{RGBWToHSV(rgbw)} {};
    Color(Color::HSV hsv) : color{hsv} {};

    Color& operator=(const Color &rhs)
    {
        if(this != &rhs)
        {
            this->color = rhs.color;
        }
        return *this;
    };

    Color& operator=(const Color::RGB &rhs)
    {
        this->color = RGBToHSV(rhs);
        return *this;
    };

    Color& operator=(const Color::RGBW &rhs)
    {
        this->color = RGBWToHSV(rhs);
        return *this;
    };

    Color& operator=(const Color::HSV &rhs)
    {
        this->color = rhs;
        return *this;
    };

    bool operator==(const Color &rhs)
    {
        return this->color == rhs.color;
    };

    bool operator!=(const Color &rhs)
    {
        return !((*this)==rhs);
    };

    Color::HSV getHSV() const {return color;};
    Color::RGB getRGB() const {return HSVToRGB(color);};
    Color::RGBW getRGBW()const {return HSVToRGBW(color);};

    static Color::RGB HSVToRGB(const Color::HSV col);
    static Color::HSV RGBToHSV(const Color::RGB col);

    static Color::RGBW HSVToRGBW(const Color::HSV col);
    static Color::HSV RGBWToHSV(const Color::RGBW col);
    static Color::RGB RGBWToRGB(const Color::RGBW col);
    static Color::RGBW RGBToRGBW(const Color::RGB col);

private:
    HSV color;
};

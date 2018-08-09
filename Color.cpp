#include "Color.hpp"

#include <algorithm>    // min

Color::RGB Color::HSVToRGB(Color::HSV col)
{
    if(col.s <= 0.0) // < is bogus, just shuts up warnings
    {
        return {col.v, col.v, col.v};
    }
    double hue = col.h;
    if(hue >= 360.0)
    {
        hue = 0.0;
    }
    hue /= 60.0;
    int32_t i = (int32_t)hue;
    double ff = hue - i;
    double p = col.v * (1.0 - col.s);
    double q = col.v * (1.0 - (col.s * ff));
    double t = col.v * (1.0 - (col.s * (1.0 - ff)));

    switch(i)
    {
    case 0:
        return {col.v, t, p};
        break;
    case 1:
        return {q, col.v, p};
        break;
    case 2:
        return {p, col.v, t};
        break;

    case 3:
        return {p, q, col.v};
        break;
    case 4:
        return {t, p, col.v};
        break;
    case 5:
    default:
        return {col.v, p, q};
        break;
    }
}


Color::HSV Color::RGBToHSV(Color::RGB col)
{
    Color::HSV out;
    double min;
    double max;
    double delta;

    min = col.r < col.g ? col.r : col.g;
    min = min   < col.b ? min   : col.b;

    max = col.r > col.g ? col.r : col.g;
    max = max   > col.b ? max   : col.b;

    out.v = max;    // v
    delta = max - min;
    if (delta < 0.00001)
    {
        out.s = 0;
        out.h = 0; // undefined, maybe nan?
        return out;
    }
    if( max > 0.0 ) // NOTE: if Max is == 0, this divide would cause a crash
    {
        out.s = (delta / max);  // s
    }
    else
    {
        // if max is 0, then r = g = b = 0
        // s = 0, h is undefined
        out.s = 0.0;
        out.h = NAN;    // its now undefined
        return out;
    }
    if( col.r >= max )  // > is bogus, just keeps compilor happy
    {
        out.h = ( col.g - col.b ) / delta;  // between yellow & magenta
    }
    else
    {
        if( col.g >= max )
        {
            out.h = 2.0 + ( col.b - col.r ) / delta;  // between cyan & yellow
        }
        else
        {
            out.h = 4.0 + ( col.r - col.g ) / delta;  // between magenta & cyan
        }
    }

    out.h *= 60.0;  // degrees

    if( out.h < 0.0 )
    {
        out.h += 360.0;
    }

    return out;
}

Color::RGBW Color::HSVToRGBW(Color::HSV col)
{
    if(col.s <= 0.0) // < is bogus, just shuts up warnings
    {
        return {0.0, 0.0, 0.0, col.v};
    }
    double hue = col.h;
    if(hue >= 360.0)
    {
        hue = 0.0;
    }
    hue /= 60.0;
    int32_t i = (int32_t)hue;
    double ff = hue - i;
    double p = col.v * (1.0 - col.s);
    double q = col.v * (1.0 - (col.s * ff));
    double t = col.v * (1.0 - (col.s * (1.0 - ff)));

    // get smallest value, which should be white
    // note there is no adaption for different white temperatures
    double min = std::min({col.v, p, q, t});

    switch(i)
    {
    case 0:
        return {col.v-min, t-min, p-min, min};
        break;
    case 1:
        return {q-min, col.v-min, p-min, min};
        break;
    case 2:
        return {p-min, col.v-min, t-min, min};
        break;

    case 3:
        return {p-min, q-min, col.v-min, min};
        break;
    case 4:
        return {t-min, p-min, col.v-min, min};
        break;
    case 5:
    default:
        return {col.v-min, p-min, q-min, min};
        break;
    }
}

Color::HSV Color::RGBWToHSV(Color::RGBW col)
{
    return RGBToHSV(RGBWToRGB(col));
}

Color::RGB Color::RGBWToRGB(Color::RGBW col)
{
    return {col.r+col.w, col.g+col.w, col.b+col.w};
}

Color::RGBW Color::RGBToRGBW(Color::RGB col)
{
    double min = std::min({col.r, col.g, col.b});
    return {col.r-min, col.g-min, col.b-min, min};
}

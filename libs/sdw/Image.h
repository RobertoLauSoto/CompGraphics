#include <algorithm>

class Image
{
public:
    struct Rgb
    {
        Rgb() : r(0), g(0), b(0) {}
        Rgb(float c) : r(c), g(c), b(c) {}
        Rgb(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
        bool operator != (const Rgb &c) const
        { return c.r != r || c.g != g || c.b != b; }
        Rgb& operator *= (const Rgb &rgb)
        { r *= rgb.r, g *= rgb.g, b *= rgb.b; return *this; }
        Rgb& operator += (const Rgb &rgb)
        { r += rgb.r, g += rgb.g, b += rgb.b; return *this; }
        friend float& operator += (float &f, const Rgb rgb)
        { f += (rgb.r + rgb.g + rgb.b) / 3.f; return f; }
        float r, g, b;
    };

    unsigned int w, h; // Image resolution
    Rgb *pixels; // 1D array of pixels
    std::string filename;
};

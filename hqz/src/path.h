
#include "ray.h"
#include <vector>

class Path
{
public:
    Path(Vec2 o, double lambda);
    Path(Vec2 o, double lambda, uint32_t path_length_expected);
    Path(Path &&p);

    size_t size() const;
    Vec2 get_origin() const;
    double get_wavelength() const;

    Vec2& operator[](size_t indx);

    Path& operator=(Path &&p);
    void append(Vec2 point);

private:
    Vec2 origin;
    std::vector<Vec2> v;    // Vertices in path traced
    double wavelength;            // wavelength of the path
};

Path::Path(Vec2 o, double lambda)
    : origin(o),
    wavelength(lambda)
{}

Path::Path(Vec2 o, double lambda, uint32_t path_length_expected)
    : Path(o,lambda)
{
    v.reserve(path_length_expected);
}

Path::Path(Path &&p) 
: v(std::move(p.v)) 
{
    origin = p.origin;
    wavelength = p.wavelength;
}

size_t Path::size() const{
    return v.size();
}

Vec2 Path::get_origin() const{
    return origin;
}
double Path::get_wavelength() const{
    return wavelength;
}

Vec2& Path::operator[](size_t indx) {
    return v[indx];
};

Path& Path::operator=(Path &&p){
    origin = p.origin;
    wavelength = p.wavelength;
    v = std::move(p.v);
    return *this;
};

void Path::append(Vec2 point) {
    v.push_back(point);
}
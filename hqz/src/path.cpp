
#include "path.h"

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

bool Path::erase_last(int n) {
    if ((0 < n) && (n < v.size())){
        v.erase(v.begin() + v.size()-n, v.end());
        return true;
    }
    return false;
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
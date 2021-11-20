
#pragma once
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

typedef std::vector<Path> Paths;


#include "ray.h"
#include <vector>

class Path
{
public:
    Path();
    Path(uint32_t path_length_expected);
    Path(Path &&path);

    Path& operator=(Path &&p);

    void append(Vec2 point);

private:

    std::vector<Vec2> v;    // Vertices in path traced
};

Path::Path(){}

Path::Path(uint32_t path_length_expected){
    v.reserve(path_length_expected);
}

Path::Path(Path &&path) 
: v(std::move(path.v)) 
{}

Path& Path::operator=(Path &&p){
    v = std::move(p.v);
    return *this;
};

void Path::append(Vec2 point) {
    v.push_back(point);
}
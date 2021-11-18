
#include "ray.h"
#include <vector>

class Path
{
public:
    Path(Vec2 o, Color c);
    Path(Vec2 o, Color c, uint32_t path_length_expected);
    Path(Path &&p);

    size_t size() const;
    Path& operator=(Path &&p);
    void append(Vec2 point);

private:
    Vec2 origin;
    std::vector<Vec2> v;    // Vertices in path traced
    Color color;            // Color of the path
};

Path::Path(Vec2 o, Color c)
    : origin(o),
    color(c)
{}

Path::Path(Vec2 o, Color c, uint32_t path_length_expected)
    : Path(o,c)
{
    v.reserve(path_length_expected);
}

Path::Path(Path &&p) 
: v(std::move(p.v)) 
{
    origin = p.origin;
    color = p.color;
}

size_t Path::size() const{
    return v.size();
}

Path& Path::operator=(Path &&p){
    origin = p.origin;
    color = p.color;
    v = std::move(p.v);
    return *this;
};

void Path::append(Vec2 point) {
    v.push_back(point);
}
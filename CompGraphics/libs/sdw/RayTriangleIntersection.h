#include <glm/glm.hpp>
#include <iostream>

class RayTriangleIntersection
{
  public:
    glm::vec3 intersectionPoint;
    float distanceFromCamera;
    ModelTriangle intersectedTriangle;
    float dot;
    float distanceFromLight;

    RayTriangleIntersection()
    {
    }

    RayTriangleIntersection(glm::vec3 point, float distance, ModelTriangle triangle, float dot_product, float dist)
    {
        intersectionPoint = point;
        distanceFromCamera = distance;
        dot = dot_product;
        distanceFromLight = dist;
        intersectedTriangle = triangle;
    }
};

std::ostream& operator<<(std::ostream& os, const glm::vec3& v)
{
    os << "{" << v.x << ", " << v.y << ", " << v.z << "}";
    return os;
}


std::ostream& operator<<(std::ostream& os, const RayTriangleIntersection& intersection)
{
    os << "Intersection is at " << intersection.intersectionPoint << " on triangle " << intersection.intersectedTriangle << " at a distance of " << intersection.distanceFromCamera << std::endl;
    return os;
}

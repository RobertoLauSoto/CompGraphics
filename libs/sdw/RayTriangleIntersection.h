#include <glm/glm.hpp>
#include <iostream>

class RayTriangleIntersection
{
  public:
    glm::vec3 intersectionPoint;
    glm::vec2 intersectionTexturePoint;
    float distanceFromCamera;
    ModelTriangle intersectedTriangle;

    RayTriangleIntersection()
    {
    }

    RayTriangleIntersection(glm::vec3 point, float distance, ModelTriangle triangle)
    {
        intersectionPoint = point;
        distanceFromCamera = distance;
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


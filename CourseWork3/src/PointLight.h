#ifndef POINT_LIGHT_H
#define POINT_LIGHT_H

#include "Light.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class PointLight : public Light
{
public:
    PointLight( glm::vec3 position, glm::vec3 color
        , float constant, float linear, float quadratic);

    glm::vec3 getPosition() { return _position; }
    float getConstant() { return _constant; }
    float getLinear() { return _linear; }
    float getQuadratic() { return _quadratic; }

    void setPosition(const glm::vec3& position) { _position = position; }
    void setConstant(float constant) { _constant = (constant > 0) ? constant : 1.0; }
    void setLinear(float linear) { _linear = (linear >= 0) ? linear : 1.0; }
    void setQuadratic(float quadratic) { _quadratic = (quadratic >= 0)? quadratic : 1.0; }

 private:
    glm::vec3 _position;
    float _constant;
    float _linear;
    float _quadratic;
};

#endif // !POINT_LIGHT_H
#ifndef SPOT_LIGHT_H
#define SPOT_LIGHT_H

#include "Light.h"

class SpotLight : public Light
{
public:
    SpotLight( glm::vec3 position, glm::vec3 color, glm::vec3 direction
        , float constant, float linear, float quadratic
        , float cutOff, float outerCutOff);   
    
    glm::vec3 getPosition() { return _position; }    
    glm::vec3 getDirection() { return _direction; }
    float getConstant() { return _constant; }
    float getLinear() { return _linear; }
    float getQuadratic() { return _quadratic; }
    float getCutOff() { return _cutOff; }
    float getCutOffInRadians() { return glm::radians(getCutOff()); }
    float getOuterCutOff() { return _outerCutOff; }
    float getOuterCutOffInRadians() { return glm::radians(getOuterCutOff()); }

    void setPosition(const glm::vec3& position) { _position = position; }
    void setDirection(const glm::vec3& direction) {_direction = direction;}
    void setCutOff(float cutOff);
    void setOuterCutOff(float outerCutOff);
    void setConstant(float constant) { _constant = (constant > 0) ? constant : 1.0; }
    void setLinear(float linear) { _linear = (linear >= 0) ? linear : 1.0; }
    void setQuadratic(float quadratic) { _quadratic = (quadratic >= 0) ? quadratic : 1.0; }
    
private:
    glm::vec3 _direction;
    glm::vec3 _position;
    float _constant;
    float _linear;
    float _quadratic;
    float _cutOff;     //stores angle (need to pass cos(cutOff) to shader)
    float _outerCutOff;// same as cutOff
};

#endif
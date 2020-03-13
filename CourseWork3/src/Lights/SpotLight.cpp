#include <Lights/SpotLight.h>

using namespace std;

SpotLight::SpotLight( glm::vec3 position, glm::vec3 color, glm::vec3 direction
    , float constant, float linear, float quadratic
    , float cutOff, float outerCutOff, LightState state)
    : Light(color, state)
{
    _position = position;
    _direction = direction;

    if (constant <= 0)
        constant = 1.0;
    if (linear < 0)
        linear = 1.0;
    if (quadratic < 0)
        quadratic = 1.0;

    _constant = constant;
    _linear = linear;
    _quadratic = quadratic;

    _cutOff = cutOff;
    _outerCutOff = outerCutOff;
}

void SpotLight::setCutOff(float cutOff)
{
    if (cutOff < 0)
        _cutOff = 0;
    else if (cutOff > 90)
        _cutOff = 90;
    else
        _cutOff = cutOff;
}

void SpotLight::setOuterCutOff(float outerCutOff)
{
    if (outerCutOff < 0)
        _outerCutOff = 0;
    else if (outerCutOff > 90)
        _outerCutOff = 90;
    else
        _outerCutOff = outerCutOff;
}
#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum class LightState
{
	On,
	Off
};

class Light
{
public:
    Light() = default;

    Light(glm::vec3 color, LightState state = LightState::On)
        : _color(color)
		, m_state(state)
        {}

    void setColor(glm::vec3 color) { _color = color; }     

    glm::vec3 getColor() { return _color; } 

	void setState(LightState state) { m_state = state; }

	LightState getState() { return m_state; }

	bool isOn() const  { return m_state == LightState::On; }

	void switchState() { m_state = isOn() ? LightState::Off : LightState::On; }

protected:    
    glm::vec3 _color;
	LightState m_state;
};


#endif
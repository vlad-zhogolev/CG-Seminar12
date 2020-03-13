#include <LightManager.h>

using namespace std;

float LightManager::movementSpeed = 5.0f;
const glm::vec3 LightManager::LEFT  = glm::vec3(1, 0, 0);
const glm::vec3 LightManager::UP    = glm::vec3(0, 1, 0);
const glm::vec3 LightManager::FRONT = glm::vec3(0, 0, 1);
const float LightManager::TIME_BETWEEN_SUN_STATES = 2.0f;

glm::vec3 defaultColorProvider(glm::vec3 initial, glm::vec3 target, float alpha)
{
	return glm::mix(initial, target, alpha);
}

glm::vec3 morningColorProvider(glm::vec3 initial, glm::vec3 target, float alpha)
{
	const auto mixCoef = glm::clamp(100 * alpha * alpha - 99, 0.f, 1.f);
	return glm::mix(initial, target, mixCoef);
}

glm::vec3 eveningColorProvider(glm::vec3 initial, glm::vec3 target, float alpha)
{
	return glm::mix(target, initial, -alpha * alpha + 1);
}

glm::vec3 nightColorProvider(glm::vec3 initial, glm::vec3 target, float alpha)
{
	const auto mixCoef = glm::clamp(-100 * alpha * alpha + 1, 0.f, 1.f);
	return glm::mix(target, initial, mixCoef);
}

std::map<TimeOfDay, SunState> LightManager::sunStates =
{
    {TimeOfDay::MORNING,{
        glm::vec3(0.98, 0.81, 0.30),
        //glm::vec3(0.29, 0.26, 0.24), 
		glm::vec3(0.0, 0.0, 0.0f),
		glm::vec3(-1, 0, 0),
        //glm::vec3(-1, -1, 0)
		glm::vec3(0, 1, 0),
		morningColorProvider
    }},
    {TimeOfDay::MIDDAY,{
        glm::vec3(0.98, 0.831, 0.25),
        glm::vec3(0.98, 0.81, 0.30),
        glm::vec3(0, -1, 0),
		glm::vec3(-1, 0, 0),
		defaultColorProvider
    }},
    {TimeOfDay::EVENING,{
        glm::vec3(0.96, 0.27, 0.27),
        glm::vec3(0.98, 0.81, 0.30),
		glm::vec3(1, 0, 0),
        glm::vec3(0, -1, 0),
		eveningColorProvider
    }},
    {TimeOfDay::NIGHT,{
        //glm::vec3(0.29, 0.26, 0.24),
		glm::vec3(0.0, 0.0, 0.0f),
		glm::vec3(0.96, 0.27, 0.27),
		//glm::vec3(-1, -1, 0),
		glm::vec3(0, 1, 0),
		glm::vec3(1, 0, 0),
		nightColorProvider
    }}
};

std::map<TimeOfDay, TimeOfDay> LightManager::nextTimeOfDay
{
    { TimeOfDay::MORNING,   TimeOfDay::MIDDAY },
    { TimeOfDay::MIDDAY,    TimeOfDay::EVENING },
    { TimeOfDay::EVENING,   TimeOfDay::NIGHT },
    { TimeOfDay::NIGHT,     TimeOfDay::MORNING }
};

void LightManager::switchToNext()
{
    switch (activeType)
    {
        case ActiveLightType::NONE:
        {
            return;
        }
        case ActiveLightType::POINT:
        {
            if (m_pointLights.size() > 0)
            {
                curPointLight = (curPointLight + 1) % m_pointLights.size();
            }
            break;
        }
        case ActiveLightType::SPOT:
        {
            if (m_spotLights.size() > 0)
            {
                curSpotLight = (curSpotLight + 1) % m_spotLights.size();
            }
            break;
        }
        case ActiveLightType::DIRECTIONAL:
        {
            if (m_directionalLights.size() > 0)
            {
                curDirectionalLight = (curDirectionalLight + 1) % m_directionalLights.size();
            }
            break;
        }
    }
}

void LightManager::switchToPrevious()
{
    switch (activeType)
    {
        case ActiveLightType::NONE:
        {
            return;
        }
        case ActiveLightType::POINT:
        {
            if (m_spotLights.size() > 0)
            {
                curPointLight = curPointLight == 0 ? m_pointLights.size() - 1 : (curPointLight - 1) % m_pointLights.size();
            }
            break;
        }
        case ActiveLightType::SPOT:
        {
            if (m_spotLights.size() > 0)
            {
                curSpotLight = curSpotLight == 0 ? m_spotLights.size() - 1 : (curSpotLight - 1) % m_spotLights.size();
            }
            break;
        }
        case ActiveLightType::DIRECTIONAL:
        {
            if (m_directionalLights.size() > 0)
            {
                curDirectionalLight = curDirectionalLight == 0 ? m_spotLights.size() - 1 : (curDirectionalLight - 1) % m_spotLights.size();
            }
            break;
        }
    }
}

void LightManager::switchLightType(ActiveLightType type)
{    
    if (activeType == type)
        activeType = ActiveLightType::NONE;
    else
        activeType = type;
}

void LightManager::translateCurrentLight(Direction dir)
{
    if (activeType == ActiveLightType::NONE ||
        (activeType == ActiveLightType::POINT && m_pointLights.size() == 0) ||
        (activeType == ActiveLightType::SPOT && m_spotLights.size() == 0))
    {
        return;
    }
    
    glm::vec3 delta;
    switch (dir)
    {
    case Direction::UP:
        delta = UP;
        break;
    case Direction::DOWN:
        delta = -UP;
        break;
    case Direction::FRONT:
        delta = FRONT;
        break;
    case Direction::BACK:
        delta = -FRONT;
        break;
    case Direction::LEFT:
        delta = LEFT;
        break;
    case Direction::RIGHT:
        delta = -LEFT;
        break;
    }
    delta *= movementSpeed * deltaTime;

    if (activeType == ActiveLightType::POINT)
    {
        m_pointLights[curPointLight].setPosition(m_pointLights[curPointLight].getPosition() + delta);
    }
    else if (activeType == ActiveLightType::SPOT)
    {
        m_spotLights[curSpotLight].setPosition(m_spotLights[curSpotLight].getPosition() + delta);
    }
}

void LightManager::switchLightState()
{
    if (activeType == ActiveLightType::NONE ||
       (activeType == ActiveLightType::POINT && m_pointLights.size() == 0) ||
       (activeType == ActiveLightType::SPOT && m_spotLights.size() == 0) ||
       (activeType == ActiveLightType::DIRECTIONAL && m_directionalLights.size() == 0))
    {
        return;
    }
    switch (activeType)
    {
        case ActiveLightType::POINT:
        {
            m_pointLights[curPointLight].switchState();
        }
        break;
        case ActiveLightType::SPOT:
        {
            m_spotLights[curSpotLight].switchState();
        }
        break;
        case ActiveLightType::DIRECTIONAL:
        {
            m_directionalLights[curDirectionalLight].switchState();
        }
		break;
        case ActiveLightType::SUN:
        {
			if (timeSinceSunStateChange == TIME_BETWEEN_SUN_STATES)
			{
				m_timeOfDay = nextTimeOfDay[m_timeOfDay];
				timeSinceSunStateChange = 0.0f;
			}
        }
        break;
    }
}

void LightManager::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // lights control
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        switchLightType(ActiveLightType::POINT);
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        switchLightType(ActiveLightType::SPOT);
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        switchLightType(ActiveLightType::DIRECTIONAL);
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
    {
        switchLightType(ActiveLightType::SUN);
    }
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
    {
        switchToPrevious();
    }
    if (key == GLFW_KEY_RIGHT&& action == GLFW_PRESS)
    {
        switchToNext();
    }
    if (key == GLFW_KEY_U && action == GLFW_REPEAT)
    {
        translateCurrentLight(Direction::UP);
    }
    if (key == GLFW_KEY_O && action == GLFW_REPEAT)
    {
        translateCurrentLight(Direction::DOWN);
    }
    if (key == GLFW_KEY_I && action == GLFW_REPEAT)
    {
        translateCurrentLight(Direction::FRONT);
    }
    if (key == GLFW_KEY_K && action == GLFW_REPEAT)
    {
        translateCurrentLight(Direction::BACK);
    }
    if (key == GLFW_KEY_J && action == GLFW_REPEAT)
    {
        translateCurrentLight(Direction::LEFT);
    }
    if (key == GLFW_KEY_L && action == GLFW_REPEAT)
    {
        translateCurrentLight(Direction::RIGHT);
    }
    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
    {
        movementSpeed = movementSpeed >= 10.0f ? 10.0f : movementSpeed + 1.0f;
    }
    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS)
    {
        movementSpeed = movementSpeed <= 0.0f ? 0.0f : movementSpeed - 1.0f;
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        switchLightState();
    }
}

void LightManager::update()
{
    if (timeSinceSunStateChange < TIME_BETWEEN_SUN_STATES)
    {
        timeSinceSunStateChange += deltaTime;
        timeSinceSunStateChange = glm::clamp(timeSinceSunStateChange, 0.0f, TIME_BETWEEN_SUN_STATES);

		auto initialColor = sunStates[m_timeOfDay].initialColor;
		auto destinationColor = sunStates[m_timeOfDay].destinationColor;

		m_sun.setColor(sunStates[m_timeOfDay].colorProvider(initialColor, destinationColor, timeSinceSunStateChange / TIME_BETWEEN_SUN_STATES));

		auto initialDirection = sunStates[m_timeOfDay].initialDirection;
		auto destinationDirection = sunStates[m_timeOfDay].destinationDirection;

        m_sun.setDirection(glm::mix(
            initialDirection,
            destinationDirection,
            timeSinceSunStateChange / TIME_BETWEEN_SUN_STATES
        ));
    }
}

void LightManager::switchTimeOfDay()
{
    // m_timeOfDay = nextTimeOfDay[m_timeOfDay];
    // switch (m_timeOfDay)
    // {
    //     case TimeOfDay::MORNING:
    //     {
    //         destinationColor = glm::vec3(0.98, 0.81, 0.30);
    //         initialColor = glm::vec3();

    //         destinationDirection = glm::vec3(-1, 0, 0);
    //     }
    //     break;
    //     case TimeOfDay::MIDDAY:
    //     {
    //         m_sun.setDirection(glm::vec3(0, -1, 0));
    //         m_sun.setColor(glm::vec3(0.98, 0.831, 0.25));
    //     }
    //     break;
    //     case TimeOfDay::EVENING:
    //     {
    //         m_sun.setDirection(glm::vec3(1, 0, 0));
    //         m_sun.setColor(glm::vec3(0.99, 0.7, 0.53));
    //     }
    //     break;
    //     case TimeOfDay::NIGHT:
    //     {
    //         // here sun is moon
    //         m_sun.setDirection(glm::vec3(-1, -1, 0));
    //         m_sun.setColor(glm::vec3(0.29, 0.26, 0.24));
    //     }
    //     break;
    // }
}
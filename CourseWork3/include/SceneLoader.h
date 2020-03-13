#ifndef SCENE_LOADER_H
#define SCENE_LOADER_H

#include <Lights/DirectionalLight.h>
#include <Lights/PointLight.h>
#include <Lights/SpotLight.h>
#include <Objects/Model.h>
#include <Objects/Object.h>
#include <Aliases.h>

#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <memory>

//TODO: convert this to class Scene which holds all scene data: lights, models, objects, etc...
class SceneLoader
{
    static const glm::vec3::value_type  MIN_ALLOWED_POSITION;
    static const glm::vec3::value_type  MAX_ALLOWED_POSITION;
    static const glm::vec3::value_type  MIN_ALLOWED_COLOR;
    static const glm::vec3::value_type  MAX_ALLOWED_COLOR;
    static const float                  MIN_ALLOWED_DEGREES_ANGLE;
    static const float                  MAX_ALLOWED_DEGREES_ANGLE;

public:

    SceneLoader() = default;

    void loadScene(std::string lightsDataPath, std::string modelsDataPath
        , DirectionalLights& dirLights, PointLights& pointLights, SpotLights& m_spotLights
        , Models& models, Objects& objects);

private:

    DirectionalLight loadDirectionalLight(std::stringstream& lightData, bool& good);
    PointLight loadPointLight(std::stringstream& lightData, bool& good);
    SpotLight loadSpotLight(std::stringstream& lightData, bool& good);

    int getModelIndex(std::string path, const Models& modles);

    glm::vec3 getVec3(std::stringstream& data);   

    bool checkRangeVec3(glm::vec3 vector, double left, double right, string message);
    bool checkAttenuation(double constant, double linear, double quadratic);
    bool checkAngles(double cutOff, double outerCutOff);
    bool checkScale(glm::vec3 scale);

    void exitOnError();

};

#endif


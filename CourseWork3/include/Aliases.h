#include <Lights/PointLight.h>
#include <Lights/SpotLight.h>
#include <Lights/DirectionalLight.h>
#include <Objects/Model.h>
#include <Objects/Object.h>

#include <vector>
#include <memory>

using DirectionalLights = vector<DirectionalLight>;
using PointLights = vector<PointLight>;
using SpotLights = vector<SpotLight>;
using Objects = vector<Object>;
using Models = vector<std::shared_ptr<Model>>;
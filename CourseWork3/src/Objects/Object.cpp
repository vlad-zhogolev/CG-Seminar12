#include <Objects/Object.h>

glm::mat4 Object::getModelMatrix()
{
    glm::mat4 model{};
    // translate
    model = glm::translate(model, _position);
    // rotate model using quaternion
    glm::quat quaternion(glm::vec3(glm::radians(_rotation.x), glm::radians(_rotation.y), glm::radians(_rotation.z)));
    model = model * toMat4(quaternion);
    // scale
    model = glm::scale(model, _scale);

    return model;
}
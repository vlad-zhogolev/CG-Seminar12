#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/Importer.hpp>
#include <assimp/Scene.h>
#include <assimp/postprocess.h>

#include <Shader.h>
#include <Camera.h>
#include <SceneLoader.h>
#include <LightManager.h>
#include <Objects/Model.h>
#include <Objects/Object.h>
#include <Aliases.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <vector>
#include <algorithm>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, LightManager& lightManager);
void renderCube();
void renderSeminarCube();
void renderPyramid();
void renderQuad();
void renderScreenQuad();
void renderSkybox(unsigned int cubemapTexture);
void renderScene(const Shader& shader);
unsigned int loadCubemap(std::vector<std::string> faces);
unsigned int loadTexture(const char* path);

// Screen settings
unsigned int screenWidth = 1200;
unsigned int screenHeight = 720;

// Camera settings
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Max number of lights (their values must match with values in shader)
const PointLights::size_type        MAX_NUMBER_OF_POINT_LIGHTS          = 32;
const SpotLights::size_type         MAX_NUMBER_OF_SPOT_LIGHTS           = 32;
const DirectionalLights::size_type  MAX_NUMBER_OF_DIRECTIONAL_LIGHTS    = 4;
const unsigned int                  SKYBOX_TEXTURE_INDEX                = 15;

// Scene contents
DirectionalLight sun(glm::vec3(0, -1, 0), glm::vec3(0.98, 0.831, 0.25));
DirectionalLights dirLights;
PointLights pointLights;
SpotLights spotLights;
Objects objects;
Models models;

// Scene settings
bool shadows = true;
bool shadowsKeyPressed = false;
bool renderSecondLight = false;
bool renderSecondLightKeyPressed = false;

// Point lights shadow maps size
const unsigned int POINT_LIGHT_SHADOW_MAP_WIDTH  = 1024; 
const unsigned int POINT_LIGHT_SHADOW_MAP_HEIGHT = 1024;

vector<std::string> faces
{
    "data/skybox/right.jpg",
    "data/skybox/left.jpg",
    "data/skybox/top.jpg",
    "data/skybox/bottom.jpg",
    "data/skybox/front.jpg",
    "data/skybox/back.jpg"
}; 

int main()
{        
    // set russian locale
    setlocale(LC_ALL, "Russian");

    // glfw: initialize and configure    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation   
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Seminar10 - Lighting", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Capture mouse by GLFW 
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Load all OpenGL function pointers   
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }   

    // Compile shaders       
    Shader shader("shaders/pbr.vert", "shaders/pbr.frag");   
    Shader shaderLightBox("shaders/deferred_light_box.vert", "shaders/deferred_light_box.frag");
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");

    // Shaders for shadows
    Shader pointShadowsShader("shaders/point_shadows.vert", "shaders/point_shadows.frag");
    Shader simpleDepthShader(
        "shaders/point_shadows_depth.vert",
        "shaders/point_shadows_depth.frag",
        "shaders/point_shadows_depth.geom"
    );
    Shader shadowAccumulatorShader("shaders/shadow_accumulator.vert", "shaders/shadow_accumulator.frag");
    Shader textureRenderingShader("shaders/textureRendering.vert", "shaders/textureRendering.frag");
    
    // Load scene   
    SceneLoader sceneLoader;
    sceneLoader.loadScene("LightData.txt", "ModelData.txt", dirLights, pointLights, spotLights, models, objects);             

    // Load skybox
    unsigned int cubemapTexture = loadCubemap(faces); 

    // Setup light manager and key callbacks for lights controls
    LightManager lightManager(pointLights, spotLights, dirLights, sun);
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowUserPointer(window, &lightManager);

    // Configure global OpenGL state: perform depth test, don't render faces, which don't look at user    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);


    // Set shader in use
    shader.use();

    // Setup sun
    shader.setVec3("sun.color",     sun.getColor());
    shader.setVec3("sun.direction", sun.getDirection());
    shader.setBool("sun.isOn",      sun.isOn());

    // Setup point lights
    PointLights::size_type pointLightsNumber = min(MAX_NUMBER_OF_POINT_LIGHTS, pointLights.size());   
    shader.setInt("pointLightsNumber", pointLightsNumber);   
    for (PointLights::size_type i = 0; i < pointLights.size(); ++i)
    {
        shader.setVec3( "pointLights[" + to_string(i) + "].position",   pointLights[i].getPosition());
        shader.setVec3( "pointLights[" + to_string(i) + "].color",      pointLights[i].getColor());        
        shader.setFloat("pointLights[" + to_string(i) + "].constant",   pointLights[i].getConstant());
        shader.setFloat("pointLights[" + to_string(i) + "].linear",     pointLights[i].getLinear());
        shader.setFloat("pointLights[" + to_string(i) + "].quadratic",  pointLights[i].getQuadratic());
        shader.setBool( "pointLights[" + to_string(i) + "].isOn",       pointLights[i].isOn());
    }    
    
    // Setup directional lights
    DirectionalLights::size_type dirLightsNumber = min(MAX_NUMBER_OF_DIRECTIONAL_LIGHTS, dirLights.size());
    shader.setInt("dirLightsNumber", dirLightsNumber);
    for (DirectionalLights::size_type i = 0; i < dirLights.size(); ++i)
    {
        shader.setVec3("dirLights[" + to_string(i) + "].color",         dirLights[i].getColor());
        shader.setVec3("dirLights[" + to_string(i) + "].direction",     dirLights[i].getDirection());
        shader.setBool("dirLights[" + to_string(i) + "].isOn",          dirLights[i].isOn());
    }

    // Setup spot lights
    SpotLights::size_type spotLightsNumber = min(MAX_NUMBER_OF_SPOT_LIGHTS, spotLights.size());
    shader.setInt("spotLightsNumber", spotLightsNumber);
    for(SpotLights::size_type i = 0; i < spotLights.size(); ++i)
    {        
        shader.setVec3( "spotLights[" + to_string(i) + "].position",    spotLights[i].getPosition());
        shader.setVec3( "spotLights[" + to_string(i) + "].color",       spotLights[i].getColor()); 
        shader.setVec3( "spotLights[" + to_string(i) + "].direction",   spotLights[i].getDirection());       
        shader.setFloat("spotLights[" + to_string(i) + "].constant",    spotLights[i].getConstant());
        shader.setFloat("spotLights[" + to_string(i) + "].linear",      spotLights[i].getLinear());
        shader.setFloat("spotLights[" + to_string(i) + "].quadratic",   spotLights[i].getQuadratic());
        shader.setFloat("spotLights[" + to_string(i) + "].cutOff",      glm::cos(spotLights[i].getCutOffInRadians()));
        shader.setFloat("spotLights[" + to_string(i) + "].outerCutOff", glm::cos(spotLights[i].getOuterCutOffInRadians()));
        shader.setBool( "spotLights[" + to_string(i) + "].isOn",        spotLights[i].isOn());
    }    

    // Depth map shader configuration
    //const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth cubemap texture
    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, POINT_LIGHT_SHADOW_MAP_WIDTH, POINT_LIGHT_SHADOW_MAP_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Configure shader for rendering scene with point light shadows
    pointShadowsShader.use();
    pointShadowsShader.setInt("diffuseTexture", 0);
    pointShadowsShader.setInt("depthMap", 1);

    auto woodTexture = loadTexture("data/textures/cube/container.png");

    auto createAndConfigureFramebuffer = [] (GLuint& framebuffer, GLuint& texture, GLuint& depthBuffer)
    {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        glGenRenderbuffers(1, &depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, screenWidth, screenHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
        // finally check if framebuffer is complete
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "Framebuffer not complete!" << std::endl;
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    };

    // Configure framebuffers for light rendering and color blending
    GLuint lightRenderFramebuffer;
    GLuint lightRenderTexture;
    GLuint lightDepthBuffer;
    createAndConfigureFramebuffer(lightRenderFramebuffer, lightRenderTexture, lightDepthBuffer);

    GLuint currentBlendingFramebuffer;
    GLuint currentBlendingTexture;
    GLuint currentBlendingDepthBuffer;
    createAndConfigureFramebuffer(currentBlendingFramebuffer, currentBlendingTexture, currentBlendingDepthBuffer);

    GLuint blendedFramebuffer;
    GLuint blendedTexture;
    GLuint blendedDepthBuffer;
    createAndConfigureFramebuffer(blendedFramebuffer, blendedTexture, blendedDepthBuffer);

    bool isFirstBlendingFramebufferUsed = true;

    // Configure shadow accumulation
    shadowAccumulatorShader.use();
    shadowAccumulatorShader.setInt("sourceTexture1", 0);
    shadowAccumulatorShader.setInt("sourceTexture2", 1);

    // Configure texture rendering shader
    textureRenderingShader.use();
    textureRenderingShader.setInt("sourceTexture", 0);

 

    // Render loop    
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic        
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        lightManager.updateDeltaTime(deltaTime);
        lightManager.update();

        // Render        
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calculate view and projection matrix for current state and position of camera
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            static_cast<float>(screenWidth) / static_cast<float>(screenHeight),
            0.1f,
            100.0f
        );
        glm::mat4 view = camera.GetViewMatrix();
        /*
                // Set shader in use and bind view and projection matrices
                shader.use();
                shader.setMat4("projection", projection);
                shader.setMat4("view", view);
                shader.setVec3("cameraPos", camera.Position);

                // Render objects
                for (unsigned int i = 0; i < objects.size(); i++)
                {
                    glm::mat4 model = objects[i].getModelMatrix();
                    shader.setMat4("model", model);

                    // Fixes normals in case of non-uniform model scaling
                    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
                    shader.setMat3("normalMatrix", normalMatrix);

                    objects[i].getModel()->Draw(shader);
                }

                shader.setVec3("sun.direction", sun.getDirection());
                shader.setVec3("sun.color",     sun.getColor());

                // Update point lights state
                for (PointLights::size_type i = 0; i < pointLights.size(); ++i)
                {
                    shader.setVec3("pointLights[" + to_string(i) + "].position",    pointLights[i].getPosition());
                    shader.setBool("pointLights[" + to_string(i) + "].isOn",        pointLights[i].isOn());
                }

                // Update spot lights state
                for (SpotLights::size_type i = 0; i < spotLights.size(); ++i)
                {
                    shader.setVec3("spotLights[" + to_string(i) + "].position", spotLights[i].getPosition());
                    shader.setBool("spotLights[" + to_string(i) + "].isOn",     spotLights[i].isOn());
                }

                // Update directional lights state
                for (DirectionalLights::size_type i = 0; i < dirLights.size(); ++i)
                {
                    shader.setBool("dirLights[" + to_string(i) + "].isOn",  dirLights[i].isOn());
                }

                // Render lights on top of scene
                shaderLightBox.use();
                shaderLightBox.setMat4("projection", projection);
                shaderLightBox.setMat4("view", view);

                for (unsigned int i = 0; i < pointLights.size(); ++i)
                {
                    glm::mat4 model = glm::mat4();
                    model = glm::translate(model, pointLights[i].getPosition());
                    model = glm::scale(model, glm::vec3(0.125f));
                    shaderLightBox.setMat4("model", model);
                    shaderLightBox.setVec3("lightColor", pointLights[i].getColor());
                    renderCube();
                }

                for (unsigned int i = 0; i < spotLights.size(); ++i)
                {
                    glm::mat4 model = glm::mat4();
                    model = glm::translate(model, spotLights[i].getPosition());
                    glm::quat rotation;
                    rotation = glm::rotation(glm::vec3(0.0f, -1.0f, 0.0f), glm::normalize(spotLights[i].getDirection()));
                    model *= glm::toMat4(rotation);
                    model = glm::scale(model, glm::vec3(0.25f));
                    shaderLightBox.setMat4("model", model);
                    shaderLightBox.setVec3("lightColor", spotLights[i].getColor());
                    renderPyramid();
                }

                // Setup skybox shader and OpenGL for skybox rendering
                skyboxShader.use();
                skyboxShader.setMat4("projection", projection);
                skyboxShader.setMat4("view", glm::mat4(glm::mat3(camera.GetViewMatrix())));
                skyboxShader.setInt("skybox", SKYBOX_TEXTURE_INDEX);

                // Render skybox
                renderSkybox(cubemapTexture);
        */

        // move light position over time
        //lightPos.z = sin(glfwGetTime() * 0.5) * 3.0;



        // // render
        // // ------
        // glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        auto renderPointLightWithShadows = [
            &simpleDepthShader, 
            &pointShadowsShader,
            &view, 
            &projection, 
            &depthMapFBO, 
            &woodTexture, 
            &depthCubemap](
            PointLight& pointLight,
            GLuint& renderingFramebuffer)
        {
            glEnable(GL_DEPTH_TEST);
            glm::vec3 lightPos = pointLight.getPosition();
            // 0. create depth cubemap transformation matrices
            // -----------------------------------------------
            float near_plane = 1.0f;
            float far_plane = 25.0f;
            glm::mat4 shadowProj = glm::perspective(
                glm::radians(90.0f),
                static_cast<float>(POINT_LIGHT_SHADOW_MAP_WIDTH) / static_cast<float>(POINT_LIGHT_SHADOW_MAP_HEIGHT),
                near_plane,
                far_plane
            );
            std::vector<glm::mat4> shadowTransforms;
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
            shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

            // 1. render scene to depth cubemap
            // --------------------------------
            glViewport(0, 0, POINT_LIGHT_SHADOW_MAP_WIDTH, POINT_LIGHT_SHADOW_MAP_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
            simpleDepthShader.use();
            for (unsigned int i = 0; i < 6; ++i)
            {
                simpleDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
            }
            simpleDepthShader.setFloat("far_plane", far_plane);
            simpleDepthShader.setVec3("lightPos", lightPos);
            renderScene(simpleDepthShader);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // 2. render scene as normal 
            // -------------------------
            glViewport(0, 0, screenWidth, screenHeight);
            glBindFramebuffer(GL_FRAMEBUFFER, renderingFramebuffer);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            pointShadowsShader.use();
            pointShadowsShader.setMat4("projection", projection);
            pointShadowsShader.setMat4("view", view);
            // set lighting uniforms
            pointShadowsShader.setVec3("lightPos", lightPos);
            pointShadowsShader.setVec3("viewPos", camera.Position);
            pointShadowsShader.setInt("shadows", shadows); // enable/disable shadows by pressing 'SPACE'
            pointShadowsShader.setFloat("far_plane", far_plane);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
            renderScene(pointShadowsShader);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        };    

        if (!pointLights.empty())
        {
            renderPointLightWithShadows(pointLights[0], blendedFramebuffer);            
        }
        for (auto i = 1; i < pointLights.size(); ++i)
        {
            renderPointLightWithShadows(pointLights[i], lightRenderFramebuffer);
            {
                glBindFramebuffer(GL_FRAMEBUFFER, currentBlendingFramebuffer);
                glDisable(GL_DEPTH_TEST);
                glClear(GL_COLOR_BUFFER_BIT);
                shadowAccumulatorShader.use();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, lightRenderTexture);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, blendedTexture);
                renderScreenQuad();
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            std::swap(currentBlendingFramebuffer, blendedFramebuffer);
            std::swap(currentBlendingTexture, blendedTexture);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, blendedFramebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        
        // blit to default framebuffer.                                           
        glBlitFramebuffer(0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        textureRenderingShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, blendedTexture);
        renderScreenQuad();
        glBindTexture(GL_TEXTURE_2D, 0);       
        
        glEnable(GL_DEPTH_TEST);
        // Setup skybox shader and OpenGL for skybox rendering
        skyboxShader.use();
        skyboxShader.setMat4("projection", projection);
        skyboxShader.setMat4("view", glm::mat4(glm::mat3(camera.GetViewMatrix())));
        skyboxShader.setInt("skybox", SKYBOX_TEXTURE_INDEX);

        // Render skybox
        renderSkybox(cubemapTexture);

        // Input
        processInput(window, lightManager);

        // GLFW: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// renders the 3D scene
// --------------------
void renderScene(const Shader &shader)
{
    // room cube
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(10.0f));
    shader.setMat4("model", model);
    glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
    shader.setInt("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
    renderSeminarCube();
    shader.setInt("reverse_normals", 0); // and of course disable it
    glEnable(GL_CULL_FACE);
    // cubes
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(4.0f, -3.5f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderSeminarCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(2.0f, 3.0f, 1.0));
    model = glm::scale(model, glm::vec3(0.75f));
    shader.setMat4("model", model);
    renderSeminarCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-3.0f, -1.0f, 0.0));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderSeminarCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.5f, 1.0f, 1.5));
    model = glm::scale(model, glm::vec3(0.5f));
    shader.setMat4("model", model);
    renderSeminarCube();
    model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-1.5f, 2.0f, -3.0));
    model = glm::rotate(model, glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
    model = glm::scale(model, glm::vec3(0.75f));
    shader.setMat4("model", model);
    renderSeminarCube();
}

unsigned int skyboxVAO = 0;
unsigned int skyboxVBO = 0;
void renderSkybox(unsigned int cubemapTexture){
    if (skyboxVAO == 0)
    {
        float vertices[] = {
            // positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);   
        // set to defaults     
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glDepthFunc(GL_LEQUAL); // For rendering skybox behind all other objects in scene
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0 + SKYBOX_TEXTURE_INDEX);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthFunc(GL_LESS);// glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {        
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,   // bottom-left
            1.0f,  1.0f, -1.0f,   // top-right
            1.0f, -1.0f, -1.0f,   // bottom-right         
            1.0f,  1.0f, -1.0f,   // top-right
            -1.0f, -1.0f, -1.0f,  // bottom-left
            -1.0f,  1.0f, -1.0f,   // top-left
           // front face
           -1.0f, -1.0f,  1.0f,  // bottom-left
           1.0f, -1.0f,  1.0f,   // bottom-right
           1.0f,  1.0f,  1.0f,   // top-right
           1.0f,  1.0f,  1.0f,  // top-right
           -1.0f,  1.0f,  1.0f,   // top-left
           -1.0f, -1.0f,  1.0f,   // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f,  // top-right
            -1.0f,  1.0f, -1.0f,  // top-left
            -1.0f, -1.0f, -1.0f,  // bottom-left
            -1.0f, -1.0f, -1.0f,  // bottom-left
            -1.0f, -1.0f,  1.0f,  // bottom-right
            -1.0f,  1.0f,  1.0f,  // top-right
            // right face
            1.0f,  1.0f,  1.0f,   // top-left
            1.0f, -1.0f, -1.0f,   // bottom-right
            1.0f,  1.0f, -1.0f,   // top-right         
            1.0f, -1.0f, -1.0f,   // bottom-right
            1.0f,  1.0f,  1.0f,   // top-left
            1.0f, -1.0f,  1.0f,   // bottom-left     
             // bottom face
            -1.0f, -1.0f, -1.0f,  // top-right
            1.0f, -1.0f, -1.0f,  // top-left
            1.0f, -1.0f,  1.0f,   // bottom-left
            1.0f, -1.0f,  1.0f,   // bottom-left
            -1.0f, -1.0f,  1.0f,   // bottom-right
            -1.0f, -1.0f, -1.0f,   // top-right
             // top face
             -1.0f,  1.0f, -1.0f, // top-left
             1.0f,  1.0f , 1.0f,   // bottom-right
             1.0f,  1.0f, -1.0f,  // top-right     
             1.0f,  1.0f,  1.0f,  // bottom-right
             -1.0f,  1.0f, -1.0f, // top-left
             -1.0f,  1.0f,  1.0f,  // bottom-left                                                                         
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindVertexArray(cubeVAO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
        // link vertex attributes        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);  

        // set to defaults      
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int seminarCubeVAO = 0;
unsigned int seminarCubeVBO = 0;
unsigned int seminarCubeTexture = 0;

void renderSeminarCube()
{
    if (seminarCubeVAO == 0)
    {
        float vertices[] = 
        {
             // coordinates            // normals               // texture coordinates
             0.5f,  0.5f, -0.5f, /**/  0.0f,  0.0f, -1.0f, /**/ 1.0f,  1.0f,
             0.5f, -0.5f, -0.5f, /**/  0.0f,  0.0f, -1.0f, /**/ 1.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, /**/  0.0f,  0.0f, -1.0f, /**/ 0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, /**/  0.0f,  0.0f, -1.0f, /**/ 0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, /**/  0.0f,  0.0f, -1.0f, /**/ 0.0f,  1.0f,
             0.5f,  0.5f, -0.5f, /**/  0.0f,  0.0f, -1.0f, /**/ 1.0f,  1.0f,
                                 /**/                      /**/
            -0.5f, -0.5f,  0.5f, /**/  0.0f,  0.0f,  1.0f, /**/ 0.0f,  0.0f,
             0.5f, -0.5f,  0.5f, /**/  0.0f,  0.0f,  1.0f, /**/ 1.0f,  0.0f,
             0.5f,  0.5f,  0.5f, /**/  0.0f,  0.0f,  1.0f, /**/ 1.0f,  1.0f,
             0.5f,  0.5f,  0.5f, /**/  0.0f,  0.0f,  1.0f, /**/ 1.0f,  1.0f,
            -0.5f,  0.5f,  0.5f, /**/  0.0f,  0.0f,  1.0f, /**/ 0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, /**/  0.0f,  0.0f,  1.0f, /**/ 0.0f,  0.0f,
                                 /**/                      /**/
            -0.5f,  0.5f,  0.5f, /**/ -1.0f,  0.0f,  0.0f, /**/ 1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, /**/ -1.0f,  0.0f,  0.0f, /**/ 1.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, /**/ -1.0f,  0.0f,  0.0f, /**/ 0.0f,  1.0f,
            -0.5f, -0.5f, -0.5f, /**/ -1.0f,  0.0f,  0.0f, /**/ 0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f, /**/ -1.0f,  0.0f,  0.0f, /**/ 0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, /**/ -1.0f,  0.0f,  0.0f, /**/ 1.0f,  0.0f,
                                 /**/                      /**/
             0.5f,  0.5f, -0.5f, /**/  1.0f,  0.0f,  0.0f, /**/ 1.0f,  1.0f,
             0.5f,  0.5f,  0.5f, /**/  1.0f,  0.0f,  0.0f, /**/ 1.0f,  0.0f,
             0.5f, -0.5f,  0.5f, /**/  1.0f,  0.0f,  0.0f, /**/ 0.0f,  0.0f,
             0.5f, -0.5f,  0.5f, /**/  1.0f,  0.0f,  0.0f, /**/ 0.0f,  0.0f,
             0.5f, -0.5f, -0.5f, /**/  1.0f,  0.0f,  0.0f, /**/ 0.0f,  1.0f,
             0.5f,  0.5f, -0.5f, /**/  1.0f,  0.0f,  0.0f, /**/ 1.0f,  1.0f,
                                 /**/                      /**/
            -0.5f, -0.5f, -0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 0.0f,  1.0f,
             0.5f, -0.5f, -0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 1.0f,  1.0f,
             0.5f, -0.5f,  0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 1.0f,  0.0f,
             0.5f, -0.5f,  0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 0.0f,  1.0f,
                                 /**/                      /**/
            -0.5f,  0.5f, -0.5f, /**/  0.0f,  1.0f,  0.0f, /**/ 0.0f,  1.0f,
            -0.5f,  0.5f,  0.5f, /**/  0.0f,  1.0f,  0.0f, /**/ 0.0f,  0.0f,
             0.5f,  0.5f,  0.5f, /**/  0.0f,  1.0f,  0.0f, /**/ 1.0f,  0.0f,
             0.5f,  0.5f,  0.5f, /**/  0.0f,  1.0f,  0.0f, /**/ 1.0f,  0.0f,
             0.5f,  0.5f, -0.5f, /**/  0.0f,  1.0f,  0.0f, /**/ 1.0f,  1.0f,
            -0.5f,  0.5f, -0.5f, /**/  0.0f,  1.0f,  0.0f, /**/ 0.0f,  1.0f
        };
        // first, configure the cube's VAO (and VBO)
        glGenVertexArrays(1, &seminarCubeVAO);
        glGenBuffers(1, &seminarCubeVBO);

        glBindBuffer(GL_ARRAY_BUFFER, seminarCubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindVertexArray(seminarCubeVAO);

        const int stride = 8;

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // texture attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        //seminarCubeTexture = loadTexture("data/textures/cube/container.png");
    }
    // render Cube
    glBindVertexArray(seminarCubeVAO);
    //glBindTexture(GL_TEXTURE_2D, seminarCubeTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int pyramidVAO = 0;
unsigned int pyramidVBO = 0;

void renderPyramid()
{
    if (pyramidVAO == 0)
    {
        float vertices[] =
        {
            //front face
             0.5f, -0.5f,  0.5f,//2
             0.0f,  0.5f,  0.0f,//5
            -0.5f, -0.5f,  0.5f,//1              
            //rigth face
             0.5f, -0.5f, -0.5f,//3
             0.0f,  0.5f,  0.0f,//5
             0.5f, -0.5f,  0.5f,
            //left face
            -0.5f, -0.5f,  0.5f,
             0.0f,  0.5f,  0.0f,
            -0.5f, -0.5f, -0.5f,//4
            //back face
            -0.5f, -0.5f, -0.5f,
             0.0f,  0.5f,  0.0f,
             0.5f, -0.5f, -0.5f,
            //bottom face
             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,
            
             0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f, -0.5f, -0.5f,
        };

        glGenVertexArrays(1, &pyramidVAO);
        glGenBuffers(1, &pyramidVBO);
        glBindVertexArray(pyramidVAO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);        
    }
    glBindVertexArray(pyramidVAO);
    glDrawArrays(GL_TRIANGLES, 0, 18);
    glBindVertexArray(0);
}


GLuint screenQuadVAO = 0;
GLuint screenQuadVBO = 0;
void renderScreenQuad()
{
    if (screenQuadVAO == 0)
    {
        float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
        };
        glGenVertexArrays(1, &screenQuadVAO);
        glGenBuffers(1, &screenQuadVBO);
        glBindVertexArray(screenQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    }
    glBindVertexArray(screenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, LightManager& lightManager)
{
    //glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // camera control
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(CameraMovement::RIGHT, deltaTime);      
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
    screenWidth = width;
    screenHeight = height;
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    void* obj = glfwGetWindowUserPointer(window);
    LightManager* lightManager = static_cast<LightManager*>(obj);
    if (lightManager)            
        lightManager->key_callback(window, key, scancode, action, mods);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !shadowsKeyPressed)
    {
        shadows = !shadows;
        shadowsKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        shadowsKeyPressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !renderSecondLightKeyPressed)
    {
        renderSecondLight = !renderSecondLight;
        renderSecondLightKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE)
    {
        renderSecondLightKeyPressed = false;
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}  

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
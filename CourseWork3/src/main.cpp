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
#include "Shader.h"
#include "Camera.h"
#include "SceneLoader.h"
#include "LightManager.h"
#include "Model.h"
#include "Object.h"
#include "Aliases.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <array>
#include <random>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, LightManager& lightManager);
void renderCube();
void renderSeminarCube();
void renderSeminarPyramid();
void renderSeminarCylinder();
void renderPhotoCube();
void renderPyramid();
void renderQuad();
void renderSkybox(unsigned int cubemapTexture);
unsigned int loadCubemap(std::vector<std::string> faces);
unsigned int loadTexture(char const * path);
void ConfigureBuffers(GLuint& VAO, GLuint& VBO, std::vector<float>& vertices);

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
DirectionalLights dirLights;
PointLights pointLights;
SpotLights spotLights;
Objects objects;
Models models; 

vector<std::string> faces
{
    "skybox/right.jpg",
    "skybox/left.jpg",
    "skybox/top.jpg",
    "skybox/bottom.jpg",
    "skybox/front.jpg",
    "skybox/back.jpg"
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
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Physically based rendering program", NULL, NULL);
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
    Shader basicLighting("shaders/basic_lighting.vert", "shaders/basic_lighting.frag");
	Shader photoCubeShader("shaders/photoCube.vert", "shaders/photoCube.frag");
    
    // Load scene   
    SceneLoader sceneLoader;
    sceneLoader.loadScene("LightData.txt", "ModelData.txt", dirLights, pointLights, spotLights, models, objects);             

    // Load skybox
    unsigned int cubemapTexture = loadCubemap(faces); 

    // Setup light manager and key callbacks for lights controls
    LightManager lightManager(pointLights, spotLights);
    glfwSetKeyCallback(window, key_callback);
    glfwSetWindowUserPointer(window, &lightManager);

    // Configure global OpenGL state: perform depth test, don't render faces, which don't look at user    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);           

    // Set shader in use
    shader.use();        

    // Setup point lights
    PointLights::size_type pointLightsNumber = min(MAX_NUMBER_OF_POINT_LIGHTS, pointLights.size());   
    shader.setInt("pointLightsNumber", pointLightsNumber);   
    for (PointLights::size_type i = 0; i < pointLights.size(); ++i)
    {
        shader.setVec3( "pointLights[" + to_string(i) + "].position", pointLights[i].getPosition());
        shader.setVec3( "pointLights[" + to_string(i) + "].color", pointLights[i].getColor());        
        shader.setFloat("pointLights[" + to_string(i) + "].constant", pointLights[i].getConstant());
        shader.setFloat("pointLights[" + to_string(i) + "].linear", pointLights[i].getLinear());
        shader.setFloat("pointLights[" + to_string(i) + "].quadratic", pointLights[i].getQuadratic());
    }    
    
    // Setup directional lights
    DirectionalLights::size_type dirLightsNumber = min(MAX_NUMBER_OF_DIRECTIONAL_LIGHTS, dirLights.size());
    shader.setInt("dirLightsNumber", dirLightsNumber);
    for (DirectionalLights::size_type i = 0; i < dirLights.size(); ++i)
    {
        shader.setVec3("dirLights[" + to_string(i) + "].color", dirLights[i].getColor());
        shader.setVec3("dirLights[" + to_string(i) + "].direction", dirLights[i].getDirection());
    }

    // Setup spot lights
    SpotLights::size_type spotLightsNumber = min(MAX_NUMBER_OF_SPOT_LIGHTS, spotLights.size());
    shader.setInt("spotLightsNumber", spotLightsNumber);
    for(SpotLights::size_type i = 0; i < spotLights.size(); ++i)
    {        
        shader.setVec3( "spotLights[" + to_string(i) + "].position", spotLights[i].getPosition());
        shader.setVec3( "spotLights[" + to_string(i) + "].color", spotLights[i].getColor()); 
        shader.setVec3( "spotLights[" + to_string(i) + "].direction", spotLights[i].getDirection());       
        shader.setFloat("spotLights[" + to_string(i) + "].constant", spotLights[i].getConstant());
        shader.setFloat("spotLights[" + to_string(i) + "].linear", spotLights[i].getLinear());
        shader.setFloat("spotLights[" + to_string(i) + "].quadratic", spotLights[i].getQuadratic());
        shader.setFloat("spotLights[" + to_string(i) + "].cutOff", glm::cos(spotLights[i].getCutOffInRadians()));
        shader.setFloat("spotLights[" + to_string(i) + "].outerCutOff", glm::cos(spotLights[i].getOuterCutOffInRadians()));
    }

    basicLighting.use();
    basicLighting.setInt("diffuse", 0);

	photoCubeShader.use();
	photoCubeShader.setInt("firstTexture", 0);
	photoCubeShader.setInt("secondTexture", 1);

    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    // Render loop    
    while (!glfwWindowShouldClose(window))
    {
        // Per-frame time logic        
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        lightManager.updateDeltaTime(deltaTime);

        // Render        
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
        // Calculate view and projection matrix for current state and position of camera
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();                  

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

        // Update point lights positions
        for (PointLights::size_type i = 0; i < pointLights.size(); ++i)                              
            shader.setVec3("pointLights[" + to_string(i) + "].position", pointLights[i].getPosition());                            

        // Update spot lights positions
        for (SpotLights::size_type i = 0; i < spotLights.size(); ++i)        
            shader.setVec3("spotLights[" + to_string(i) + "].position", spotLights[i].getPosition());            
        

        // render scene contents
        basicLighting.use();
        basicLighting.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
        basicLighting.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        basicLighting.setVec3("lightPos", lightPos);
        basicLighting.setVec3("viewPos", camera.Position);
        basicLighting.setMat4("projection", projection);
        basicLighting.setMat4("view", view);
        
        {   // block allows to define matrix with name 'model'
            glm::mat4 model = glm::mat4(1.0f);
            basicLighting.setMat4("model", model);
            // render ground
            renderQuad();
        }

        { // Seminar scene contents
            // Boxes
            glm::mat4 model = glm::mat4{};
            glm::mat4 moveCubeUp = glm::translate(model, glm::vec3(0.5f, 0.5f, -0.6f));
            basicLighting.setMat4("model", moveCubeUp);
            renderSeminarCube();

            glm::mat4 moveSecondCube = glm::translate(model, glm::vec3(0.7f, 0.5f, 0.8f));
            basicLighting.setMat4("model", moveSecondCube);
            renderSeminarCube();

            glm::mat4 moveThirdCube = glm::translate(model, glm::vec3(-0.6f, 0.5f, -0.4f));
            basicLighting.setMat4("model", moveThirdCube);
            renderSeminarCube();

            glm::mat4 moveFourthCube = glm::translate(model, glm::vec3(-0.7f, 0.5f, 0.7f));
            basicLighting.setMat4("model", moveFourthCube);
            renderSeminarCube();

            glm::mat4 moveFifthCube = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0f));
            basicLighting.setMat4("model", moveFifthCube);
            renderSeminarCube();

            glm::mat4 pyramidModel = glm::mat4{};
            pyramidModel = glm::translate(pyramidModel, glm::vec3(0.0f, 2.75, 0.0f));

            static float dynamicRotationAngle = 0;
            static float currentQuarter = 0;
            const auto cylinderNumber = 8;
            const float sphereRadius = 3;
            
            std::random_device randomDevice;  //Will be used to obtain a seed for the random number engine
            std::mt19937 generator(randomDevice()); //Standard mersenne_twister_engine seeded with rd()
            std::uniform_real_distribution<> distribution(-glm::quarter_pi<float>(), glm::quarter_pi<float>());
            
            static std::array<glm::vec3, cylinderNumber> currentCylinderPosition;
            static std::array<glm::vec3, cylinderNumber> destinationCylinderPosition;
            static std::array<float, cylinderNumber> yVelocities;
			static std::array<float, cylinderNumber> currentCylinderHeight;
			static bool isHeightInitialized = false;
			if (!isHeightInitialized)
			{
				isHeightInitialized = true;
				for (auto i = 0; i < cylinderNumber; ++i)
				{
					currentCylinderHeight[i] = 0.0f;
				}
			}
  
            dynamicRotationAngle += glm::quarter_pi<float>() * deltaTime;
            if (dynamicRotationAngle - currentQuarter > glm::half_pi<float>())
            {
                if (currentQuarter == 0.0f)
                {
                    currentQuarter = glm::half_pi<float>();
                }
                else if (currentQuarter == glm::half_pi<float>())
                {
                    currentQuarter = glm::pi<float>();
                }
                else if (currentQuarter == glm::pi<float>())
                {
                    currentQuarter = glm::three_over_two_pi<float>();
                }
                else if (currentQuarter == glm::three_over_two_pi<float>())
                {
                    currentQuarter = 0.0f;
                    dynamicRotationAngle = 0.0f;
                }
                for (auto i = 0; i < cylinderNumber; ++i)
                {
                    const auto destinationAngle = currentQuarter + glm::half_pi<float>();
                    const auto heightAngle = distribution(generator);
                    const auto newPosition = glm::vec3(glm::cos(destinationAngle) * sphereRadius, sphereRadius * glm::sin(heightAngle), glm::sin(destinationAngle));
                    
                    const auto rotationAngle = glm::dot(currentCylinderPosition[i], newPosition) / (sphereRadius * sphereRadius);
                    yVelocities[i] = (newPosition.y - currentCylinderPosition[i].y) / 4.0f;
                }
            }

            // Mason all-seeing eye
            pyramidModel = glm::rotate(pyramidModel, dynamicRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
            basicLighting.setMat4("model", pyramidModel);
            renderSeminarPyramid();

            // Circle of cylinder
            
            for (auto i = 0; i < cylinderNumber; ++i)
            {
                auto cylinderModel = glm::mat4{};
				currentCylinderHeight[i] += yVelocities[i] * deltaTime;
                const auto startRotationAngle = i * glm::two_pi<float>() / static_cast<float>(cylinderNumber);
                // Set rotation around global Y axis
                cylinderModel = glm::rotate(cylinderModel, startRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                cylinderModel = glm::rotate(cylinderModel, -dynamicRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
                // Translate cylinder by the height and radius of circle of cylinders
                cylinderModel = glm::translate(cylinderModel, glm::vec3(0.0f, currentCylinderHeight[i] + 2.0f , 3.0f));
                // Put object on the side and scale it
                cylinderModel = glm::rotate(cylinderModel, glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
                cylinderModel = glm::scale(cylinderModel, glm::vec3(0.5f, 1.0f, 0.5f));
                // Rotate around object's Y axis (simulate wheel movement)
                cylinderModel = glm::rotate(cylinderModel, -dynamicRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

				currentCylinderPosition[i] = glm::vec3(
					glm::cos(startRotationAngle - dynamicRotationAngle),
					currentCylinderHeight[i],
					glm::sin(startRotationAngle - dynamicRotationAngle)
				);
                basicLighting.setMat4("model", cylinderModel);
                renderSeminarCylinder();
            }

			// Photo cube
            static float alpha = 0;
            static bool isAlphaIncreasing = true;
			const static float fullTextureSwapTime = 2.0; // time in seconds
            if (isAlphaIncreasing)
            {
                alpha += deltaTime / fullTextureSwapTime;
                if (alpha > 1.0f)
                {
                    //alpha = 2.0f - alpha; // We do not lose accumulated value
					glm::clamp(alpha, 0.0f, 1.0f);
                    isAlphaIncreasing = false;
                }
            }
            else
            {
                alpha -= deltaTime / fullTextureSwapTime;
                if (alpha < 0.0f)
                {
                    //alpha = -alpha; // We do not lose accumulated value
					glm::clamp(alpha, 0.0f, 1.0f);
                    isAlphaIncreasing = true;
                }
            }

			photoCubeShader.use();
			photoCubeShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
			photoCubeShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
			photoCubeShader.setVec3("lightPos", lightPos);
			photoCubeShader.setVec3("viewPos", camera.Position);
			photoCubeShader.setMat4("projection", projection);
			photoCubeShader.setMat4("view", view);
            photoCubeShader.setFloat("alpha", alpha);
            auto photoCubeModel = glm::mat4{};
			photoCubeModel = glm::translate(photoCubeModel, glm::vec3(0.f, 5.f, 0.f));
            photoCubeShader.setMat4("model", photoCubeModel);
            renderPhotoCube();

        } // End of seminar scene contents

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

        // basic lighting cube
        glm::mat4 model = glm::mat4();
        model = glm::translate(model, glm::vec3(lightPos));
        model = glm::scale(model, glm::vec3(0.125f));
        shaderLightBox.setMat4("model", model);
        shaderLightBox.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        renderCube();

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

        // Input
        processInput(window, lightManager);

        // GLFW: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
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

unsigned int seminarPyramidVAO = 0;
unsigned int seminarPyramidVBO = 0;
unsigned int seminarPyramidTexture = 0;

void renderSeminarPyramid()
{
    if (seminarPyramidVAO == 0)
    {
        float vertices[] =
        {
            // View from up
            // 4---------3
            // | \     / |
            // |  \   /  |
            // |    5    |
            // |  /   \  |
            // | /     \ |
            // 1---------2
            
            // front face              // normals               // texture coordinates
             0.5f, -0.5f,  0.5f, /**/  0.0f,  0.7f,  0.7f, /**/ 1.0f, 0.0f, //2
             0.0f,  0.5f,  0.0f, /**/  0.0f,  0.7f,  0.7f, /**/ 0.5f, 1.0f, //5
            -0.5f, -0.5f,  0.5f, /**/  0.0f,  0.7f,  0.7f, /**/ 0.0f, 0.0f, //1              
            // rigth face        /**/                      /**/
             0.5f, -0.5f, -0.5f, /**/  0.7f,  0.7f,  0.0f, /**/ 1.0f, 0.0f, //3
             0.0f,  0.5f,  0.0f, /**/  0.7f,  0.7f,  0.0f, /**/ 0.5f, 1.0f, //5
             0.5f, -0.5f,  0.5f, /**/  0.7f,  0.7f,  0.0f, /**/ 0.0f, 0.0f,
            // left face         /**/                      /**/
            -0.5f, -0.5f,  0.5f, /**/ -0.7f,  0.7f,  0.0f, /**/ 1.0f, 0.0f,
             0.0f,  0.5f,  0.0f, /**/ -0.7f,  0.7f,  0.0f, /**/ 0.5f, 1.0f,
            -0.5f, -0.5f, -0.5f, /**/ -0.7f,  0.7f,  0.0f, /**/ 0.0f, 0.0f, //4
            // back face         /**/                      /**/
            -0.5f, -0.5f, -0.5f, /**/  0.0f,  0.7f, -0.7f, /**/ 1.0f, 0.0f,
             0.0f,  0.5f,  0.0f, /**/  0.0f,  0.7f, -0.7f, /**/ 0.5f, 1.0f,
             0.5f, -0.5f, -0.5f, /**/  0.0f,  0.7f, -0.7f, /**/ 0.0f, 0.0f,
            // bottom face       /**/                      /**/
             0.5f, -0.5f, -0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 1.0f, 0.0f, //3
             0.5f, -0.5f,  0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 1.0f, 1.0f, //2
            -0.5f, -0.5f,  0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 0.0f, 1.0f, //1
                                 /**/                      /**/
             0.5f, -0.5f, -0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 1.0f, 0.0f, //3
            -0.5f, -0.5f,  0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 0.0f, 1.0f, //1
            -0.5f, -0.5f, -0.5f, /**/  0.0f, -1.0f,  0.0f, /**/ 0.0f, 0.0f, //4
        };

        glGenVertexArrays(1, &seminarPyramidVAO);
        glGenBuffers(1, &seminarPyramidVBO);
        glBindVertexArray(seminarPyramidVAO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, seminarPyramidVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

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

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        seminarPyramidTexture = loadTexture("textures/pyramid/mason1.jpg");
    }
    glBindVertexArray(seminarPyramidVAO);
    glBindTexture(GL_TEXTURE_2D, seminarPyramidTexture);
    glDrawArrays(GL_TRIANGLES, 0, 18);
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

        seminarCubeTexture = loadTexture("textures/cube/container.png");
    }
    // render Cube
    glBindVertexArray(seminarCubeVAO);
    glBindTexture(GL_TEXTURE_2D, seminarCubeTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

unsigned int firstPhotoCubeVAO = 0;
unsigned int firsthotoCubeVBO = 0;
unsigned int secondPhotoCubeVAO = 0;
unsigned int secondhotoCubeVBO = 0;
unsigned int thirdPhotoCubeVAO = 0;
unsigned int thirdhotoCubeVBO = 0;
unsigned int PhotoCubeTexture1 = 0;
unsigned int PhotoCubeTexture2 = 0;
unsigned int PhotoCubeTexture3 = 0;
unsigned int PhotoCubeTexture4 = 0;
unsigned int PhotoCubeTexture5 = 0;
unsigned int PhotoCubeTexture6 = 0;

void renderPhotoCube()
{
    if (firstPhotoCubeVAO == 0)
    {
        std::vector<float> firstVertices 
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
        };
        std::vector<float> secondVertices
        {
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
        };
        std::vector<float> thirdVertices
        {
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

        ConfigureBuffers(firstPhotoCubeVAO, firsthotoCubeVBO, firstVertices);
        ConfigureBuffers(secondPhotoCubeVAO, secondhotoCubeVBO, secondVertices);
        ConfigureBuffers(thirdPhotoCubeVAO, thirdhotoCubeVBO, thirdVertices);

        PhotoCubeTexture1 = loadTexture("textures/photoCube/1.png");
        PhotoCubeTexture2 = loadTexture("textures/photoCube/2.png");
        PhotoCubeTexture3 = loadTexture("textures/photoCube/3.png");
        PhotoCubeTexture4 = loadTexture("textures/photoCube/4.png");
        PhotoCubeTexture5 = loadTexture("textures/photoCube/5.png");
        PhotoCubeTexture6 = loadTexture("textures/photoCube/6.png");
    }
    // render Cube
    glBindVertexArray(firstPhotoCubeVAO);
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, PhotoCubeTexture1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, PhotoCubeTexture2);
    glDrawArrays(GL_TRIANGLES, 0, 12);

	glBindVertexArray(secondPhotoCubeVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, PhotoCubeTexture3);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, PhotoCubeTexture4);
	glDrawArrays(GL_TRIANGLES, 0, 12);

	glBindVertexArray(thirdPhotoCubeVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, PhotoCubeTexture5);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, PhotoCubeTexture6);
	glDrawArrays(GL_TRIANGLES, 0, 12);

	// reset state to default
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}

std::vector<float> GenerateCircle(glm::vec3 center, float radius, float height, int segmentsNumber, bool isDown)
{
    std::vector<float> vertices;
    float stepAngle = glm::two_pi<float>() / segmentsNumber;
    for (auto i = 0; i < segmentsNumber; ++i)
    {
        vertices.push_back(center.x);
        vertices.push_back(center.y + (isDown ? -height / 2.0f : height / 2.0f));
        vertices.push_back(center.z);
        vertices.push_back(0);
        vertices.push_back(isDown ? -1 : 1);
        vertices.push_back(0);
        vertices.push_back(0.5);
        vertices.push_back(0.5);

        if (isDown)
        {
            vertices.push_back(center.x + glm::cos(stepAngle * i) * radius);
            vertices.push_back(center.y + (isDown ? -height / 2.0f : height / 2.0f));
            vertices.push_back(center.z + glm::sin(stepAngle * i) * radius);
            vertices.push_back(0);
            vertices.push_back(isDown ? -1 : 1);
            vertices.push_back(0);
            vertices.push_back(0.5 + 0.5 * glm::cos(stepAngle * i));
            vertices.push_back(0.5 + 0.5 * glm::sin(stepAngle * i));

            vertices.push_back(center.x + glm::cos(stepAngle * (i + 1)) * radius);
            vertices.push_back(center.y + (isDown ? -height / 2.0f : height / 2.0f));
            vertices.push_back(center.z + glm::sin(stepAngle * (i + 1)) * radius);
            vertices.push_back(0);
            vertices.push_back(isDown ? -1 : 1);
            vertices.push_back(0);
            vertices.push_back(0.5 + 0.5 * glm::cos(stepAngle * (i + 1)));
            vertices.push_back(0.5 + 0.5 * glm::sin(stepAngle * (i + 1)));
        }
        else
        {
            vertices.push_back(center.x + glm::cos(stepAngle * (i + 1)) * radius);
            vertices.push_back(center.y + (isDown ? -height / 2.0f : height / 2.0f));
            vertices.push_back(center.z + glm::sin(stepAngle * (i + 1)) * radius);
            vertices.push_back(0);
            vertices.push_back(isDown ? -1 : 1);
            vertices.push_back(0);
            vertices.push_back(0.5 + 0.5 * glm::cos(stepAngle * (i + 1)));
            vertices.push_back(0.5 + 0.5 * glm::sin(stepAngle * (i + 1)));

            vertices.push_back(center.x + glm::cos(stepAngle * i) * radius);
            vertices.push_back(center.y + (isDown ? -height / 2.0f : height / 2.0f));
            vertices.push_back(center.z + glm::sin(stepAngle * i) * radius);
            vertices.push_back(0);
            vertices.push_back(isDown ? -1 : 1);
            vertices.push_back(0);
            vertices.push_back(0.5 + 0.5 * glm::cos(stepAngle * i));
            vertices.push_back(0.5 + 0.5 * glm::sin(stepAngle * i));
        }
        
    }
    return vertices;
}

std::array<float, 8> GenerateVertex(glm::vec3& center, float radius, float height, float segmentHeight, float segmentAngle)
{
    std::array<float, 8> vertex;
    // vertex coordinates
    vertex[0] = center.x + glm::cos(segmentAngle) * radius;
    vertex[1] = center.y + segmentHeight - height / 2.0f;
    vertex[2] = center.z + glm::sin(segmentAngle) * radius;
    // normal
    vertex[3] = glm::cos(segmentAngle);
    vertex[4] = 0;
    vertex[5] = glm::sin(segmentAngle);
    // texture coordinates
    vertex[6] = segmentAngle / glm::two_pi<float>();
    vertex[7] = segmentHeight / height;
    
    return vertex;
}

std::vector<float> GenerateBorder(glm::vec3 center, float radius, int segmentsNumber, float height, int heightSegmentsNumber)
{
    std::vector<float> vertices;
    float stepAngle = glm::two_pi<float>() / segmentsNumber;
    float segmentHeight = height / heightSegmentsNumber;
    for (auto i = 0; i < heightSegmentsNumber; ++i)
    {
        for (auto j = 0; j < segmentsNumber; ++j)
        {
            auto vertex = GenerateVertex(center, radius, height, i * segmentHeight, j * stepAngle);
            vertices.insert(vertices.end(), vertex.begin(), vertex.end());

            vertex = GenerateVertex(center, radius, height, (i + 1) * segmentHeight, j * stepAngle);
            vertices.insert(vertices.end(), vertex.begin(), vertex.end());

            vertex = GenerateVertex(center, radius, height, i * segmentHeight, (j + 1) * stepAngle);
            vertices.insert(vertices.end(), vertex.begin(), vertex.end());

            
            vertex = GenerateVertex(center, radius, height, i * segmentHeight, (j + 1) * stepAngle);
            vertices.insert(vertices.end(), vertex.begin(), vertex.end());

            vertex = GenerateVertex(center, radius, height, (i + 1) * segmentHeight, j * stepAngle);
            vertices.insert(vertices.end(), vertex.begin(), vertex.end());

            vertex = GenerateVertex(center, radius, height, (i + 1) * segmentHeight, (j + 1) * stepAngle);
            vertices.insert(vertices.end(), vertex.begin(), vertex.end());
        }
    }
    return vertices;
}

void ConfigureBuffers(GLuint& VAO, GLuint& VBO, std::vector<float>& vertices)
{
    assert(vertices.size() % 8 == 0);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glBindVertexArray(VAO);

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

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

GLuint downCircleVAO = 0;
GLuint downCircleVBO = 0;
GLuint upCircleVAO = 0;
GLuint upCircleVBO = 0;
GLuint circleTexture = 0;

GLuint borderVAO = 0;
GLuint borderVBO = 0;
GLuint borderTexture = 0;

void renderSeminarCylinder()
{
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    float radius = 1.0f;
    float height = 1.0f;
    int heightSegmentsNumber = 2;
    int angleSegmentsNumber = 18;

    if (downCircleVAO == 0)
    {
        auto downCircleVertices = GenerateCircle(center, radius, height, angleSegmentsNumber, true);
        auto upCircleVertices = GenerateCircle(center, radius, height, angleSegmentsNumber, false);
        auto borderVertices = GenerateBorder(center, radius, angleSegmentsNumber, height, heightSegmentsNumber);
        
        ConfigureBuffers(downCircleVAO, downCircleVBO, downCircleVertices);
        ConfigureBuffers(upCircleVAO, upCircleVBO, upCircleVertices);
        ConfigureBuffers(borderVAO, borderVBO, borderVertices);

        circleTexture = loadTexture("textures/cylinder/awesomeface.png");
        borderTexture = loadTexture("textures/cylinder/barrel1.jpg");
    }

    glBindTexture(GL_TEXTURE_2D, circleTexture);
    glBindVertexArray(downCircleVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3 * angleSegmentsNumber);
    glBindVertexArray(0);
    
    glBindVertexArray(upCircleVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3 * angleSegmentsNumber);
    glBindVertexArray(0);
    
    glBindTexture(GL_TEXTURE_2D, borderTexture);
    glBindVertexArray(borderVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6 * heightSegmentsNumber * angleSegmentsNumber);
    glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
unsigned int quadTexture = 0;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions          //normals                       // texture Coords
            -1000.0f, 0.0f, /**/  1000.0f, 0.0f, 1.0f, 0.0f, /**/    0.0f, 1000.0f,
             1000.0f, 0.0f, /**/  1000.0f, 0.0f, 1.0f, 0.0f, /**/ 1000.0f, 1000.0f,
            -1000.0f, 0.0f, /**/ -1000.0f, 0.0f, 1.0f, 0.0f, /**/    0.0f,    0.0f,
                            /**/                             /**/
             1000.0f, 0.0f, /**/  1000.0f, 0.0f, 1.0f, 0.0f, /**/ 1000.0f, 1000.0f,
             1000.0f, 0.0f, /**/ -1000.0f, 0.0f, 1.0f, 0.0f, /**/ 1000.0f,    0.0f,
            -1000.0f, 0.0f, /**/ -1000.0f, 0.0f, 1.0f, 0.0f, /**/    0.0f,    0.0f,
        };

        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

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

        quadTexture = loadTexture("textures/cube/container.png");
    }
    glBindTexture(GL_TEXTURE_2D, quadTexture);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
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
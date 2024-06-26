#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>


void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(char const * path);

unsigned int loadCubemap(vector<std::string> faces);



// settings
const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 900;
bool blinn = false;
bool blinnKeyPressed = false;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;


// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;

    glm::vec3 hoverPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 waspPosition = glm::vec3(1.0f, 2.0f, 1.0f);

    float hoverScale = 1.0f;
    float waspScale = 1.0f;

    PointLight pointLight;
    SpotLight spotLight;

    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

//void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
//    IMGUI_CHECKVERSION();
//    ImGui::CreateContext();
//    ImGuiIO &io = ImGui::GetIO();
//    (void) io;



//    ImGui_ImplGlfw_InitForOpenGL(window, true);
//    ImGui_ImplOpenGL3_Init("#version 330 core");

    programState->camera.MovementSpeed = 10.0f;

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    Shader ourShader("resources/shaders/model_lighting.vs", "resources/shaders/model_lighting.fs");
    Shader galaxyShader("resources/shaders/galaxy_skybox.vs", "resources/shaders/galaxy_skybox.fs");
    Shader pyramidShader("resources/shaders/pyramid_shader.vs", "resources/shaders/pyramid_shader.fs");


    float pyramidBaseVertices[] = {
            //prvi trougao baze
            0.5f, 0.0f, -0.5f, 1.0f, 1.0f,
            0.5f, 0.0f, 0.5f, 1.0f, 0.0f,
            -0.5f, 0.0f, 0.5f, 0.0f, 0.0f,
            //drugi trougao baze
            -0.5f, 0.0f, 0.5f, 0.0f, 0.0f,
            -0.5f, 0.0f, -0.5f, 0.0f, 1.0f,
            0.5f, 0.0f, -0.5f, 1.0f, 1.0f,
            //strane piramide
            -0.5f, 0.0f, 0.5f, 0.0f, 0.0f,
            0.5f, 0.0f, 0.5f, 1.0f, 0.0f,
            0.0f, sqrt(2.0f)/2.0f, 0.0f, 0.5f, 1.0f,

            0.5f, 0.0f, 0.5f, 0.0f, 0.0f,
            0.5f, 0.0f, -0.5f, 1.0f, 0.0f,
            0.0f, sqrt(2.0f)/2.0f, 0.0f, 0.5f, 1.0f,

            0.5f, 0.0f, -0.5f, 0.0f, 0.0f,
            -0.5f, 0.0f, -0.5f, 1.0f, 0.0f,
            0.0f, sqrt(2.0f)/2.0f, 0.0f, 0.5f, 1.0f,

            -0.5f, 0.0f, -0.5f, 0.0f, 0.0f,
            -0.5f, 0.0f, 0.5f, 1.0f, 0.0f,
            0.0f, sqrt(2.0f)/2.0f, 0.0f, 0.5f, 1.0f
    };


    float skyboxVertices[] = {
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
    //pyramid buffers
    unsigned int pyramidVBO, pyramidVAO;
    glGenVertexArrays(1, &pyramidVAO);
    glGenBuffers(1, &pyramidVBO);
    glBindVertexArray(pyramidVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidBaseVertices), &pyramidBaseVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    //skybox buffers
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    //load pyramid sides texture
    unsigned int pyramidTexture = loadTexture(FileSystem::getPath("resources/textures/honeycomb.jpg").c_str());

    //vector of cubemap component paths
    vector<std::string> cube_sides
            {
                    FileSystem::getPath("resources/textures/galaxy_skybox/right.jpg").c_str(),
                    FileSystem::getPath("resources/textures/galaxy_skybox/left.jpg").c_str(),
                    FileSystem::getPath("resources/textures/galaxy_skybox/bottom.jpg").c_str(),
                    FileSystem::getPath("resources/textures/galaxy_skybox/top.jpg").c_str(),
                    FileSystem::getPath("resources/textures/galaxy_skybox/front.jpg").c_str(),
                    FileSystem::getPath("resources/textures/galaxy_skybox/back.jpg").c_str()
            };

    //load maps

    //stbi_set_flip_vertically_on_load(false);
    unsigned int cubemapTexture = loadCubemap(cube_sides);
    //stbi_set_flip_vertically_on_load(true);



    //configurating shaders

    pyramidShader.use();
    pyramidShader.setInt("texture1", 0);

    galaxyShader.use();
    galaxyShader.setInt("skybox", 0);






    // load models
    // -----------
    //stbi_set_flip_vertically_on_load(false);
    Model sunflowerModel("resources/objects/bee_hover/untitled.obj");
    sunflowerModel.SetShaderTextureNamePrefix("material.");
    //stbi_set_flip_vertically_on_load(true);

    stbi_set_flip_vertically_on_load(false);
    Model wasp("resources/objects/robo_wasp/untitled.obj");
    wasp.SetShaderTextureNamePrefix("material.");
    stbi_set_flip_vertically_on_load(true);



    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0f, 0.0f);

    pointLight.ambient = glm::vec3(2.0f);

    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    SpotLight& spotLight = programState->spotLight;
    spotLight.position = glm::vec3(0.0f, 5.0, 0.0);
    spotLight.ambient = glm::vec3(10.0f, 10.0f, 10.0f);
    spotLight.diffuse = glm::vec3(0.6f, 0.6f, 0.6f);
    spotLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.constant = 1.0f;
    spotLight.linear = 0.09f;
    spotLight.quadratic = 0.032f;
    spotLight.cutOff = glm::cos(glm::radians(5.0f));
    spotLight.outerCutOff = glm::cos(glm::radians(15.0f));




    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    vector <glm::vec3> wasp_positions = {
            glm::vec3(5.0f, 1.0f, 3.0f),
            glm::vec3(5.0f, 2.0f, 3.0f),
            glm::vec3(-3.0f, 3.0f, 3.0f),
            glm::vec3(0.0f, 2.0f, -3.0f),
            glm::vec3(-2.0f, 1.0f, 4.0f),
            glm::vec3(3.0f, 3.4f, -5.0f),
            glm::vec3(2.0f, 5.0f, 3.0f),
            glm::vec3(-5.0f, 5.0f, -3.0f),
            glm::vec3(3.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 2.0f, 3.0f),
            glm::vec3(-2.0f, 8.0f, 3.0f),
            glm::vec3(3.0f, 1.0f, -3.0f),
            glm::vec3(-4.0f, 4.0f, 4.0f),
            glm::vec3(2.0f, 1.4f, -5.0f),
            glm::vec3(1.0f, 3.0f, 3.0f),
            glm::vec3(3.0f, 5.0f, -3.0f),
            glm::vec3(1.0f, 1.0f, 3.0f),
            glm::vec3(-5.0f, 2.0f, 1.0f),
            glm::vec3(-2.0f, 3.0f, 3.0f),
            glm::vec3(1.0f, 4.0f, -2.0f),
            glm::vec3(-6.0f, 2.0f, 1.0f),
            glm::vec3(5.0f, 1.4f, -3.0f),
            glm::vec3(3.0f, 6.0f, 3.0f),
            glm::vec3(-4.0f, 5.0f, -4.0f),
            glm::vec3(1.0f, 1.0f, 5.0f),
            glm::vec3(2.0f, 2.0f, 3.0f),
            glm::vec3(-3.0f, 1.0f, 2.0f),
            glm::vec3(3.0f, 7.0f, -4.0f),
            glm::vec3(-4.0f, 4.0f, 4.0f),
            glm::vec3(2.0f, 1.4f, -5.0f),
            glm::vec3(1.0f, 3.0f, 3.0f),
            glm::vec3(2.0f, 15.0f, -3.0f)
    };

    vector <glm::vec3> wasp_rotations = {
            glm::vec3(-1.0f, 5.0f, 1.0f),
            glm::vec3(0.0f, -4.0f, 1.0f),
            glm::vec3(1.0f, 3.0f, 0.3f),
            glm::vec3(0.0f, -4.0f, 2.0f),
            glm::vec3(1.0f, 3.0f, -5.0f),
            glm::vec3(-1.0, -5.0f, 1.0f),
            glm::vec3(3.0f, 5.0f, -0.3f),
            glm::vec3(0.0f, -2.0f, 3.0f),
            glm::vec3(-1.0f, 5.0f, 1.0f),
            glm::vec3(0.0f, -4.0f, 1.0f),
            glm::vec3(1.0f, 3.0f, 0.3f),
            glm::vec3(0.0f, -4.0f, 2.0f),
            glm::vec3(1.0f, 3.0f, -5.0f),
            glm::vec3(-1.0, -5.0f, 1.0f),
            glm::vec3(3.0f, 5.0f, -0.3f),
            glm::vec3(0.0f, -2.0f, 3.0f),
            glm::vec3(-1.0f, 5.0f, 1.0f),
            glm::vec3(0.0f, -4.0f, 1.0f),
            glm::vec3(1.0f, 3.0f, 0.3f),
            glm::vec3(0.0f, -4.0f, 2.0f),
            glm::vec3(1.0f, 3.0f, -5.0f),
            glm::vec3(-1.0, -5.0f, 1.0f),
            glm::vec3(3.0f, 5.0f, -0.3f),
            glm::vec3(0.0f, -2.0f, 3.0f),
            glm::vec3(-1.0f, 5.0f, 1.0f),
            glm::vec3(0.0f, -4.0f, 1.0f),
            glm::vec3(1.0f, 3.0f, 0.3f),
            glm::vec3(0.0f, -4.0f, 2.0f),
            glm::vec3(1.0f, 3.0f, -5.0f),
            glm::vec3(-1.0, -5.0f, 1.0f),
            glm::vec3(3.0f, 5.0f, -0.3f),
            glm::vec3(0.0f, -2.0f, 3.0f)
    };



    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // view/projection transformations
        ourShader.use();
        glm::mat4 sceneModel = glm::mat4(1.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 300.0f);
        ourShader.setMat4("model", sceneModel);
        ourShader.setMat4("view", view);
        ourShader.setMat4("projection", projection);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        //pyramid 1 rendering (rotating)

        pyramidShader.use();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
        view = programState->camera.GetViewMatrix();
        //model = glm::translate(model, glm::vec3(4.0f, 0.505f, 0.0f));
        pyramidShader.setMat4("view", view);
        pyramidShader.setMat4("projection", projection);

        glm::mat4 tmp = model;

        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::rotate(model, currentFrame * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        pyramidShader.setMat4("model", model);
        glBindVertexArray(pyramidVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pyramidTexture);
        glDrawArrays(GL_TRIANGLES, 0, 18);
        glBindVertexArray(0);

        //pyramid 2 rendering (rotating)
        model = tmp;
        model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));
        model = glm::rotate(model, currentFrame * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        pyramidShader.setMat4("model", model);
        glBindVertexArray(pyramidVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pyramidTexture);
        glDrawArrays(GL_TRIANGLES, 0, 18);
        glBindVertexArray(0);

        glDisable(GL_CULL_FACE);







        ourShader.use();
        pointLight.position = glm::vec3(22.0f * cos(currentFrame), 1.0f, 22.0f * sin(currentFrame));

        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        ourShader.setBool("blinn", blinn);

        //spot light
        //FLASHLIGHT
        ourShader.setVec3("spotLight.position", programState->camera.Position);


        //FLASHLIGHT
        ourShader.setVec3("spotLight.direction", programState->camera.Front);

        ourShader.setVec3("spotLight.ambient", spotLight.ambient);
        ourShader.setVec3("spotLight.diffuse", glm::vec3(0.85f, 0.25f, 0.0f));
        ourShader.setVec3("spotLight.specular", spotLight.specular);
        ourShader.setFloat("spotLight.constant", spotLight.constant);
        ourShader.setFloat("spotLight.linear", spotLight.linear);
        ourShader.setFloat("spotLight.quadratic", spotLight.quadratic);
        ourShader.setFloat("spotLight.cutOff", spotLight.cutOff);
        ourShader.setFloat("spotLight.outerCutOff", spotLight.outerCutOff);




        // render bee_hover

        model = glm::mat4(1.0f);
        model = glm::translate(model, 15.0f * glm::vec3(cos(currentFrame), 0.0f, sin(currentFrame)));
        model = glm::rotate(model, glm::radians(currentFrame * 58.0f), glm::vec3(0.0f, -1.0f, 0.0f));
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setMat4("model", model);
        sunflowerModel.Draw(ourShader);


        // render robo_wasps

        for(int i = 0; i < wasp_positions.size(); i++){
            model = glm::mat4(1.0f);
            float sign[] = {-1.0f, 1.0f};
            model = glm::translate(model,wasp_positions[i]);
            wasp_rotations[i] += sign[rand() % 2] * glm::vec3(currentFrame * (rand() % 2),
                                           currentFrame * (rand() % 2),
                                           currentFrame * (rand() % 2));
            model = glm::rotate(model, glm::radians(30.0f), wasp_rotations[i]);
            model = glm::scale(model, glm::vec3(programState->waspScale));
            ourShader.setMat4("model", model);
            wasp.Draw(ourShader);
        }



        //skybox rendering
//        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);

        galaxyShader.use();
        galaxyShader.setInt("skybox", 0);
        glm::mat4 viewCube = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        glm::mat4 skyModel = glm::mat4(1.0f);
        skyModel = glm::translate(skyModel, glm::vec3(0.0f, 0.0f, 0.0f));
        galaxyShader.setMat4("model", skyModel);
        galaxyShader.setMat4("view", viewCube);
        galaxyShader.setMat4("projection", projection);

        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
//        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);




//        if (programState->ImGuiEnabled)
//            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        programState->camera.Position.y -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        programState->camera.Position.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        blinn = !blinn;
        blinnKeyPressed = true;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if   (programState->CameraMouseMovementUpdateEnabled && !glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

//void DrawImGui(ProgramState *programState) {
//    ImGui_ImplOpenGL3_NewFrame();
//    ImGui_ImplGlfw_NewFrame();
//    ImGui::NewFrame();
//
//
//    {
//        static float f = 0.0f;
//        ImGui::Begin("Hello window");
//        ImGui::Text("Hello text");
//        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
//        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
//        ImGui::DragFloat3("Backpack position", (float*)&programState->hoverPosition);
//        ImGui::DragFloat("Backpack scale", &programState->hoverScale, 0.05, 0.1, 4.0);
//
//        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
//        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
//        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
//        ImGui::End();
//    }
//
//    {
//        ImGui::Begin("Camera info");
//        const Camera& c = programState->camera;
//        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
//        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
//        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
//        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
//        ImGui::End();
//    }
//
//    ImGui::Render();
//    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

unsigned int loadTexture(char const * path) {
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

unsigned int loadCubemap(vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
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

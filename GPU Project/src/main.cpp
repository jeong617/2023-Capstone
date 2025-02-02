#define _CRT_SECURE_NO_WARNINGS

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <AreaTex.h>
#include <SearchTex.h>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void changeViewpoint(int view);
void timeChecker(std::ofstream& outputFile, bool& benchActive);

// settings
float SCR_WIDTH = 1600.0;
float SCR_HEIGHT = 900.0;

// camera
Camera camera(glm::vec3(-35.0f, 10.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -360.0f, -0.5f);
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;
bool allowMouseInput = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float offsetX = 0.0f;
float offsetY = 0.0f;

// fps counter
double prevTime = 0.0;
double crntTime = 0.0;
double timeDiff;
unsigned int counter = 0;
std::string frameDisplay;
void timeChecker(std::ofstream& outputFile, bool& benchActive);

// global projection variables
glm::mat4 globalCurrProj;
glm::mat4 globalPrevProj;

// TAA reprojection
unsigned int temporalFrame = 0;
bool temporalReproject = false;
glm::mat4 currViewProj;
glm::mat4 prevViewProj;
float reprojectionWeightScale = 30.0f;

// jittering
glm::vec2 jitter;
const glm::vec2 jitters[2] = {
    { -0.25f,  0.25f }
    ,{ 0.25f,  -0.25f }
};

// detail screen
float cursorPosX = 0.0;
float cursorPosY = 0.0;

// AA variables
static bool antiAliasing;
static bool msaa;
static bool fxaa;
static bool smaa;
static bool taa;
static bool wasTAAOn;

static int currentAA = 0;

static bool temporalAAFirstFrame = true;

static bool isImage;

static bool detailScreen;

GLuint colorTex;
GLuint multiSamplingTex;
GLuint depthTex;
GLuint edgeTex;
GLuint blendTex;
GLuint imageTex;
GLuint detailTex;

GLuint areaTex;
GLuint searchTex;

GLuint currentTex;
GLuint previousTex;
GLuint velocityTex;

GLuint Subsample1;
GLuint Subsample2;

GLuint colorFBO;
GLuint multisampledFBO;
GLuint edgeFBO;
GLuint blendFBO;
GLuint detailFBO;

GLuint currentFBO;
GLuint previousFBO;

GLuint colorRBO;
GLuint multiSampledRBO;
GLuint detailRBO;

GLuint quadVAO, quadVBO;

// highest as default
GLuint msaaQualityLevel = 4;
GLuint smaaPreset = 3;

static int currentMSAAQuality = 4;
static int previousMSAAQuailty = 0;

static int currentSMAAQuality = 3;
static int previousSMAAQuality = 0;



struct SMAAParameters
{
    GLfloat threshold;
    GLfloat depthThreshold;
    GLint maxSearchSteps;
    GLint maxSearchStepsDiag;
    GLint cornerRounding;
    // GLuint  pad0;
    // GLuint  pad1;
    // GLuint  pad2;
};

SMAAParameters smaaPresets[4] =
{
        {0.15f, 0.1f * 0.15f, 4, 0, 0} // low
        ,
        {0.10f, 0.1f * 0.10f, 8, 0, 0} // medium
        ,
        {0.10f, 0.1f * 0.10f, 16, 8, 25} // high
        ,
        {0.05f, 0.1f * 0.05f, 32, 16, 25} // ultra
};

GLuint msaaSamples[5] = { 1, 2, 4, 8, 16 };


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    const char* glsl_version = "#version 450";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Anti Aliasing Project", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.ConfigWindowsResizeFromEdges = false;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    // IM_ASSERT(font != NULL);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.5f, 0.5f, 0.5f, 1.00f);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // load textures
    glEnable(GL_TEXTURE_2D);

    // create a color attachment texture
    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &multiSamplingTex);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multiSamplingTex);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaaSamples[msaaQualityLevel], GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);

    glGenTextures(1, &edgeTex);
    glBindTexture(GL_TEXTURE_2D, edgeTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &blendTex);
    glBindTexture(GL_TEXTURE_2D, blendTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &currentTex);
    glBindTexture(GL_TEXTURE_2D, currentTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &previousTex);
    glBindTexture(GL_TEXTURE_2D, previousTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &Subsample1);
    glBindTexture(GL_TEXTURE_2D, Subsample1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &Subsample2);
    glBindTexture(GL_TEXTURE_2D, Subsample2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &detailTex);
    glBindTexture(GL_TEXTURE_2D, detailTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 250, 250, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);


    // load Image
    // -----------
    int width, height, numChannels;
    unsigned char* imageData = stbi_load("resources/Images/SyntheticTests.png", &width, &height, &numChannels, 0);

    if (!imageData)
    {
        std::cout << "Failed to load image" << std::endl;
        return -1;
    }

    glGenTextures(1, &imageTex);
    glBindTexture(GL_TEXTURE_2D, imageTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);

    // flip SMAA textures
    unsigned char* buffer1 = new unsigned char[AREATEX_SIZE];
    // std::vector<unsigned char> tempBuffer1(AREATEX_SIZE);
    for (unsigned int y = 0; y < AREATEX_HEIGHT; y++)
    {
        unsigned int srcY = AREATEX_HEIGHT - 1 - y;
        // unsigned int srcY = y;
        memcpy(&buffer1[y * AREATEX_PITCH], areaTexBytes + srcY * AREATEX_PITCH, AREATEX_PITCH);
    }

    glGenTextures(1, &areaTex);
    glBindTexture(GL_TEXTURE_2D, areaTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, (GLsizei)AREATEX_WIDTH, (GLsizei)AREATEX_HEIGHT, 0, GL_RG, GL_UNSIGNED_BYTE, buffer1); // areaTexBytes);

    delete[] buffer1;
    buffer1 = new unsigned char[SEARCHTEX_SIZE];

    // std::vector<unsigned char> tempBuffer2(SEARCHTEX_SIZE);
    for (unsigned int y = 0; y < SEARCHTEX_HEIGHT; y++)
    {
        unsigned int srcY = SEARCHTEX_HEIGHT - 1 - y;
        // unsigned int srcY = y;
        memcpy(&buffer1[y * SEARCHTEX_PITCH], searchTexBytes + srcY * SEARCHTEX_PITCH, SEARCHTEX_PITCH);
    }
    glGenTextures(1, &searchTex);
    glBindTexture(GL_TEXTURE_2D, searchTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, (GLsizei)SEARCHTEX_WIDTH, (GLsizei)SEARCHTEX_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, buffer1); // searchTexBytes);

    delete[] buffer1;
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    stbi_image_free(imageData);

    // Initialize FBOs
    // ---------------
    glGenFramebuffers(1, &colorFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, colorFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

    glGenRenderbuffers(1, &colorRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, colorRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, colorRBO);

    // MSAA
    glGenFramebuffers(1, &multisampledFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, multisampledFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, multiSamplingTex, 0);

    glGenRenderbuffers(1, &multiSampledRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, multiSampledRBO);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples[msaaQualityLevel], GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, multiSampledRBO);

    // SMAA
    glGenFramebuffers(1, &edgeFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, edgeFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, edgeTex, 0);
    glGenFramebuffers(1, &blendFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, blendFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blendTex, 0);

    // TAA
    glGenFramebuffers(1, &currentFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currentTex, 0);
    glGenFramebuffers(1, &previousFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, previousTex, 0);

    // Detail
    glGenFramebuffers(1, &detailFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, detailFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, detailTex, 0);

    glGenRenderbuffers(1, &detailRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, detailRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 250, 250);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, detailRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // build and compile shaders
    // -------------------------
    Shader modelShader("shader/basicModel.vs", "shader/basicModel.fs");
    Shader screenShader("shader/basicScreen.vs", "shader/basicScreen.fs"); // basic screen shader used for MSAA
    Shader imageShader("shader/ImageShader.vs", "shader/ImageShader.fs");

    Shader fxaaShader("shader/fxaa_demo.vs", "shader/fxaa_demo.fs");

    Shader smaaEdgeShader("shader/smaaEdge.vs", "shader/smaaEdge.fs");
    Shader smaaWeightShader("shader/smaaBlendWeight.vs", "shader/smaaBlendWeight.fs");
    Shader smaaBlendShader("shader/smaaNeighbor.vs", "shader/smaaNeighbor.fs");

    Shader taaShader("shader/temporal.vs", "shader/temporal.fs");

    // load models
    // -----------
    Model container("resources/objects/container/Container.obj");
    Model sponza("resources/objects/sponza-master/sponza.obj");

    Model currentModel = container;

    modelShader.use();
    modelShader.setInt("texture_diffuse1", 0);

    imageShader.use();
    imageShader.setInt("texture_diffuse1", 0); // �ؽ�ó ���� �ε��� ����

    screenShader.use();
    screenShader.setInt("screenTexture", 0);

    // FXAA Shader
    // -----------
    fxaaShader.use();
    fxaaShader.setInt("colorTex", 0);
    fxaaShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

    // Edge Shader
    // -----------
    smaaEdgeShader.use();
    smaaEdgeShader.setInt("colorTex", 0);
    // smaaEdgeShader.setInt("predicationTex", 0);

    smaaEdgeShader.setFloat("smaaThershold", smaaPresets[smaaPreset].threshold);
    smaaEdgeShader.setFloat("smaaDepthThreshold", smaaPresets[smaaPreset].depthThreshold);
    smaaEdgeShader.setInt("smaaMaxSearchSteps", smaaPresets[smaaPreset].maxSearchSteps);
    smaaEdgeShader.setInt("smaaMaxSearchStepsDiag", smaaPresets[smaaPreset].maxSearchStepsDiag);
    smaaEdgeShader.setInt("smaaCornerRounding", smaaPresets[smaaPreset].cornerRounding);

    smaaEdgeShader.setFloat("predicationThreshold", 0.01);
    smaaEdgeShader.setFloat("predicationScale", 2.0);
    smaaEdgeShader.setFloat("predicationStrength", 0.4);

    smaaEdgeShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

    // Weight Shader
    // -------------
    smaaWeightShader.use();
    smaaWeightShader.setInt("edgesTex", 0);
    smaaWeightShader.setInt("areaTex", 1);
    smaaWeightShader.setInt("searchTex", 2);

    smaaWeightShader.setFloat("smaaThershold", smaaPresets[smaaPreset].threshold);
    smaaWeightShader.setFloat("smaaDepthThreshold", smaaPresets[smaaPreset].depthThreshold);
    smaaWeightShader.setInt("smaaMaxSearchSteps", smaaPresets[smaaPreset].maxSearchSteps);
    smaaWeightShader.setInt("smaaMaxSearchStepsDiag", smaaPresets[smaaPreset].maxSearchStepsDiag);
    smaaWeightShader.setInt("smaaCornerRounding", smaaPresets[smaaPreset].cornerRounding);

    smaaWeightShader.setVec4("subsampleIndices", glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));

    smaaWeightShader.setFloat("predicationThreshold", 0.01);
    smaaWeightShader.setFloat("predicationScale", 2.0);
    smaaWeightShader.setFloat("predicationStrength", 0.4);

    smaaWeightShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

    // Blend Shader
    // ------------
    smaaBlendShader.use();
    smaaBlendShader.setInt("colorTex", 0);
    smaaBlendShader.setInt("blendTex", 1);

    smaaBlendShader.setFloat("smaaThershold", smaaPresets[smaaPreset].threshold);
    smaaBlendShader.setFloat("smaaDepthThreshold", smaaPresets[smaaPreset].depthThreshold);
    smaaBlendShader.setInt("smaaMaxSearchSteps", smaaPresets[smaaPreset].maxSearchSteps);
    smaaBlendShader.setInt("smaaMaxSearchStepsDiag", smaaPresets[smaaPreset].maxSearchStepsDiag);
    smaaBlendShader.setInt("smaaCornerRounding", smaaPresets[smaaPreset].cornerRounding);

    smaaBlendShader.setFloat("predicationThreshold", 0.01);
    smaaBlendShader.setFloat("predicationScale", 2.0);
    smaaBlendShader.setFloat("predicationStrength", 0.4);

    smaaBlendShader.setVec4("screenSize", glm::vec4(1.0f / float(SCR_WIDTH), 1.0f / float(SCR_HEIGHT), SCR_WIDTH, SCR_HEIGHT));

    // TAA Shader
    // ------------
    taaShader.use();
    taaShader.setInt("currentTex", 0);
    taaShader.setInt("previousTex", 1);

    taaShader.setVec4("screenSize", glm::vec4(1.0f / float(SCR_WIDTH), 1.0f / float(SCR_HEIGHT), SCR_WIDTH, SCR_HEIGHT));

    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    // screen quad VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    // load txt for benchmark result
    ofstream outputFile("result.txt");
    if (!outputFile)
    {
        std::cerr << "Failed to open text files(frame)" << std::endl;
        return 1;
    }

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // frame counter implementation
        // -----------------------------
        crntTime = glfwGetTime();
        timeDiff = crntTime - prevTime;
        counter++;

        if (timeDiff >= 1.0 / 5.0)
        {
            double FPS = (1.0 / timeDiff) * counter;
            double ms = (timeDiff / counter) * 1000;

            std::stringstream fpsStream, msStream;
            fpsStream << std::fixed << std::setprecision(1) << FPS;
            msStream << std::fixed << std::setprecision(1) << ms;
            frameDisplay = fpsStream.str() + "FPS/ " + msStream.str() + "ms";

            outputFile << fpsStream.str() + "\n";

            prevTime = crntTime;
            counter = 0;
        }
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // input
        // -----
        processInput(window);

        /* ----- Render Control Panel GUI ----- */
        {
            // Set window size before create it
            ImGui::SetNextWindowSize(ImVec2(200, 550), 0);
            ImGui::Begin("Control Panel", NULL, ImGuiWindowFlags_NoMove);  // Create a window called "Hello, world!" and append into it.

            ImGui::SeparatorText("Frame Counter");

            ImGui::TextColored(ImVec4(1, 1, 0, 1), frameDisplay.c_str());

            ImGui::SeparatorText("Anti Aliasing");
            if (ImGui::Checkbox("AA On", &antiAliasing))
            {
                switch (currentAA %= 4) // fit the number to switch-loop
                {
                    // remember which option was activated last time
                case 0:
                    msaa = true;
                    break;
                case 1:
                    fxaa = true;
                    break;
                case 2:
                    smaa = true;
                    break;
                }
                if (wasTAAOn) {
                    taa = true;
                }
            }

            if (ImGui::BeginTable("split", 2))
            {
                ImGui::TableNextColumn();
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::RadioButton("MSAA", &currentAA, 0)) {
                    msaa = true;
                    fxaa = smaa = false;
                    outputFile << "AA Method : MSAA " << std::endl;
                }
                ImGui::TableNextColumn();
                if (ImGui::RadioButton("FXAA", &currentAA, 1)) {
                    fxaa = true;
                    smaa = msaa = false;
                    outputFile << "AA Method : FXAA " << std::endl;
                }
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::RadioButton("SMAA", &currentAA, 2)) {
                    smaa = true;
                    fxaa = msaa = false;
                    outputFile << "AA Method : SMAA " << std::endl;
                }

                ImGui::EndTable();
            }
            ImGui::SeparatorText("Temporal AA");
            if (ImGui::Checkbox("TAA", &taa)) {
                if (taa) {
                    wasTAAOn = true;
                }
                else {
                    wasTAAOn = false;
                    temporalAAFirstFrame = true;
                }

                outputFile << "AA Method : TAA " << std::endl;
                if (msaa) {
                    outputFile << "AA Method : MSAA + TAA " << std::endl;
                }
                if (fxaa) {
                    outputFile << "AA Method : FXAA + TAA" << std::endl;
                }
                if (smaa) {
                    outputFile << "AA Method : SMAA + TAA" << std::endl;
                }
            }

            // Bind to 'AA on' button
            if (antiAliasing == false)
            {
                currentAA += 4; // make none of the buttons are selected if AA is off
                msaa = false;
                fxaa = false;
                smaa = false;
                taa = false;
            }

            /* ----- MSAA Quality ----- */
            const char* msaaQualities[] = { "1X", "2X", "4X", "8X", "16X" };

            ImGui::SeparatorText("MSAA Quality");
            ImGui::Combo("##MSAA Quality", &currentMSAAQuality, msaaQualities, IM_ARRAYSIZE(msaaQualities));

            if (currentMSAAQuality != previousMSAAQuailty)
            {
                switch (currentMSAAQuality)
                {
                case 0:
                    msaaQualityLevel = 0;
                    outputFile << "MSAA 1X " << std::endl;
                    break;
                case 1:
                    msaaQualityLevel = 1;
                    outputFile << "MSAA 2X " << std::endl;
                    break;
                case 2:
                    msaaQualityLevel = 2;
                    outputFile << "MSAA 4X " << std::endl;
                    break;
                case 3:
                    msaaQualityLevel = 3;
                    outputFile << "MSAA 8X " << std::endl;
                    break;
                case 4:
                    msaaQualityLevel = 4;
                    outputFile << "MSAA 16X " << std::endl;
                    break;
                }
                previousMSAAQuailty = currentMSAAQuality;
            }

            // Change the actual number of samples
            if (msaa)
            {
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multiSamplingTex);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaaSamples[msaaQualityLevel], GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

                glBindRenderbuffer(GL_RENDERBUFFER, multiSampledRBO);
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples[msaaQualityLevel], GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
                glBindRenderbuffer(GL_RENDERBUFFER, 0);
            }

            /* ----- SMAA Quality ----- */
            const char* smaaQualities[] = { "LOW", "MEDIUM", "HIGH", "ULTRA" };

            ImGui::SeparatorText("SMAA Quality");
            ImGui::Combo("##SMAA Quality", &currentSMAAQuality, smaaQualities, IM_ARRAYSIZE(smaaQualities));

            if (currentSMAAQuality != previousSMAAQuality)
            {
                switch (currentSMAAQuality)
                {
                case 0:
                    smaaPreset = 0;
                    outputFile << "SMAA LOW " << std::endl;
                    break;
                case 1:
                    smaaPreset = 1;
                    outputFile << "SMAA MEDIUM " << std::endl;
                    break;
                case 2:
                    smaaPreset = 2;
                    outputFile << "SMAA HIGH " << std::endl;
                    break;
                case 3:
                    smaaPreset = 3;
                    outputFile << "SMAA ULTRA " << std::endl;
                    break;
                }
                previousSMAAQuality = currentSMAAQuality;
            }

            /* ----- Change Viewpoint ----- */
            ImGui::SeparatorText("Viewpoint");
            if (ImGui::Button("1"))
                changeViewpoint(1);
            ImGui::SameLine();
            if (ImGui::Button("2"))
                changeViewpoint(2);
            ImGui::SameLine();
            if (ImGui::Button("3"))
                changeViewpoint(3);

            /*----- Change Scene -----*/
            const char* scenes[] = { "Container", "Sponza", "Image" };
            static int currentScene = 0;
            static int previousScene = 3;
            ImGui::SeparatorText("Scene");
            ImGui::Combo("Scene", &currentScene, scenes, IM_ARRAYSIZE(scenes));

            if (currentScene != previousScene)
            {
                switch (currentScene)
                {
                case 0:
                    isImage = false;
                    changeViewpoint(1);
                    currentModel = container;
                    outputFile << "Current Scene : Container " << std::endl;
                    break;
                case 1:
                    isImage = false;
                    changeViewpoint(1);
                    currentModel = sponza;
                    outputFile << "Current Scene : Sponza " << std::endl;
                    break;
                case 2:
                    isImage = true;
                    camera.Position = glm::vec3(-0.122459f, 0.039916f, 5.372975f);
                    camera.Yaw = -89.200050f;
                    camera.Pitch = -0.900008;
                    camera.ProcessMouseMovement(0, 0);
                    outputFile << "Current Scene : Image " << std::endl;
                    break;
                }
                previousScene = currentScene;
            }

            ImGui::SeparatorText("Detail Screen");
            ImGui::Checkbox("Show", &detailScreen);

            /*----- Benchmarking -----*/
            bool benchActive = false;
            ImGui::NewLine();
            if (ImGui::Button("Benchmark(10s)"))
            {
                benchActive = true;
                std::thread timeCheckerThread(timeChecker, std::ref(outputFile), std::ref(benchActive));
                timeCheckerThread.detach();
            }

            ImGui::NewLine();
            if (ImGui::Button("Exit"))
                return 0;

            ImGui::End();
        }

        if (antiAliasing)
        {
            if (msaa)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, multisampledFBO);
            }
            else
            {
                glBindFramebuffer(GL_FRAMEBUFFER, colorFBO);
            }
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // view/projection transformations
        glm::mat4 model;

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 viewProj = projection * view * model;

        if (!isImage)
        {
            allowMouseInput = true;
            modelShader.use();

            if (!taa)
            {
                modelShader.setMat4("projection", projection);
                modelShader.setMat4("view", view);
            }
            else
            {
                temporalFrame = (temporalFrame + 1) % 2;

                jitter = jitters[temporalFrame];
                jitter = jitter * 2.0f * glm::vec2(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT);
                glm::mat4 jitterMatrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(jitter, 0.0f));
                projection = jitterMatrix * projection;

                modelShader.setMat4("projection", globalCurrProj);
                modelShader.setMat4("view", view);

                prevViewProj = currViewProj;
                currViewProj = projection;
                globalCurrProj = currViewProj;
                globalPrevProj = prevViewProj;

            }
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
            model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));  // it's a bit too big for our scene, so scale it down
            modelShader.setMat4("model", model);
            currentModel.Draw(modelShader);
        }
        else
        {
            allowMouseInput = false;
            imageShader.use();

            if (!taa)
            {
                imageShader.setMat4("projection", projection);
                imageShader.setMat4("view", view);
            }
            else
            {
                temporalFrame = (temporalFrame + 1) % 2;

                jitter = jitters[temporalFrame];
                jitter = jitter * 2.0f * glm::vec2(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT);
                glm::mat4 jitterMatrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(jitter, 0.0f));
                projection = jitterMatrix * projection;

                imageShader.setMat4("projection", globalCurrProj);
                imageShader.setMat4("view", view);

                prevViewProj = currViewProj;
                currViewProj = projection;
                globalCurrProj = currViewProj;
                globalPrevProj = prevViewProj;

            }

            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            // render the loaded model
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f)); // translate it down so it's at the center of the scene
            model = glm::scale(model, glm::vec3(2.15f, 2.15f, 1.0f));   // scale

            // projection matrix (needed for final 2D views)
            // glm::mat4 projection = glm::ortho(0, width, height, 0, 0, 1000);

            // modelShader.setMat4("projection", projection);
            imageShader.setMat4("model", model);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, imageTex);

            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (antiAliasing && wasTAAOn) {

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
            // clear all relevant buffers
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
            glClear(GL_COLOR_BUFFER_BIT);

            if (msaa) {

                glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampledFBO);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, colorFBO);
                glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

                glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glDisable(GL_DEPTH_TEST);

                screenShader.use();
                glBindVertexArray(quadVAO);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, colorTex); // use the now resolved color attachment as the quad's texture
                glDrawArrays(GL_TRIANGLES, 0, 6);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            if (fxaa) {

                glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
                glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
                // clear all relevant buffers
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
                glClear(GL_COLOR_BUFFER_BIT);

                fxaaShader.use();
                fxaaShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

                glBindVertexArray(quadVAO);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, colorTex);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            if (smaa) {

                /* EDGE DETECTION PASS */
                glBindFramebuffer(GL_FRAMEBUFFER, edgeFBO);
                glDisable(GL_DEPTH_TEST);
                // clear all relevant buffers
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                smaaEdgeShader.use();
                // set SMAA quality
                smaaEdgeShader.setFloat("smaaThershold", smaaPresets[smaaPreset].threshold);
                smaaEdgeShader.setFloat("smaaDepthThreshold", smaaPresets[smaaPreset].depthThreshold);
                smaaEdgeShader.setInt("smaaMaxSearchSteps", smaaPresets[smaaPreset].maxSearchSteps);
                smaaEdgeShader.setInt("smaaMaxSearchStepsDiag", smaaPresets[smaaPreset].maxSearchStepsDiag);
                smaaEdgeShader.setInt("smaaCornerRounding", smaaPresets[smaaPreset].cornerRounding);
                smaaEdgeShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

                glBindVertexArray(quadVAO);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, colorTex); // use the color attachment texture as the texture of the quad plane
                glDrawArrays(GL_TRIANGLES, 0, 6);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                /* BLENDING WEIGHT PASS */
                glBindFramebuffer(GL_FRAMEBUFFER, blendFBO);

                // clear all relevant buffers
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                smaaWeightShader.use();
                // set SMAA quality
                smaaWeightShader.setFloat("smaaThershold", smaaPresets[smaaPreset].threshold);
                smaaWeightShader.setFloat("smaaDepthThreshold", smaaPresets[smaaPreset].depthThreshold);
                smaaWeightShader.setInt("smaaMaxSearchSteps", smaaPresets[smaaPreset].maxSearchSteps);
                smaaWeightShader.setInt("smaaMaxSearchStepsDiag", smaaPresets[smaaPreset].maxSearchStepsDiag);
                smaaWeightShader.setInt("smaaCornerRounding", smaaPresets[smaaPreset].cornerRounding);
                smaaWeightShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

                glBindVertexArray(quadVAO);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, edgeTex);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, areaTex);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, searchTex);

                glDrawArrays(GL_TRIANGLES, 0, 6);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);

                /*
                /* NEIGHBORHOOD BLENDING PASS */
                smaaBlendShader.use();
                // set SMAA quality
                smaaBlendShader.setFloat("smaaThershold", smaaPresets[smaaPreset].threshold);
                smaaBlendShader.setFloat("smaaDepthThreshold", smaaPresets[smaaPreset].depthThreshold);
                smaaBlendShader.setInt("smaaMaxSearchSteps", smaaPresets[smaaPreset].maxSearchSteps);
                smaaBlendShader.setInt("smaaMaxSearchStepsDiag", smaaPresets[smaaPreset].maxSearchStepsDiag);
                smaaBlendShader.setInt("smaaCornerRounding", smaaPresets[smaaPreset].cornerRounding);
                smaaBlendShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

                glBindVertexArray(quadVAO);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, colorTex);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, blendTex);

                glDrawArrays(GL_TRIANGLES, 0, 6);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }

            taaShader.use();

            glBindVertexArray(quadVAO);

            if (temporalAAFirstFrame) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, currentTex);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, currentTex);

                temporalAAFirstFrame = false;
            }
            else {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, currentTex);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, previousTex);
            }

            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousFBO);
            glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else {
            // TAA off
            if (msaa) {

                glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampledFBO);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, colorFBO);
                glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glDisable(GL_DEPTH_TEST);

                screenShader.use();
                glBindVertexArray(quadVAO);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, colorTex); // use the now resolved color attachment as the quad's texture
                glDrawArrays(GL_TRIANGLES, 0, 6);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            if (fxaa) {

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
                // clear all relevant buffers
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // set clear color to white (not really necessary actually, since we won't be able to see behind the quad anyways)
                glClear(GL_COLOR_BUFFER_BIT);

                fxaaShader.use();
                fxaaShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

                glBindVertexArray(quadVAO);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, colorTex);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
            if (smaa) {

                /* EDGE DETECTION PASS */
                glBindFramebuffer(GL_FRAMEBUFFER, edgeFBO);
                glDisable(GL_DEPTH_TEST);
                // clear all relevant buffers
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                smaaEdgeShader.use();
                // set SMAA quality
                smaaEdgeShader.setFloat("smaaThershold", smaaPresets[smaaPreset].threshold);
                smaaEdgeShader.setFloat("smaaDepthThreshold", smaaPresets[smaaPreset].depthThreshold);
                smaaEdgeShader.setInt("smaaMaxSearchSteps", smaaPresets[smaaPreset].maxSearchSteps);
                smaaEdgeShader.setInt("smaaMaxSearchStepsDiag", smaaPresets[smaaPreset].maxSearchStepsDiag);
                smaaEdgeShader.setInt("smaaCornerRounding", smaaPresets[smaaPreset].cornerRounding);
                smaaEdgeShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

                glBindVertexArray(quadVAO);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, colorTex);

                glDrawArrays(GL_TRIANGLES, 0, 6);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                /* BLENDING WEIGHT PASS */
                glBindFramebuffer(GL_FRAMEBUFFER, blendFBO);

                // clear all relevant buffers
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                smaaWeightShader.use();
                // set SMAA quality
                smaaWeightShader.setFloat("smaaThershold", smaaPresets[smaaPreset].threshold);
                smaaWeightShader.setFloat("smaaDepthThreshold", smaaPresets[smaaPreset].depthThreshold);
                smaaWeightShader.setInt("smaaMaxSearchSteps", smaaPresets[smaaPreset].maxSearchSteps);
                smaaWeightShader.setInt("smaaMaxSearchStepsDiag", smaaPresets[smaaPreset].maxSearchStepsDiag);
                smaaWeightShader.setInt("smaaCornerRounding", smaaPresets[smaaPreset].cornerRounding);
                smaaWeightShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

                glBindVertexArray(quadVAO);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, edgeTex);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, areaTex);
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, searchTex);

                glDrawArrays(GL_TRIANGLES, 0, 6);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                /*
                /* NEIGHBORHOOD BLENDING PASS */
                smaaBlendShader.use();
                // set SMAA quality
                smaaBlendShader.setFloat("smaaThershold", smaaPresets[smaaPreset].threshold);
                smaaBlendShader.setFloat("smaaDepthThreshold", smaaPresets[smaaPreset].depthThreshold);
                smaaBlendShader.setInt("smaaMaxSearchSteps", smaaPresets[smaaPreset].maxSearchSteps);
                smaaBlendShader.setInt("smaaMaxSearchStepsDiag", smaaPresets[smaaPreset].maxSearchStepsDiag);
                smaaBlendShader.setInt("smaaCornerRounding", smaaPresets[smaaPreset].cornerRounding);
                smaaBlendShader.setVec4("screenSize", glm::vec4(1.0f / SCR_WIDTH, 1.0f / SCR_HEIGHT, SCR_WIDTH, SCR_HEIGHT));

                glBindVertexArray(quadVAO);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, colorTex);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, blendTex);

                glDrawArrays(GL_TRIANGLES, 0, 6);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
            }
        }


        /* ----- Render detail image where cursor located ----- */
        if (detailScreen) {
            unsigned int viewportSize = 300;
            unsigned int viewportBeginX = SCR_WIDTH - 360;
            unsigned int viewportBeginY = SCR_HEIGHT - 360;

            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                cursorPosX = cursorPosX;
                cursorPosY = cursorPosY;
            }
            else {
                if (lastX < 50) {
                    cursorPosX = 50;
                }
                else if (lastX > SCR_WIDTH - 70) {
                    cursorPosX = SCR_WIDTH - 70;
                }
                else {
                    cursorPosX = lastX;
                }
                if (lastY < 50) {
                    cursorPosY = SCR_HEIGHT - 50;
                }
                else if (lastY > SCR_HEIGHT - 70) {
                    cursorPosY = 70;
                }
                else {
                    cursorPosY = SCR_HEIGHT - lastY;
                }
            }

            ImGui::SetNextWindowSize(ImVec2(viewportSize + 10, viewportSize + 70), 0);
            ImGui::SetNextWindowPos(ImVec2(viewportBeginX, 60));
            ImGui::Begin("Detail Screen", NULL, ImGuiWindowFlags_NoMove);

            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, detailFBO);
            /* ----- Cursor is the center of the detail screen ----- */
            glBlitFramebuffer(cursorPosX - 50, cursorPosY - 50, cursorPosX + 70, cursorPosY + 70, 0, 0, 300, 300, GL_COLOR_BUFFER_BIT, GL_NEAREST);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            //glViewport(viewportBeginX, viewportBeginY, viewportSize, viewportSize);

            // we get the screen position of the window
            ImVec2 pos = ImGui::GetCursorScreenPos();

            ImGui::GetWindowDrawList()->AddImage(
                (void*)detailTex,
                ImVec2(pos.x, pos.y + 30),
                ImVec2(pos.x + viewportSize, pos.y + viewportSize + 30),
                ImVec2(0, 1),
                ImVec2(1, 0)
            );

            ImGui::SeparatorText("Press and hold SPACE to lock the view");

            ImGui::End();

            //if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            //    //printf("lastX: %f, lastY: %f\n", xPos, yPos);
            //    screenShader.use();
            //    glBindVertexArray(quadVAO);
            //    glActiveTexture(GL_TEXTURE0);
            //    glBindTexture(GL_TEXTURE_2D, detailTex); // use the now resolved color attachment as the quad's texture
            //    glDrawArrays(GL_TRIANGLES, 0, 6);
            //}

        }

        ImGui::Render();

        // Render dear imgui into screen
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    outputFile.close();

    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (taa && !wasTAAOn) {
        wasTAAOn = true;
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if (taa && wasTAAOn) {
            wasTAAOn = false;
            temporalAAFirstFrame = true;
        }
        camera.ProcessKeyboard(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (taa && wasTAAOn) {
            wasTAAOn = false;
            temporalAAFirstFrame = true;
        }
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        if (taa && wasTAAOn) {
            wasTAAOn = false;
            temporalAAFirstFrame = true;
        }
        camera.ProcessKeyboard(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        if (taa && wasTAAOn) {
            wasTAAOn = false;
            temporalAAFirstFrame = true;
        }
        camera.ProcessKeyboard(RIGHT, deltaTime);
    }

    float velocity = deltaTime * 5.0f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        if (taa && wasTAAOn) {
            wasTAAOn = false;
            temporalAAFirstFrame = true;
        }
        camera.Position += camera.Up * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        if (taa && wasTAAOn) {
            wasTAAOn = false;
            temporalAAFirstFrame = true;
        }
        camera.Position -= camera.Up * velocity;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    SCR_WIDTH = width;
    SCR_HEIGHT = height;

    {
        glBindTexture(GL_TEXTURE_2D, colorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, multiSamplingTex);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaaSamples[msaaQualityLevel], GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);
        glBindTexture(GL_TEXTURE_2D, edgeTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, blendTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glBindTexture(GL_TEXTURE_2D, currentTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, previousTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

        glBindRenderbuffer(GL_RENDERBUFFER, multiSampledRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples[msaaQualityLevel], GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }


    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        if (taa && !wasTAAOn) {
            wasTAAOn = true;
        }

        lastX = xpos;
        lastY = ypos;
        return;
    }

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

    // mouse disabled when image is on
    if (!allowMouseInput)
    {
        return;
    }

    if (taa && wasTAAOn) {
        wasTAAOn = false;
        temporalAAFirstFrame = true;
    }

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    std::cout << "Button Clicked!: " << button << std::endl;
}
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    std::cout << "Cursor moved! x: " << xpos << " y: " << ypos << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        double x = camera.Position.x;
        double y = camera.Position.y;
        double z = camera.Position.z;
        double yaw = camera.Yaw;
        double pitch = camera.Pitch;

        printf("Pos: (%f, %f, %f), POV: (%f, %f)\n", x, y, z, yaw, pitch);
    }
}

void changeViewpoint(int view)
{
    if (!isImage) {
        if (view == 1)
        {
            camera.Position = glm::vec3(-35.0f, 10.0f, 0.0f);
            camera.Yaw = -360.0f;
            camera.Pitch = -0.5f;
            camera.ProcessMouseMovement(0, 0);

        }
        if (view == 2)
        {
            camera.Position = glm::vec3(-1.70f, 7.44f, -7.60f);
            camera.Yaw = 111.90;
            camera.Pitch = -6.60;
            camera.ProcessMouseMovement(0, 0);
        }
        if (view == 3)
        {
            camera.Position = glm::vec3(-10.09f, 7.89f, -6.09f);
            camera.Yaw = -40.60;
            camera.Pitch = 33.30;
            camera.ProcessMouseMovement(0, 0);
        }
    }
}

void timeChecker(std::ofstream& outputFile, bool& benchActive)
{
    std::cout << "timer set" << std::endl;
    outputFile << "start benchmarking" << std::endl;
    int targetTimeSeconds = 10;

    std::chrono::seconds waitTime(targetTimeSeconds);

    std::this_thread::sleep_for(waitTime);
    outputFile << "recorded fps for 10s" << std::endl;

    benchActive = false;
    std::cout << "timer ended" << std::endl;
}

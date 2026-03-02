///////////////////////////////////////////////////////////////////////////////
// SceneManager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Modified by: (YOU) for Milestone Four (textures)
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
    const char* g_ModelName = "model";
    const char* g_ColorValueName = "objectColor";
    const char* g_TextureValueName = "objectTexture";
    const char* g_UseTextureName = "bUseTexture";
    const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes = new ShapeMeshes();

    // IMPORTANT: start with 0 textures loaded
    m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 ***********************************************************/
SceneManager::~SceneManager()
{
    m_pShaderManager = NULL;
    delete m_basicMeshes;
    m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
    int width = 0;
    int height = 0;
    int colorChannels = 0;
    GLuint textureID = 0;

    // flip images vertically when loaded (common for OpenGL UVs)
    stbi_set_flip_vertically_on_load(true);

    unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);

    if (image)
    {
        std::cout << "Successfully loaded image: " << filename
            << ", width: " << width << ", height: " << height
            << ", channels: " << colorChannels << std::endl;

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // wrapping (REPEAT is what enables tiling!)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (colorChannels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (colorChannels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
            stbi_image_free(image);
            glBindTexture(GL_TEXTURE_2D, 0);
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_textureIDs[m_loadedTextures].ID = textureID;
        m_textureIDs[m_loadedTextures].tag = tag;
        m_loadedTextures++;

        return true;
    }

    std::cout << "Could not load image: " << filename << std::endl;
    return false;
}

/***********************************************************
 *  BindGLTextures()
 ***********************************************************/
void SceneManager::BindGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
    }
}

/***********************************************************
 *  DestroyGLTextures()
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
    // Correct cleanup: delete textures (NOT glGenTextures)
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glDeleteTextures(1, &m_textureIDs[i].ID);
        m_textureIDs[i].ID = 0;
    }
    m_loadedTextures = 0;
}

/***********************************************************
 *  FindTextureSlot()
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
    int textureSlot = -1;
    int index = 0;
    bool bFound = false;

    while ((index < m_loadedTextures) && (bFound == false))
    {
        if (m_textureIDs[index].tag.compare(tag) == 0)
        {
            textureSlot = index;
            bFound = true;
        }
        else
            index++;
    }

    return textureSlot;
}

/***********************************************************
 *  SetTransformations()
 ***********************************************************/
void SceneManager::SetTransformations(
    glm::vec3 scaleXYZ,
    float XrotationDegrees,
    float YrotationDegrees,
    float ZrotationDegrees,
    glm::vec3 positionXYZ)
{
    glm::mat4 modelView;
    glm::mat4 scale;
    glm::mat4 rotationX;
    glm::mat4 rotationY;
    glm::mat4 rotationZ;
    glm::mat4 translation;

    scale = glm::scale(scaleXYZ);
    rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
    translation = glm::translate(positionXYZ);

    modelView = translation * rotationX * rotationY * rotationZ * scale;

    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setMat4Value(g_ModelName, modelView);
    }
}

/***********************************************************
 *  SetShaderColor()
 ***********************************************************/
void SceneManager::SetShaderColor(float r, float g, float b, float a)
{
    glm::vec4 currentColor(r, g, b, a);

    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, false);
        m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
    }
}

/***********************************************************
 *  SetShaderTexture()
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string textureTag)
{
    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, true);

        int textureSlot = FindTextureSlot(textureTag);
        m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
    }
}

/***********************************************************
 *  SetTextureUVScale()
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
    if (NULL != m_pShaderManager)
    {
        m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
    }
}

/***********************************************************
 *  PrepareScene()  (Milestone Four: load textures here)
 ***********************************************************/
void SceneManager::PrepareScene()
{
    // Meshes
    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadCylinderMesh();
    m_basicMeshes->LoadConeMesh();

    // Textures (NOTE: this path matches your project folder "textures")
    // Tag it "marble" so we can reference it by name later.
    CreateGLTexture("textures/marble_01_diff_2k.jpg", "marble");

    // Bind them once so they’re available (safe to call; also re-bound each frame below)
    BindGLTextures();
}

/***********************************************************
 *  RenderScene()
 ***********************************************************/
void SceneManager::RenderScene()
{
    // Re-bind textures each frame (keeps state consistent)
    BindGLTextures();

    // transformations
    glm::vec3 scaleXYZ;
    float XrotationDegrees = 0.0f;
    float YrotationDegrees = 0.0f;
    float ZrotationDegrees = 0.0f;
    glm::vec3 positionXYZ;

    /****************************************************************/
    // FLOOR PLANE (still a plain color for contrast)
    /****************************************************************/
    scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);
    positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
    m_basicMeshes->DrawPlaneMesh();

    /****************************************************************/
    // CANDLE (Cylinder + Cone)
    /****************************************************************/

    // ==================== Candle Body (Cylinder) ====================
    // Apply texture + tiling (complex technique requirement)
    scaleXYZ = glm::vec3(0.35f, 1.20f, 0.35f);
    positionXYZ = glm::vec3(0.0f, 1.20f, 0.0f);

    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);

    // Use marble texture on candle body
    SetShaderTexture("marble");

    // TILING (complex texturing technique): increase UVs so marble repeats
    SetTextureUVScale(2.5f, 2.5f);

    m_basicMeshes->DrawCylinderMesh();

    // ==================== Candle Flame (Cone) ====================
    // Turn texture OFF by setting a color
    scaleXYZ = glm::vec3(0.15f, 0.35f, 0.15f);
    positionXYZ = glm::vec3(0.0f, 2.75f, 0.0f);

    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
    SetShaderColor(1.0f, 0.6f, 0.1f, 1.0f);
    m_basicMeshes->DrawConeMesh();
}
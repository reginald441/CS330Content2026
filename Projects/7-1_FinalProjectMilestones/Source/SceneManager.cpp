///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
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
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
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

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue("g_UseLightingName", true);

	// Main white light above the desk
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 7.0f, 6.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setBoolValue("pointLights[0].active", true);

	// Soft blue light to show a clear lighting update
	m_pShaderManager->setVec3Value("pointLights[1].position", -5.0f, 4.0f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.08f, 0.12f, 0.20f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.20f, 0.45f, 1.0f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.40f, 0.60f, 1.0f);
	m_pShaderManager->setBoolValue("pointLights[1].active", true);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadSphereMesh();

	// Load textures
	CreateGLTexture("textures/metal.jpg", "metal");
	CreateGLTexture("textures/floor.jpg", "floor");
	CreateGLTexture("textures/screen.jpg", "screen");
	CreateGLTexture("textures/chair.jpg", "chair");

	// Bind textures
	BindGLTextures();
}

/***********************************************************
 *  RenderScene()
 ***********************************************************/
void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;

	// Floor
	scaleXYZ = glm::vec3(24.0f, 1.0f, 14.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("floor");
	SetTextureUVScale(6.0f, 6.0f);
	m_basicMeshes->DrawPlaneMesh();

	// Back wall
	scaleXYZ = glm::vec3(24.0f, 1.0f, 9.0f);
	positionXYZ = glm::vec3(0.0f, 4.0f, -5.4f);
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.55f, 0.70f, 0.95f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// Main black desk
	scaleXYZ = glm::vec3(18.5f, 0.35f, 4.0f);
	positionXYZ = glm::vec3(0.5f, 1.0f, -1.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("metal");
	SetTextureUVScale(4.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();

	// Left L-shaped desk section
	scaleXYZ = glm::vec3(5.2f, 0.35f, 6.4f);
	positionXYZ = glm::vec3(-6.0f, 1.0f, 0.7f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.11f, 0.11f, 0.11f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Right desk extension
	scaleXYZ = glm::vec3(5.0f, 0.35f, 4.8f);
	positionXYZ = glm::vec3(6.2f, 1.0f, 0.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.12f, 0.12f, 0.12f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Visible desk legs - adjusted so all legs clearly rest on the carpet
	scaleXYZ = glm::vec3(0.45f, 1.8f, 0.45f);

	// Main desk legs
	positionXYZ = glm::vec3(-7.0f, 0.90f, 4.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(6.0f, 0.90f, 4.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-7.0f, 0.90f, 0.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(7.0f, 0.90f, 0.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Small foot pads under desk legs so they visibly sit on the carpet
	scaleXYZ = glm::vec3(0.70f, 0.08f, 0.70f);

	positionXYZ = glm::vec3(-7.0f, 0.04f, 4.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(6.0f, 0.04f, 4.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-7.0f, 0.04f, 0.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(7.0f, 0.04f, 0.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Monitors
	float monitorX[5] = { -7.4f, -3.6f, 0.0f, 3.6f, 7.4f };
	float monitorY[5] = { 2.65f, 2.75f, 2.80f, 2.75f, 2.65f };
	float monitorRotY[5] = { 28.0f, 10.0f, 0.0f, -10.0f, -28.0f };

	for (int i = 0; i < 5; i++)
	{
		scaleXYZ = glm::vec3(3.35f, 2.35f, 0.18f);
		positionXYZ = glm::vec3(monitorX[i], monitorY[i], -2.70f);
		SetTransformations(scaleXYZ, 0.0f, monitorRotY[i], 0.0f, positionXYZ);
		SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
		m_basicMeshes->DrawBoxMesh();

		scaleXYZ = glm::vec3(3.0f, 2.0f, 0.08f);
		positionXYZ = glm::vec3(monitorX[i], monitorY[i] + 0.05f, -2.52f);
		SetTransformations(scaleXYZ, 0.0f, monitorRotY[i], 0.0f, positionXYZ);
		SetShaderTexture("screen");
		SetTextureUVScale(1.0f, 1.0f);
		m_basicMeshes->DrawBoxMesh();

		if (i == 0 || i == 4)
		{
			scaleXYZ = glm::vec3(3.25f, 0.42f, 0.20f);
			positionXYZ = glm::vec3(monitorX[i], 1.58f, -2.48f);
			SetTransformations(scaleXYZ, 0.0f, monitorRotY[i], 0.0f, positionXYZ);
			SetShaderColor(0.82f, 0.82f, 0.78f, 1.0f);
			m_basicMeshes->DrawBoxMesh();
		}

		scaleXYZ = glm::vec3(0.18f, 0.70f, 0.18f);
		positionXYZ = glm::vec3(monitorX[i], 1.50f, -2.25f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderColor(0.45f, 0.45f, 0.45f, 1.0f);
		m_basicMeshes->DrawBoxMesh();

		scaleXYZ = glm::vec3(1.05f, 0.10f, 0.55f);
		positionXYZ = glm::vec3(monitorX[i], 1.18f, -2.05f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderColor(0.28f, 0.28f, 0.28f, 1.0f);
		m_basicMeshes->DrawBoxMesh();
	}

	// Laptop
	scaleXYZ = glm::vec3(2.1f, 0.14f, 1.35f);
	positionXYZ = glm::vec3(5.35f, 1.35f, 0.45f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.025f, 0.025f, 0.025f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(2.0f, 1.25f, 0.10f);
	positionXYZ = glm::vec3(5.35f, 2.10f, -0.35f);
	SetTransformations(scaleXYZ, -65.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("screen");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Keyboards
	scaleXYZ = glm::vec3(1.9f, 0.08f, 0.45f);
	positionXYZ = glm::vec3(-5.2f, 1.33f, 0.35f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, -6.0f, positionXYZ);
	SetShaderColor(0.92f, 0.92f, 0.88f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-2.7f, 1.33f, 0.35f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 4.0f, positionXYZ);
	SetShaderColor(0.92f, 0.92f, 0.88f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(2.4f, 0.10f, 0.50f);
	positionXYZ = glm::vec3(0.8f, 1.34f, 0.38f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("chair");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Mice
	scaleXYZ = glm::vec3(0.30f, 0.14f, 0.42f);
	positionXYZ = glm::vec3(-4.0f, 1.45f, 0.90f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.90f, 0.90f, 0.86f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	positionXYZ = glm::vec3(-1.7f, 1.45f, 0.85f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.90f, 0.90f, 0.86f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	scaleXYZ = glm::vec3(0.35f, 0.16f, 0.45f);
	positionXYZ = glm::vec3(2.7f, 1.45f, 0.55f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.04f, 0.04f, 0.04f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// Books
	scaleXYZ = glm::vec3(1.2f, 0.10f, 0.85f);
	positionXYZ = glm::vec3(4.0f, 1.36f, 1.20f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, -8.0f, positionXYZ);
	SetShaderColor(0.70f, 0.05f, 0.04f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(6.8f, 1.36f, 1.10f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 6.0f, positionXYZ);
	SetShaderColor(0.18f, 0.11f, 0.06f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Cables
	for (int i = 0; i < 5; i++)
	{
		scaleXYZ = glm::vec3(0.022f, 0.022f, 1.5f);
		positionXYZ = glm::vec3(-2.4f + i * 1.1f, 0.70f, 0.95f);
		SetTransformations(scaleXYZ, 35.0f, 0.0f, 0.0f, positionXYZ);
		SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
		m_basicMeshes->DrawCylinderMesh();
	}

	// Center office chair
	scaleXYZ = glm::vec3(2.8f, 0.50f, 2.4f);
	positionXYZ = glm::vec3(0.0f, 0.85f, 3.20f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("chair");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(2.3f, 0.35f, 3.0f);
	positionXYZ = glm::vec3(0.0f, 1.25f, 3.95f);
	SetTransformations(scaleXYZ, -70.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("chair");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Armrests
	scaleXYZ = glm::vec3(0.18f, 0.60f, 1.20f);
	positionXYZ = glm::vec3(-1.2f, 1.0f, 3.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(1.2f, 1.0f, 3.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Chair center support post - clearly reaches from chair seat to carpet
	scaleXYZ = glm::vec3(0.25f, 1.05f, 0.25f);
	positionXYZ = glm::vec3(0.0f, 0.55f, 3.25f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Chair floor base left/right
	scaleXYZ = glm::vec3(2.4f, 0.10f, 0.28f);
	positionXYZ = glm::vec3(0.0f, 0.08f, 3.25f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Chair floor base front/back
	scaleXYZ = glm::vec3(0.28f, 0.10f, 2.0f);
	positionXYZ = glm::vec3(0.0f, 0.08f, 3.25f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.02f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Chair visible feet/wheels touching carpet
	scaleXYZ = glm::vec3(0.30f, 0.18f, 0.30f);

	positionXYZ = glm::vec3(-1.2f, 0.12f, 3.25f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	positionXYZ = glm::vec3(1.2f, 0.12f, 3.25f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	positionXYZ = glm::vec3(0.0f, 0.12f, 2.25f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	positionXYZ = glm::vec3(0.0f, 0.12f, 4.25f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.01f, 0.01f, 0.01f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// ---------- Small plant on right side of desk ----------

// Plant pot
	scaleXYZ = glm::vec3(0.45f, 0.35f, 0.45f);
	positionXYZ = glm::vec3(6.2f, 2.05f, 2.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.35f, 0.20f, 0.10f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Plant stem
	scaleXYZ = glm::vec3(0.08f, 0.35f, 0.08f);
	positionXYZ = glm::vec3(6.2f, 2.35f, 2.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.25f, 0.15f, 0.05f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Plant top
	scaleXYZ = glm::vec3(0.45f, 0.45f, 0.45f);
	positionXYZ = glm::vec3(6.2f, 2.70f, 2.2f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.05f, 0.45f, 0.15f, 1.0f);
	m_basicMeshes->DrawSphereMesh();

	// ---------- Wall sign: Innovation Workspace ----------

// Sign background on wall
	scaleXYZ = glm::vec3(5.0f, 1.4f, 0.08f);
	positionXYZ = glm::vec3(-5.5f, 5.6f, -5.45f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.02f, 0.02f, 0.08f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Bright blue main stripe
	scaleXYZ = glm::vec3(4.3f, 0.22f, 0.10f);
	positionXYZ = glm::vec3(-5.5f, 5.65f, -5.52f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.0f, 0.65f, 1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// White top line
	scaleXYZ = glm::vec3(3.6f, 0.08f, 0.10f);
	positionXYZ = glm::vec3(-5.5f, 6.05f, -5.53f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Green bottom line
	scaleXYZ = glm::vec3(3.6f, 0.08f, 0.10f);
	positionXYZ = glm::vec3(-5.5f, 5.15f, -5.53f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.0f, 0.85f, 0.35f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Small left square logo
	scaleXYZ = glm::vec3(0.45f, 0.45f, 0.12f);
	positionXYZ = glm::vec3(-7.6f, 5.6f, -5.56f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.0f, 0.85f, 0.35f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Small right square logo
	scaleXYZ = glm::vec3(0.45f, 0.45f, 0.12f);
	positionXYZ = glm::vec3(-3.4f, 5.6f, -5.56f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.0f, 0.65f, 1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Coffee mug body
	scaleXYZ = glm::vec3(0.35f, 0.45f, 0.35f);
	positionXYZ = glm::vec3(-1.0f, 1.55f, 0.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Lamp base
	scaleXYZ = glm::vec3(0.65f, 0.10f, 0.65f);
	positionXYZ = glm::vec3(-7.15f, 1.48f, 3.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Straight lamp pole - connected from base upward
	scaleXYZ = glm::vec3(0.10f, 3.20f, 0.10f);
	positionXYZ = glm::vec3(-7.15f, 3.05f, 3.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.75f, 0.75f, 0.75f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	// Top horizontal lamp arm - lifted above the monitor
	scaleXYZ = glm::vec3(1.65f, 0.10f, 0.10f);
	positionXYZ = glm::vec3(-6.35f, 5.15f, 3.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.75f, 0.75f, 0.75f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// Lamp bulb/head - lifted above the monitor and attached to arm
	scaleXYZ = glm::vec3(0.35f, 0.35f, 0.35f);
	positionXYZ = glm::vec3(-5.45f, 5.05f, 3.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 0.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();
}
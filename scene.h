#pragma once

#include "opengl.h"
#include "packed_freelist.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <string>

struct DiffuseMap
{
    GLuint DiffuseMapTO;
};

struct AlphaMap
{
	GLuint AlphaMapTO;
};

struct SpecularMap
{
	GLuint SpecularMapTO;
};

struct BumpMap
{
	GLuint BumpMapTO;
};


struct Material
{
    std::string Name;

    float Ambient[3];
    float Diffuse[3];
    float Specular[3];
    float Shininess;
    uint32_t DiffuseMapID;
	uint32_t AlphaMapID;
	uint32_t SpecularMapID;
	uint32_t BumpMapID;
};

struct Mesh
{
    std::string Name;

    GLuint MeshVAO;
    GLuint PositionBO;
    GLuint TexCoordBO;
    GLuint NormalBO;
    GLuint IndexBO;
	//added
	GLuint TangentBO;

    GLuint IndexCount;
    GLuint VertexCount;

    std::vector<GLDrawElementsIndirectCommand> DrawCommands;
    std::vector<uint32_t> MaterialIDs;
};

struct Transform
{
    glm::vec3 Scale;
    glm::vec3 RotationOrigin;
    glm::quat Rotation;
    glm::vec3 Translation;
	//added code
	glm::uint32_t ParentID;
};

struct Instance
{
    uint32_t MeshID;
    uint32_t TransformID;
};

struct Camera
{
    // View
    glm::vec3 Eye;
    glm::vec3 Look;
    glm::vec3 Up;

    // Projection
    float FovY;

};


struct Light
{
	// View
	glm::vec3 Position;
	glm::vec3 Direction;
	glm::vec3 Up;

	// Projection
	float FovY;
};

class Scene
{
public:
    packed_freelist<DiffuseMap> DiffuseMaps;
	packed_freelist<AlphaMap> AlphaMaps;
	packed_freelist<SpecularMap> SpecularMaps;
	packed_freelist<BumpMap> BumpMaps;
    packed_freelist<Material> Materials;
    packed_freelist<Mesh> Meshes;
    packed_freelist<Transform> Transforms;
    packed_freelist<Instance> Instances;
	GLuint cubemapTexture;
    Camera MainCamera;
	Light MainLight;

    void Init();
};

void LoadMeshesFromFile(
    Scene* scene,
    const std::string& filename,
    std::vector<uint32_t>* loadedMeshIDs);

void AddMeshInstance(
    Scene* scene,
    uint32_t meshID,
    uint32_t* newInstanceID);

void CalculateTangentArray(long vertexCount, const std::vector<float> vertex, const std::vector<float> normal,
	const std::vector<float> texcoord, long triangleCount, const std::vector<unsigned int> triangle, std::vector<float>& tangent);

GLuint LoadCubemap(
	std::vector<const GLchar*> faces
);
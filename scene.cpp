#include "scene.h"
#include <iostream>
#include "preamble.glsl"

#include "tiny_obj_loader.h"
#include "stb_image.h"

#include <map>

void Scene::Init()
{
    // Need to specify size up front. These numbers are pretty arbitrary.
    DiffuseMaps = packed_freelist<DiffuseMap>(512);
	AlphaMaps = packed_freelist<AlphaMap>(512);
	SpecularMaps = packed_freelist<SpecularMap>(512);
	BumpMaps = packed_freelist<BumpMap>(512);
    Materials = packed_freelist<Material>(512);
    Meshes = packed_freelist<Mesh>(512);
    Transforms = packed_freelist<Transform>(4096);
    Instances = packed_freelist<Instance>(4096);
}

void LoadMeshesFromFile(
    Scene* scene,
    const std::string& filename,
    std::vector<uint32_t>* loadedMeshIDs)
{
        // assume mtl is in the same folder as the obj
    std::string mtl_basepath = filename;
    size_t last_slash = mtl_basepath.find_last_of("/");
    if (last_slash == std::string::npos)
        mtl_basepath = "./";
    else
        mtl_basepath = mtl_basepath.substr(0, last_slash + 1);

    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    if (!tinyobj::LoadObj(
        shapes, materials, err, 
        filename.c_str(), mtl_basepath.c_str(),
        tinyobj::calculate_normals | tinyobj::triangulation))
    {
        fprintf(stderr, "tinyobj::LoadObj(%s) error: %s\n", filename.c_str(), err.c_str());
        return;
    }
    
    if (!err.empty())
    {
        fprintf(stderr, "tinyobj::LoadObj(%s) warning: %s\n", filename.c_str(), err.c_str());
    }

    // Add materials to the scene
    std::map<std::string, uint32_t> diffuseMapCache;
	std::map<std::string, uint32_t> alphaMapCache;
	std::map<std::string, uint32_t> specularMapCache;
	std::map<std::string, uint32_t> bumpMapCache;
    std::vector<uint32_t> newMaterialIDs;
    for (const tinyobj::material_t& materialToAdd : materials)
    {
        Material newMaterial;

        newMaterial.Name = materialToAdd.name;

        newMaterial.Ambient[0] = materialToAdd.ambient[0];
        newMaterial.Ambient[1] = materialToAdd.ambient[1];
        newMaterial.Ambient[2] = materialToAdd.ambient[2];
        newMaterial.Diffuse[0] = materialToAdd.diffuse[0];
        newMaterial.Diffuse[1] = materialToAdd.diffuse[1];
        newMaterial.Diffuse[2] = materialToAdd.diffuse[2];
        newMaterial.Specular[0] = materialToAdd.specular[0];
        newMaterial.Specular[1] = materialToAdd.specular[1];
        newMaterial.Specular[2] = materialToAdd.specular[2];
        newMaterial.Shininess = materialToAdd.shininess;

        newMaterial.DiffuseMapID = -1;
		newMaterial.AlphaMapID = -1;
		newMaterial.SpecularMapID = -1;
		newMaterial.BumpMapID = -1;

        if (!materialToAdd.diffuse_texname.empty())
        {
            auto cachedTexture = diffuseMapCache.find(materialToAdd.diffuse_texname);

            if (cachedTexture != end(diffuseMapCache))
            {
                newMaterial.DiffuseMapID = cachedTexture->second;
            }
            else
            {
                std::string diffuse_texname_full = mtl_basepath + materialToAdd.diffuse_texname;
                int x, y, comp;
                stbi_set_flip_vertically_on_load(1);
                stbi_uc* pixels = stbi_load(diffuse_texname_full.c_str(), &x, &y, &comp, 4);
                stbi_set_flip_vertically_on_load(0);

                if (!pixels)
                {
                    fprintf(stderr, "stbi_load(%s): %s\n", diffuse_texname_full.c_str(), stbi_failure_reason());
                }
                else
                {
                    float maxAnisotropy;
                    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

                    GLuint newDiffuseMapTO;
                    glGenTextures(1, &newDiffuseMapTO);
                    glBindTexture(GL_TEXTURE_2D, newDiffuseMapTO);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
                    glGenerateMipmap(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    DiffuseMap newDiffuseMap;
                    newDiffuseMap.DiffuseMapTO = newDiffuseMapTO;
                    
                    uint32_t newDiffuseMapID = scene->DiffuseMaps.insert(newDiffuseMap);

                    diffuseMapCache.emplace(materialToAdd.diffuse_texname, newDiffuseMapID);

                    newMaterial.DiffuseMapID = newDiffuseMapID;

                    stbi_image_free(pixels);
                }
            }
        }

		if (!materialToAdd.alpha_texname.empty())
		{
			auto cachedTexture = alphaMapCache.find(materialToAdd.alpha_texname);
			if (cachedTexture != end(alphaMapCache))
			{
				newMaterial.AlphaMapID = cachedTexture->second;
			}
			else
			{
				std::string Alpha_texname_full = mtl_basepath + materialToAdd.alpha_texname;
				int x, y, comp;
				stbi_set_flip_vertically_on_load(1);
				stbi_uc* pixels = stbi_load(Alpha_texname_full.c_str(), &x, &y, &comp, 4);
				stbi_set_flip_vertically_on_load(0);

				if (!pixels)
				{
					fprintf(stderr, "stbi_load(%s): %s\n", Alpha_texname_full.c_str(), stbi_failure_reason());
				}
				else
				{
					float maxAnisotropy;
					glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

					GLuint newAlphaMapTO;
					glGenTextures(1, &newAlphaMapTO);
					glBindTexture(GL_TEXTURE_2D, newAlphaMapTO);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
					glGenerateMipmap(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, 0);

					AlphaMap newAlphaMap;
					newAlphaMap.AlphaMapTO = newAlphaMapTO;
					size_t a = scene->AlphaMaps.capacity();
					uint32_t newAlphaMapID = scene->AlphaMaps.insert(newAlphaMap);

					alphaMapCache.emplace(materialToAdd.alpha_texname, newAlphaMapID);

					newMaterial.AlphaMapID = newAlphaMapID;

					stbi_image_free(pixels);
				}
			}
		}


		if (!materialToAdd.specular_texname.empty())
		{
			auto cachedTexture = specularMapCache.find(materialToAdd.specular_texname);
			if (cachedTexture != end(specularMapCache))
			{
				newMaterial.SpecularMapID = cachedTexture->second;
			}
			else
			{
				std::string Specular_texname_full = mtl_basepath + materialToAdd.specular_texname;
				int x, y, comp;
				stbi_set_flip_vertically_on_load(1);
				stbi_uc* pixels = stbi_load(Specular_texname_full.c_str(), &x, &y, &comp, 4);
				stbi_set_flip_vertically_on_load(0);

				if (!pixels)
				{
					fprintf(stderr, "stbi_load(%s): %s\n", Specular_texname_full.c_str(), stbi_failure_reason());
				}
				else
				{
					float maxAnisotropy;
					glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

					GLuint newSpecularMapTO;
					glGenTextures(1, &newSpecularMapTO);
					glBindTexture(GL_TEXTURE_2D, newSpecularMapTO);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
					glGenerateMipmap(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, 0);

					SpecularMap newSpecularMap;
					newSpecularMap.SpecularMapTO = newSpecularMapTO;
					size_t a = scene->SpecularMaps.capacity();
					uint32_t newSpecularMapID = scene->SpecularMaps.insert(newSpecularMap);

					specularMapCache.emplace(materialToAdd.specular_texname, newSpecularMapID);

					newMaterial.SpecularMapID = newSpecularMapID;

					stbi_image_free(pixels);
				}
			}
		}

		if (!materialToAdd.bump_texname.empty())
		{
			auto cachedTexture = bumpMapCache.find(materialToAdd.bump_texname);
			if (cachedTexture != end(bumpMapCache))
			{
				newMaterial.BumpMapID = cachedTexture->second;
			}
			else
			{
				std::string Bump_texname_full = mtl_basepath + materialToAdd.bump_texname;
				int x, y, comp;
				stbi_set_flip_vertically_on_load(1);
				stbi_uc* pixels = stbi_load(Bump_texname_full.c_str(), &x, &y, &comp, 4);
				stbi_set_flip_vertically_on_load(0);

				if (!pixels)
				{
					fprintf(stderr, "stbi_load(%s): %s\n", Bump_texname_full.c_str(), stbi_failure_reason());
				}
				else
				{
					float maxAnisotropy;
					glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);

					GLuint newBumpMapTO;
					glGenTextures(1, &newBumpMapTO);
					glBindTexture(GL_TEXTURE_2D, newBumpMapTO);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8_SNORM, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);//GL_RGBA8_SNORM buggy
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
					glGenerateMipmap(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, 0);

					BumpMap newBumpMap;
					newBumpMap.BumpMapTO = newBumpMapTO;
					size_t a = scene->BumpMaps.capacity();
					uint32_t newBumpMapID = scene->BumpMaps.insert(newBumpMap);

					bumpMapCache.emplace(materialToAdd.bump_texname, newBumpMapID);

					newMaterial.BumpMapID = newBumpMapID;

					stbi_image_free(pixels);
				}
			}
		}
		
        uint32_t newMaterialID = scene->Materials.insert(newMaterial);

        newMaterialIDs.push_back(newMaterialID);
    }

    // Add meshes (and prototypes) to the scene
    for (const tinyobj::shape_t& shapeToAdd : shapes)
    {
        const tinyobj::mesh_t& meshToAdd = shapeToAdd.mesh;

        Mesh newMesh;

        newMesh.Name = shapeToAdd.name;

        newMesh.IndexCount = (GLuint)meshToAdd.indices.size();
        newMesh.VertexCount = (GLuint)meshToAdd.positions.size() / 3;

		//creat tangent spaces
		std::vector<float> tan_arry;
		CalculateTangentArray(newMesh.VertexCount, meshToAdd.positions, meshToAdd.normals, meshToAdd.texcoords, (GLuint)meshToAdd.indices.size()/3, meshToAdd.indices, tan_arry);

        if (meshToAdd.positions.empty())
        {
            // should never happen
            newMesh.PositionBO = 0;
        }
        else
        {
            GLuint newPositionBO;
            glGenBuffers(1, &newPositionBO);
            glBindBuffer(GL_ARRAY_BUFFER, newPositionBO);
            glBufferData(GL_ARRAY_BUFFER, meshToAdd.positions.size() * sizeof(meshToAdd.positions[0]), meshToAdd.positions.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            newMesh.PositionBO = newPositionBO;
        }

        if (meshToAdd.texcoords.empty())
        {
            newMesh.TexCoordBO = 0;
        }
        else
        {
            GLuint newTexCoordBO;
            glGenBuffers(1, &newTexCoordBO);
            glBindBuffer(GL_ARRAY_BUFFER, newTexCoordBO);
            glBufferData(GL_ARRAY_BUFFER, meshToAdd.texcoords.size() * sizeof(meshToAdd.texcoords[0]), meshToAdd.texcoords.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            newMesh.TexCoordBO = newTexCoordBO;
        }

        if (meshToAdd.normals.empty())
        {
            newMesh.NormalBO = 0;
        }
        else
        {
            GLuint newNormalBO;
            glGenBuffers(1, &newNormalBO);
            glBindBuffer(GL_ARRAY_BUFFER, newNormalBO);
            glBufferData(GL_ARRAY_BUFFER, meshToAdd.normals.size() * sizeof(meshToAdd.normals[0]), meshToAdd.normals.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            newMesh.NormalBO = newNormalBO;
        }

        if (meshToAdd.indices.empty())
        {
            // should never happen
            newMesh.IndexBO = 0;
        }
        else
        {
            GLuint newIndexBO;
            glGenBuffers(1, &newIndexBO);
            // Why not bind to GL_ELEMENT_ARRAY_BUFFER?
            // Because binding to GL_ELEMENT_ARRAY_BUFFER attaches the EBO to the currently bound VAO, which might stomp somebody else's state.
            glBindBuffer(GL_ARRAY_BUFFER, newIndexBO);
            glBufferData(GL_ARRAY_BUFFER, meshToAdd.indices.size() * sizeof(meshToAdd.indices[0]), meshToAdd.indices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            newMesh.IndexBO = newIndexBO;
        }
		//added for tangentspace
		if (tan_arry.empty())
		{
			newMesh.TangentBO = 0;
		}
		else 
		{
			GLuint newTangentBO;
			glGenBuffers(1, &newTangentBO);
			glBindBuffer(GL_ARRAY_BUFFER, newTangentBO);
			glBufferData(GL_ARRAY_BUFFER, tan_arry.size() * sizeof(tan_arry[0]), &tan_arry[0], GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			newMesh.TangentBO = newTangentBO;
		}
        
        // Hook up VAO
        {
            GLuint newMeshVAO;
            glGenVertexArrays(1, &newMeshVAO);

            glBindVertexArray(newMeshVAO);
			
            if (newMesh.PositionBO)
            {
                glBindBuffer(GL_ARRAY_BUFFER, newMesh.PositionBO);
                glVertexAttribPointer(SCENE_POSITION_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glEnableVertexAttribArray(SCENE_POSITION_ATTRIB_LOCATION);
            }

            if (newMesh.TexCoordBO)
            {
                glBindBuffer(GL_ARRAY_BUFFER, newMesh.TexCoordBO);
                glVertexAttribPointer(SCENE_TEXCOORD_ATTRIB_LOCATION, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glEnableVertexAttribArray(SCENE_TEXCOORD_ATTRIB_LOCATION);
            }

            if (newMesh.NormalBO)
            {
                glBindBuffer(GL_ARRAY_BUFFER, newMesh.NormalBO);
                glVertexAttribPointer(SCENE_NORMAL_ATTRIB_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);

                glEnableVertexAttribArray(SCENE_NORMAL_ATTRIB_LOCATION);
            }

			//added for tangend_arr
			if (newMesh.TangentBO)
			{
				glBindBuffer(GL_ARRAY_BUFFER, newMesh.TangentBO);
				glVertexAttribPointer(SCENE_TANGENT_ATTRIB_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 4, 0);
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				glEnableVertexAttribArray(SCENE_TANGENT_ATTRIB_LOCATION);
			}

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newMesh.IndexBO);

            glBindVertexArray(0);

            newMesh.MeshVAO = newMeshVAO;
        }

        // split mesh into draw calls with different materials
        int numFaces = (int)meshToAdd.indices.size() / 3;
        int currMaterialFirstFaceIndex = 0;
        for (int faceIdx = 0; faceIdx < numFaces; faceIdx++)
        {
            bool isLastFace = faceIdx + 1 == numFaces;
            bool isNextFaceDifferent = isLastFace || meshToAdd.material_ids[faceIdx + 1] != meshToAdd.material_ids[faceIdx];
            if (isNextFaceDifferent)
            {
                GLDrawElementsIndirectCommand currDrawCommand;
                currDrawCommand.count = ((faceIdx + 1) - currMaterialFirstFaceIndex) * 3;
                currDrawCommand.primCount = 1;
                currDrawCommand.firstIndex = currMaterialFirstFaceIndex * 3;
                currDrawCommand.baseVertex = 0;
                currDrawCommand.baseInstance = 0;

                uint32_t currMaterialID = newMaterialIDs[meshToAdd.material_ids[faceIdx]];

                newMesh.DrawCommands.push_back(currDrawCommand);
                newMesh.MaterialIDs.push_back(currMaterialID);

                currMaterialFirstFaceIndex = faceIdx + 1;
            }
        }

        uint32_t newMeshID = scene->Meshes.insert(newMesh);

        if (loadedMeshIDs)
        {
            loadedMeshIDs->push_back(newMeshID);
        }
    }
}

//calculate tanSpace
void CalculateTangentArray(long vertexCount, const std::vector<float> vertex, const std::vector<float> normal,
	const std::vector<float> texcoord, long triangleCount, const std::vector<unsigned int> triangle, std::vector<float>& tangent)
{
	glm::vec3 *tan1 = new glm::vec3[vertexCount * 2];
	glm::vec3 *tan2 = tan1 + vertexCount;
	memset(tan1, 0, vertexCount * sizeof(glm::vec3) * 2);
	//std::cout << "vertCount si: " << vertexCount << "\n";
	for (long a = 0; a < triangleCount; a++)
	{
		long i1, i2, i3;

		i1 = triangle[3 * a + 0];
		i2 = triangle[3 * a + 1];
		i3 = triangle[3 * a + 2];
		//std::cout << i1 << "," << i2 << "," << i3 << "\n";
		//vertex: x0, y0, z0, x1, y1, z1, x2, y2, z2...
		glm::vec3 v1 = glm::vec3(vertex[i1 * 3 + 0], vertex[i1 * 3 + 1], vertex[i1 * 3 + 2]);
		glm::vec3 v2 = glm::vec3(vertex[i2 * 3 + 0], vertex[i2 * 3 + 1], vertex[i2 * 3 + 2]);
		glm::vec3 v3 = glm::vec3(vertex[i3 * 3 + 0], vertex[i3 * 3 + 1], vertex[i3 * 3 + 2]);
		//std::cout << v1 << "," << v2 << "," << v3 << "\n";
		const glm::vec2 w1 = glm::vec2(texcoord[i1 * 2 + 0], texcoord[i1 * 2 + 1]);
		const glm::vec2 w2 = glm::vec2(texcoord[i2 * 2 + 0], texcoord[i2 * 2 + 1]);
		const glm::vec2 w3 = glm::vec2(texcoord[i3 * 2 + 0], texcoord[i3 * 2 + 1]);

		float x1 = v2[0] - v1[0];
		float x2 = v3[0] - v1[0];
		float y1 = v2[1] - v1[1];
		float y2 = v3[1] - v1[1];
		float z1 = v2[2] - v1[2];
		float z2 = v3[2] - v1[2];

		float s1 = w2[0] - w1[0];
		float s2 = w3[0] - w1[0];
		float t1 = w2[1] - w1[1];
		float t2 = w3[1] - w1[1];
		
		
		float r = 1.0F / (s1 * t2 - s2 * t1);
		glm::vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r);
		glm::vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r);

		tan1[i1] += sdir;
		tan1[i2] += sdir;
		tan1[i3] += sdir;

		tan2[i1] += tdir;
		tan2[i2] += tdir;
		tan2[i3] += tdir;
	}

	tangent.resize(vertexCount * 4);

	for (long a = 0; a < vertexCount; a++)
	{
		glm::vec3 n = glm::vec3(normal[a * 3 + 0], normal[a * 3 + 1], normal[a * 3 + 2]);
		glm::vec3 t = glm::vec3(tan1[a].x, tan1[a].y, tan1[a].z);

		// Gram-Schmidt orthogonalize
		tangent[4 * a + 0] = normalize(t - n * dot(n, t))[0];
		tangent[4 * a + 1] = normalize(t - n * dot(n, t))[1];
		tangent[4 * a + 2] = normalize(t - n * dot(n, t))[2];

		// Calculate handedness
		tangent[4 * a + 3] = (dot(cross(n, t), tan2[a]) < 0.0F) ? -1.0F : 1.0F;
	}
	delete[] tan1;
	
}

void AddMeshInstance(
    Scene* scene,
    uint32_t meshID,
    uint32_t* newInstanceID)
{
    Transform newTransform;
    newTransform.Scale = glm::vec3(1.0f);
	newTransform.ParentID = -1;

    uint32_t newTransformID = scene->Transforms.insert(newTransform);

    Instance newInstance;
    newInstance.MeshID = meshID;
    newInstance.TransformID = newTransformID;

    uint32_t tmpNewInstanceID = scene->Instances.insert(newInstance);
    if (newInstanceID)
    {
        *newInstanceID = tmpNewInstanceID;
    }
}

GLuint LoadCubemap(std::vector<const GLchar*> faces) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glActiveTexture(GL_TEXTURE0 + SCENE_SKYBOX_TEXTURE_BINDING);

	int width, height;
	//unsigned char* image;

	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
	for (GLuint i = 0; i < faces.size(); i++)
	{
		//stbi_set_flip_vertically_on_load(1);
		stbi_uc* pixels = stbi_load(faces[i], &width, &height, 0, 3);
		//stbi_set_flip_vertically_on_load(0);
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
			GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels
		);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	return textureID;
}
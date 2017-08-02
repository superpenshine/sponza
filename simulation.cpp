#include "simulation.h"
#include "scene.h"
#include "imgui.h"
#define FLYTHROUGH_CAMERA_IMPLEMENTATION
#include "flythrough_camera.h"
#include <glm/gtc/type_ptr.hpp>
#include <SDL.h>
#include <iostream>

float interval = 0.5;
int max = 5;
float yzero = 0;
int slots = max / interval + 1;
float table[11];//max/interval + 1
float accum = 0;
int flag = 0;

float evaluate(float x) {
	return x*x*x + 3 * x;
}

float get_x_by_arc_len(float u) {
	if (u < 0 || u > 1)
		return -1.0;
	if (u == 0 || u == 1)
		return u;
	int i;
	for (i = 0; i < slots; i++) {
		if (u == table[i])
			return i;
		if (u < table[i])
			break;
	}
	return 1.0*(i - 1)*interval + (u - table[i - 1]) / (table[i] - table[i - 1])*interval;

}

void Simulation::Init(Scene* scene)
{
    mScene = scene;

    std::vector<uint32_t> loadedMeshIDs;

    loadedMeshIDs.clear();
    LoadMeshesFromFile(mScene, "assets/cube/cube.obj", &loadedMeshIDs);
    for (uint32_t loadedMeshID : loadedMeshIDs)
    {
		{
		uint32_t newInstanceID;
		AddMeshInstance(mScene, loadedMeshID, &newInstanceID);
		uint32_t newTransformID = scene->Instances[newInstanceID].TransformID;
		scene->Transforms[newTransformID].Scale = glm::vec3(2.0f);
		}

	}

    loadedMeshIDs.clear();
    LoadMeshesFromFile(mScene, "assets/teapot/teapot.obj", &loadedMeshIDs);
    for (uint32_t loadedMeshID : loadedMeshIDs)
    {
        // place a teapot on the side
        {
            uint32_t newInstanceID;
            AddMeshInstance(mScene, loadedMeshID, &newInstanceID);
            uint32_t newTransformID = scene->Instances[newInstanceID].TransformID;
            scene->Transforms[newTransformID].Translation += glm::vec3(3.0f, 1.0f, 4.0f);
		}

        //place a teapot on top of the cube
		{
			uint32_t parentInstanceID;
			AddMeshInstance(mScene, loadedMeshID, &parentInstanceID);
			uint32_t newTransformID = scene->Instances[parentInstanceID].TransformID;
			scene->Transforms[newTransformID].Translation += glm::vec3(0.0f, 2.0f, 0.0f);
			pID = parentInstanceID;
		}

		// place another teapot on the side
		{
			uint32_t childInstanceID;
			AddMeshInstance(mScene, loadedMeshID, &childInstanceID);
			uint32_t childTransformID = scene->Instances[childInstanceID].TransformID;
			scene->Transforms[childTransformID].Translation += glm::vec3(3.0f, 0.5f, -4.0f);
			scene->Transforms[childTransformID].RotationOrigin = -(scene->Transforms[childTransformID].Translation);
			scene->Transforms[childTransformID].ParentID = scene->Instances[pID].TransformID;
			Simulation::mSpinningTransformID = childTransformID;
		}
    }
    loadedMeshIDs.clear();
    LoadMeshesFromFile(mScene, "assets/floor/floor.obj", &loadedMeshIDs);
    for (uint32_t loadedMeshID : loadedMeshIDs)
    {
        AddMeshInstance(mScene, loadedMeshID, nullptr);
    }

	
	
	//add sponza material
	loadedMeshIDs.clear();
	LoadMeshesFromFile(mScene, "assets/sponza/sponza.obj", &loadedMeshIDs);
	for (uint32_t loadedMeshID : loadedMeshIDs)
	{
	if (scene->Meshes[loadedMeshID].Name == "sponza_04")
	{
	continue;
	}
	uint32_t newInstanceID;
	AddMeshInstance(mScene, loadedMeshID, &newInstanceID);
	uint32_t newTransformID = scene->Instances[newInstanceID].TransformID;
	scene->Transforms[newTransformID].Scale = glm::vec3(1.0f / 50.0f);
	}
	
	//skybox
	std::vector<const GLchar*> faces;
	faces.push_back("assets/ame_oasis/oasisnight_rt.tga");
	faces.push_back("assets/ame_oasis/oasisnight_lf.tga");
	faces.push_back("assets/ame_oasis/oasisnight_up.tga");
	faces.push_back("assets/ame_oasis/oasisnight_dn.tga");
	faces.push_back("assets/ame_oasis/oasisnight_bk.tga");
	faces.push_back("assets/ame_oasis/oasisnight_ft.tga");

	GLuint cubemapTexture1 = LoadCubemap(faces);
	scene->cubemapTexture = cubemapTexture1;
	

	//set up arc_lenth_table
	for (int i = 0; i < slots; i++) {
		float yone = evaluate(i*interval);
		table[i] = (yone - yzero) / (interval);
		yzero = yone;
	}
	int Max_Arc_Len = table[slots - 1];
	for (int i = 0; i < slots; i++)
		table[i] /= Max_Arc_Len;

    Camera mainCamera;
    mainCamera.Eye = glm::vec3(5.0f, 5.0f, 5.0f);
    glm::vec3 target = glm::vec3(0.0f);
    mainCamera.Look = normalize(target - mainCamera.Eye);
    mainCamera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
    mainCamera.FovY = glm::radians(70.0f);
    mScene->MainCamera = mainCamera;

	Light mainLight;
	mainLight.Position = glm::vec3(5.0f, 5.0f, 0.0f);
	target = glm::vec3(0.0f);
	mainLight.Direction = normalize(target - mainLight.Position);
	mainLight.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	mainLight.FovY = glm::radians(70.0f);
	mScene->MainLight = mainLight;
}

void Simulation::HandleEvent(const SDL_Event& ev)
{
    if (ev.type == SDL_MOUSEMOTION)
    {
        mDeltaMouseX += ev.motion.xrel;
        mDeltaMouseY += ev.motion.yrel;
    }
}

void Simulation::Update(float deltaTime)
{
	int d;
    const Uint8* keyboard = SDL_GetKeyboardState(&d);
    
    int mx, my;
    Uint32 mouse = SDL_GetMouseState(&mx, &my);

	float angularVelocity = 30.0f;
	mScene->Transforms[mSpinningTransformID].Rotation *= glm::angleAxis(glm::radians(angularVelocity * deltaTime), glm::vec3(0.0f, 1.0f, 0.0f));

    if ((mouse & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0)
    {
        flythrough_camera_update(
            value_ptr(mScene->MainCamera.Eye),
            value_ptr(mScene->MainCamera.Look),
            value_ptr(mScene->MainCamera.Up),
            NULL,
            deltaTime,
            5.0f, // eye_speed
            0.1f, // degrees_per_cursor_move
            80.0f, // max_pitch_rotation_degrees
            mDeltaMouseX, mDeltaMouseY,
            keyboard[SDL_SCANCODE_W], keyboard[SDL_SCANCODE_A], keyboard[SDL_SCANCODE_S], keyboard[SDL_SCANCODE_D],
            keyboard[SDL_SCANCODE_SPACE], keyboard[SDL_SCANCODE_LCTRL],
            0);
    }

	if (keyboard[SDL_SCANCODE_Z])
	{
		std::cout << deltaTime << "\n";
		mScene->MainCamera.Eye[2] = get_x_by_arc_len(accum);
		mScene->MainCamera.Eye[0] = accum;
		if (flag == 0)
			accum += deltaTime;
		else
			accum -= deltaTime;
		if (accum >= 1.0)
			flag = 1;
		else if (accum <= 0.0)
			flag = 0;
		flythrough_camera_update(
			value_ptr(mScene->MainCamera.Eye),
			value_ptr(mScene->MainCamera.Look),
			value_ptr(mScene->MainCamera.Up),
			NULL,
			deltaTime,
			5.0f, // eye_speed
			0.1f, // degrees_per_cursor_move
			80.0f, // max_pitch_rotation_degrees
			mDeltaMouseX, mDeltaMouseY,
			keyboard[SDL_SCANCODE_W], keyboard[SDL_SCANCODE_A], keyboard[SDL_SCANCODE_S], keyboard[SDL_SCANCODE_D],
			keyboard[SDL_SCANCODE_SPACE], keyboard[SDL_SCANCODE_LCTRL],
			0);
			
		
	}

    mDeltaMouseX = 0;
    mDeltaMouseY = 0;


	Light& light = mScene->MainLight;




    if (ImGui::Begin("Example GUI Window"))
    {
        ImGui::Text("Mouse Pos: (%d, %d)", mx, my);
		ImGui::Text("Mouse Pos: (%d, %d)", mx, my);
		ImGui::SliderFloat3("Light position", value_ptr(light.Position), -40.0f, 40.0f);
    }
    ImGui::End();
}

void* Simulation::operator new(size_t sz)
{
    // zero out the memory initially, for convenience.
    void* mem = ::operator new(sz);
    memset(mem, 0, sz);
    return mem;
}
    
#pragma once

#include <cstddef>

struct SDL_Window;
union SDL_Event;
class Scene;

class Simulation
{
    Scene* mScene;
	Scene* mBox;

    int mDeltaMouseX;
    int mDeltaMouseY;
	int mSpinningTransformID;
	int pID;

public:
    void Init(Scene* scene);
    void HandleEvent(const SDL_Event& ev);
    void Update(float deltaTime);

    void* operator new(size_t sz);
};

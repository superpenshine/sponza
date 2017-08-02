#pragma once

#include "shaderset.h"

struct SDL_Window;
class Scene;

class Renderer
{
    Scene* mScene;

    // ShaderSet explanation:
    // https://nlguillemot.wordpress.com/2016/07/28/glsl-shader-live-reloading/
    ShaderSet mShaders;

    GLuint* mSceneSP;
	GLuint* mShadowSP;
	GLuint* mSky;
	GLuint* mBlur;
	GLuint* mGbuffer;
	GLuint* mComposite;
	GLuint* mSSAO;
	GLuint* mSSAOBlur;

    int mBackbufferWidth;
    int mBackbufferHeight;
    GLuint mBackbufferFBO;
    GLuint mBackbufferColorTO;
	GLuint mBackbufferColorTO2;
    GLuint mBackbufferDepthTO;
	//Bloom
	GLuint pingpongFBO[2];
	GLuint pingpongVBO;
	GLuint pingpongVAO;
	GLuint pingpongBuffer[2];
	GLuint compositeFBO;
	GLuint compositeVBO;
	GLuint compositeVAO;
	GLuint mCompositebufferColorTO;
	//shadow
	GLuint mShadowDepthTO;
	GLuint kShadowMapResolution;
	GLuint mShadowFBO;
	GLfloat mShadowSlopeScaleBias;
	GLfloat mShadowDepthBias;
	//skybox
	GLuint skyboxVAO;
	GLuint skyboxVBO;
	GLuint cubemapTexture;
	//gBuffer
	GLuint gBufferFBO;
	GLuint gPosition, gNormal, gSpecular;
	GLuint gBufferDepthTO;
	GLuint gAlbedo;
	//SSAO
	GLuint noiseTexture;
	GLuint ssaoColorBuffer;
	GLuint ssaoFBO;
	GLuint SSAOVBO;
	GLuint SSAOVAO;
	GLfloat ssaoKernel[192];//64*3
	//SSAO Blur
	GLuint ssaoBlurFBO, ssaoColorBufferBlur;

	GLuint* mDepthVisSP;
	GLuint mNullVAO;
	bool mShowDepthVis;

public:
    void Init(Scene* scene);
    void Resize(int width, int height);
    void Render();

    void* operator new(size_t sz);
};

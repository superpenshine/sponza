#include "renderer.h"
#include "scene.h"
#include "imgui.h"

#include "preamble.glsl"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SDL.h>
#include <random>

std::default_random_engine generator;
std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
void Renderer::Init(Scene* scene)
{
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
    mScene = scene;
	kShadowMapResolution = 1024;
    // feel free to increase the GLSL version if your computer supports it
    mShaders.SetVersion("410");
    mShaders.SetPreambleFile("preamble.glsl");

	mSky = mShaders.AddProgramFromExts({ "sky.vert", "sky.frag" });
    mSceneSP = mShaders.AddProgramFromExts({ "scene.vert", "scene.frag" });
	mShadowSP = mShaders.AddProgramFromExts({ "shadow.vert", "shadow.frag" });
	mBlur = mShaders.AddProgramFromExts({ "blur.vert", "blur.frag" });
	mGbuffer = mShaders.AddProgramFromExts({ "mGbuffer.vert", "mGbuffer.frag" });
	mSSAO = mShaders.AddProgramFromExts({ "mSSAO.vert", "mSSAO.frag" });
	mSSAOBlur = mShaders.AddProgramFromExts({ "mSSAOBlur.vert", "mSSAOBlur.frag" });
	mComposite = mShaders.AddProgramFromExts({ "composite.vert", "composite.frag" });
	//added (shadow)

	glGenTextures(1, &mShadowDepthTO);//the texture we are going to render to
	glBindTexture(GL_TEXTURE_2D, mShadowDepthTO);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, kShadowMapResolution, kShadowMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);//allocate memmory for the textures, Give an empty image to OpenGL ( the last "0" )
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);//ensure that tests that fall outside the view of the light return the far depth
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);//¡°GL_LEQUAL¡±, which allows you to sample the texture using a GLSL ¡°sampler2DShadow¡±, which automatically does depth comparisons upon sampling
	const float kShadowBorderDepth[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, kShadowBorderDepth);
	glBindTexture(GL_TEXTURE_2D, 0);
	//bind texture
	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	glGenFramebuffers(1, &mShadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mShadowFBO);//all future texture functions will modify this framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mShadowDepthTO, 0); //attach a texture image to a framebuffer object
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLfloat skyboxVertices[] = {
		//right    
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		//left
		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,
		//up
		-1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,
		//down
		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		//back
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		//front
		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,
	};
	/*
	GLfloat skyboxVertices[] = {
		// Positions          
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
	};*/
	//uploads veretex data to gpu
	glGenBuffers(1, &skyboxVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Setup skybox VAO, mapping from vertex buffers to vertex shader
	glGenVertexArrays(1, &skyboxVAO);
	glBindVertexArray(skyboxVAO);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//bloom pingpong VAO and VBO
	GLfloat dummy_vertices[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		1.0f, 1.0f,

		-1.0f, -1.0f,
		1.0f, 1.0f,
		-1.0f, 1.0f
	};
	glGenBuffers(1, &pingpongVBO);
	glBindBuffer(GL_ARRAY_BUFFER, pingpongVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(dummy_vertices), &dummy_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenVertexArrays(1, &pingpongVAO);
	glBindVertexArray(pingpongVAO);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, pingpongVBO);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//bloom composite VAO and VBO
	glGenBuffers(1, &compositeVBO);
	glBindBuffer(GL_ARRAY_BUFFER, compositeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(dummy_vertices), &dummy_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenVertexArrays(1, &compositeVAO);
	glBindVertexArray(compositeVAO);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, compositeVBO);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//SSAO VAO and VBO
	glGenBuffers(1, &SSAOVBO);
	glBindBuffer(GL_ARRAY_BUFFER, SSAOVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(dummy_vertices), &dummy_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenVertexArrays(1, &SSAOVAO);
	glBindVertexArray(SSAOVAO);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, SSAOVBO);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	for (GLuint i = 0; i < 64; i+=3)//creating SSAO kernel samples
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)
		);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		//GLfloat scale = lerp(0.1f, 1.0f, scale * scale);
		GLfloat scale = 1.0*(GLfloat)i / 64;
		scale = std::fma(scale*scale, 1.0f, std::fma(-scale*scale, 0.1f, 0.1f));
		sample *= scale;
		ssaoKernel[i] = sample.x;
		ssaoKernel[i+1] = sample.y;
		ssaoKernel[i+2] = sample.z;
	}
	
	//imgui debug code
	mDepthVisSP = mShaders.AddProgramFromExts({ "depthvis.vert", "depthvis.frag" });
	glGenVertexArrays(1, &mNullVAO);
	glBindVertexArray(mNullVAO);
	glBindVertexArray(0);
}

void Renderer::Resize(int width, int height)
{
    mBackbufferWidth = width;
    mBackbufferHeight = height;

    // Init Backbuffer FBO
    {
		//gBufferFBO setup
		glDeleteTextures(1, &gPosition);
		glGenTextures(1, &gPosition);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, mBackbufferWidth, mBackbufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteTextures(1, &gNormal);
		glGenTextures(1, &gNormal);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, mBackbufferWidth, mBackbufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteTextures(1, &gBufferDepthTO);
		glGenTextures(1, &gBufferDepthTO);
		glBindTexture(GL_TEXTURE_2D, gBufferDepthTO);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, mBackbufferWidth, mBackbufferHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenTextures(1, &gAlbedo);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, mBackbufferWidth, mBackbufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glDeleteFramebuffers(1, &gBufferFBO);
		glGenFramebuffers(1, &gBufferFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gBufferDepthTO, 0);
		GLuint attachments0[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, attachments0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//backbuffer
        glDeleteTextures(1, &mBackbufferColorTO);
        glGenTextures(1, &mBackbufferColorTO);
        glBindTexture(GL_TEXTURE_2D, mBackbufferColorTO);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, mBackbufferWidth, mBackbufferHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);//allocate memmory for the textures
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

		glDeleteTextures(1, &mBackbufferColorTO2);
		glGenTextures(1, &mBackbufferColorTO2);
		glBindTexture(GL_TEXTURE_2D, mBackbufferColorTO2);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, mBackbufferWidth, mBackbufferHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);//allocate memmory for the textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

        glDeleteTextures(1, &mBackbufferDepthTO);
        glGenTextures(1, &mBackbufferDepthTO);
        glBindTexture(GL_TEXTURE_2D, mBackbufferDepthTO);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, mBackbufferWidth, mBackbufferHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glBindTexture(GL_TEXTURE_2D, 0);

        glDeleteFramebuffers(1, &mBackbufferFBO);
        glGenFramebuffers(1, &mBackbufferFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, mBackbufferFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mBackbufferColorTO, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mBackbufferColorTO2, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mBackbufferDepthTO, 0);
		GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, attachments);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//ping-pong FBO for blur
		glDeleteFramebuffers(2, pingpongFBO);
		glDeleteTextures(2, pingpongBuffer);
		glGenFramebuffers(2, pingpongFBO);
		glGenTextures(2, pingpongBuffer);
		for (GLuint i = 0; i < 2; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
			glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGBA16, mBackbufferWidth, mBackbufferHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0
			);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//SSAO rotation texture

		std::vector<glm::vec3> ssaoNoise;	//creat kernel rotation
		for (GLuint i = 0; i < 16; i++)
		{
			glm::vec3 noise(
				randomFloats(generator) * 2.0 - 1.0,
				randomFloats(generator) * 2.0 - 1.0,
				0.0f);
			ssaoNoise.push_back(noise);
		}

		glDeleteTextures(1, &noiseTexture);
		glGenTextures(1, &noiseTexture);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		glDeleteFramebuffers(1, &ssaoFBO);
		glGenFramebuffers(1, &ssaoFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
		glGenTextures(1, &ssaoColorBuffer);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, mBackbufferWidth, mBackbufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuffer, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//SSAO BlurFBO setup
		glGenFramebuffers(1, &ssaoBlurFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
		glGenTextures(1, &ssaoColorBufferBlur);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, mBackbufferWidth, mBackbufferHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBufferBlur, 0);

		//Composite FBO
		glDeleteTextures(1, &mCompositebufferColorTO);
		glDeleteFramebuffers(1, &compositeFBO);
		glGenFramebuffers(1, &compositeFBO);
		glGenTextures(1, &mCompositebufferColorTO);//texture used as color buffer
		glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
		glBindTexture(GL_TEXTURE_2D, mCompositebufferColorTO);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA16, mBackbufferWidth, mBackbufferHeight, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mCompositebufferColorTO, 0
		);


		GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "glCheckFramebufferStatus: %x\n", fboStatus);
        }
		glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//
    }
}

void Renderer::Render()
{
    mShaders.UpdatePrograms();
	const Camera& mainCamera = mScene->MainCamera;
	//imgui debug code
	if (ImGui::Begin("Renderer Options", 0, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Checkbox("Show shadow map", &mShowDepthVis);
		ImGui::SliderFloat("Slope Scale Bias", &mShadowSlopeScaleBias, 0.0f, 10.0f);
		ImGui::SliderFloat("Depth Bias", &mShadowDepthBias, 0.0f, 1000.0f);
	}
	ImGui::End();

//alpha masking
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

/*
#pragma omp parallel for
	for (int i = 0; i < 100; i++)
	{

	}
*/
	// Clear last frame
	{
		//geometryFBO
		glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
		glClearColor(0.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//SSAOFBO
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
		glClearColor(158.0f / 255.0f, 66.0f / 255.0f, 244.0f / 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		//mBackbuffer
		glBindFramebuffer(GL_FRAMEBUFFER, mBackbufferFBO);
		glClearColor(0.0f / 255.0f, 0.0f / 255.0f, 0.0f / 255.0f, 1.0f);
		//glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	//render shadow
	glUseProgram(*mShadowSP);
	
	GLint SCENE_MODELVIEWPROJECTION_UNIFORM_LOCATION = glGetUniformLocation(*mShadowSP, "ModelViewProjection");
	const Light& mainLight = mScene->MainLight;
	glm::vec3 lightPos = mainLight.Position;
	glm::vec3 up = mainLight.Up;
	glm::mat4 lightWorldView = glm::lookAt(lightPos, lightPos + mainLight.Direction, up);
	glm::mat4 lightViewProjection = glm::perspective(mainLight.FovY, 1.0f, 0.01f, 100.0f);
	glm::mat4 lightWorldProjection = lightViewProjection * lightWorldView;

	glBindFramebuffer(GL_FRAMEBUFFER, mShadowFBO);
	glClearDepth(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, kShadowMapResolution, kShadowMapResolution);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(mShadowSlopeScaleBias, mShadowDepthBias);

	for (uint32_t instanceID : mScene->Instances)
	{
		const Instance* instance = &mScene->Instances[instanceID];
		const Mesh* mesh = &mScene->Meshes[instance->MeshID];
		const Transform* transform = &mScene->Transforms[instance->TransformID];
		//added
		glm::mat4 modelWorld;
		for (const Transform* curr_transform = transform; true; curr_transform = &mScene->Transforms[curr_transform->ParentID]) {
			modelWorld = translate(-transform->RotationOrigin) * modelWorld;
			modelWorld = mat4_cast(transform->Rotation) * modelWorld;
			modelWorld = translate(transform->RotationOrigin) * modelWorld;
			modelWorld = scale(transform->Scale) * modelWorld;
			modelWorld = translate(transform->Translation) * modelWorld;
			if (curr_transform->ParentID == -1) {
				break;
			}
		}
		glm::mat4 modelViewProjection = lightWorldProjection * modelWorld;//modelVP in shadow.vert
		glProgramUniformMatrix4fv(*mShadowSP, SCENE_MODELVIEWPROJECTION_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(modelViewProjection));
		glBindVertexArray(mesh->MeshVAO);
		for (size_t meshDrawIdx = 0; meshDrawIdx < mesh->DrawCommands.size(); meshDrawIdx++)
		{
			const GLDrawElementsIndirectCommand* drawCmd = &mesh->DrawCommands[meshDrawIdx];
			glDrawElementsBaseVertex(GL_TRIANGLES, drawCmd->count, GL_UNSIGNED_INT, (GLvoid*)(sizeof(GLuint) * drawCmd->firstIndex), drawCmd->baseVertex);
			// TODO: Draw every object in the scene,
			// the same way as the scene rendering pass, but you don't need any material properties.
			// One major different is your viewprojection matrix is now based on the light,
			// rather than the camera.
		}
	}

	glBindVertexArray(0);
	glPolygonOffset(0.0f, 0.0f);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	//Geometry pass
	if (*mGbuffer)
	{
		glUseProgram(*mGbuffer);

		glm::vec3 eye = mainCamera.Eye;
		glm::vec3 up = mainCamera.Up;

		glm::mat4 worldView = glm::lookAt(eye, eye + mainCamera.Look, up);
		glm::mat4 viewProjection = glm::perspective(mainCamera.FovY, (float)mBackbufferWidth / mBackbufferHeight, 0.01f, 100.0f);
		glm::mat4 worldProjection = viewProjection * worldView;
		GLint GBUFFER_MODELWORLD_UNIFORM_LOCATION = glGetUniformLocation(*mGbuffer, "ModelWorld");
		GLint GBUFFER_NORMAL_MODELWORLD_UNIFORM_LOCATION = glGetUniformLocation(*mGbuffer, "Normal_ModelWorld");
		GLint GBUFFER_MODELVIEWPROJECTION_UNIFORM_LOCATION = glGetUniformLocation(*mGbuffer, "ModelViewProjection");
		GLint GBUFFER_HAS_BUMP_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mGbuffer, "HasBumpMap");
		GLint GBUFFER_BUMP_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mGbuffer, "BumpMap");
		GLint GBUFFER_HAS_DIFFUSE_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mGbuffer, "HasDiffuseMap");
		GLint GBUFFER_DIFFUSE_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mGbuffer, "DiffuseMap");
		GLint GBUFFER_DIFFUSE_UNIFORM_LOCATION = glGetUniformLocation(*mGbuffer, "Diffuse");


		glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
		glViewport(0, 0, mBackbufferWidth, mBackbufferHeight);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glEnable(GL_DEPTH_TEST);
		for (uint32_t instanceID : mScene->Instances)
		{
			const Instance* instance = &mScene->Instances[instanceID];
			const Mesh* mesh = &mScene->Meshes[instance->MeshID];
			const Transform* transform = &mScene->Transforms[instance->TransformID];
			//added

			glm::mat4 modelWorld;
			for (const Transform* curr_transform = transform; true;) {
				modelWorld = translate(-transform->RotationOrigin) * modelWorld;
				modelWorld = mat4_cast(transform->Rotation) * modelWorld;
				modelWorld = translate(transform->RotationOrigin) * modelWorld;
				modelWorld = scale(transform->Scale) * modelWorld;
				modelWorld = translate(transform->Translation) * modelWorld;

				if (curr_transform->ParentID == -1) {
					break;
				}
				curr_transform = &mScene->Transforms[curr_transform->ParentID];
			}
			glm::mat3 normal_ModelWorld;
			normal_ModelWorld = mat3_cast(transform->Rotation) * normal_ModelWorld;
			normal_ModelWorld = glm::mat3(scale(1.0f / transform->Scale)) * normal_ModelWorld;

			glm::mat4 modelViewProjection = worldProjection * modelWorld;

			glProgramUniformMatrix4fv(*mGbuffer, GBUFFER_MODELWORLD_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(modelWorld));
			glProgramUniformMatrix3fv(*mGbuffer, GBUFFER_NORMAL_MODELWORLD_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(normal_ModelWorld));
			glProgramUniformMatrix4fv(*mGbuffer, GBUFFER_MODELVIEWPROJECTION_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(modelViewProjection));
			glBindVertexArray(0);

			glBindVertexArray(mesh->MeshVAO);

			for (size_t meshDrawIdx = 0; meshDrawIdx < mesh->DrawCommands.size(); meshDrawIdx++)
			{
				const GLDrawElementsIndirectCommand* drawCmd = &mesh->DrawCommands[meshDrawIdx];
				const Material* material = &mScene->Materials[mesh->MaterialIDs[meshDrawIdx]];
				// fake normal texture
				glActiveTexture(GL_TEXTURE0);
				glProgramUniform1i(*mGbuffer, GBUFFER_BUMP_MAP_UNIFORM_LOCATION, SCENE_BUMP_MAP_TEXTURE_BINDING);
				if ((int)material->BumpMapID == -1)
				{
					glBindTexture(GL_TEXTURE_2D, 0);
					glProgramUniform1i(*mGbuffer, GBUFFER_HAS_BUMP_MAP_UNIFORM_LOCATION, 0);
				}
				else
				{
					const BumpMap* bumpMap = &mScene->BumpMaps[material->BumpMapID];
					glBindTexture(GL_TEXTURE_2D, bumpMap->BumpMapTO);
					glProgramUniform1i(*mGbuffer, GBUFFER_HAS_BUMP_MAP_UNIFORM_LOCATION, 1);
				}
				
				glActiveTexture(GL_TEXTURE0 + 1);
				glProgramUniform1i(*mGbuffer, GBUFFER_DIFFUSE_MAP_UNIFORM_LOCATION, SCENE_DIFFUSE_MAP_TEXTURE_BINDING);
				if ((int)material->DiffuseMapID == -1)
				{
					glBindTexture(GL_TEXTURE_2D, 0);
					glProgramUniform1i(*mGbuffer, GBUFFER_HAS_DIFFUSE_MAP_UNIFORM_LOCATION, 0);
				}
				else
				{
					const DiffuseMap* diffuseMap = &mScene->DiffuseMaps[material->DiffuseMapID];
					glBindTexture(GL_TEXTURE_2D, diffuseMap->DiffuseMapTO);
					glProgramUniform1i(*mGbuffer, GBUFFER_HAS_DIFFUSE_MAP_UNIFORM_LOCATION, 1);
				}
				glProgramUniform3fv(*mGbuffer, GBUFFER_DIFFUSE_UNIFORM_LOCATION, 1, material->Diffuse);
				glDrawElementsBaseVertex(GL_TRIANGLES, drawCmd->count, GL_UNSIGNED_INT, (GLvoid*)(sizeof(GLuint) * drawCmd->firstIndex), drawCmd->baseVertex);
			}
			glBindVertexArray(0);
		}
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_FRAMEBUFFER_SRGB);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);
	}
	//SSAO
	if (*mSSAO) {
		glUseProgram(*mSSAO);
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, noiseTexture);
		GLint SSAO_gPOSITION = glGetUniformLocation(*mSSAO, "gPosition");
		glUniform1i(SSAO_gPOSITION, 0);
		GLint SSAO_gNORMAL = glGetUniformLocation(*mSSAO, "gNormal");
		glUniform1i(SSAO_gNORMAL, 1);
		GLint SSAO_TEX_NOISE = glGetUniformLocation(*mSSAO, "texNoise");
		glUniform1i(SSAO_TEX_NOISE, 2); 
		GLint SSAO_KERNEL_SAMPLE = glGetUniformLocation(*mSSAO, "samples");
		glUniformMatrix3fv(SSAO_KERNEL_SAMPLE, 64, GL_FALSE, ssaoKernel);//check
		GLint SSAO_PROJECTION = glGetUniformLocation(*mSSAO, "projection");
		glm::mat4 viewProjection = glm::perspective(mainCamera.FovY, (float)mBackbufferWidth / mBackbufferHeight, 0.01f, 100.0f);//view space to clip space
		glUniformMatrix4fv(SSAO_PROJECTION, 1, GL_FALSE, glm::value_ptr(viewProjection));
		GLint SSAO_HEIGHT = glGetUniformLocation(*mSSAO, "Height");
		glProgramUniform1i(*mSceneSP, SSAO_HEIGHT, GLfloat(mBackbufferHeight));
		GLint SSAO_WIDTH = glGetUniformLocation(*mSSAO, "Width");
		glProgramUniform1i(*mSceneSP, SSAO_HEIGHT, GLfloat(mBackbufferWidth));
		glBindVertexArray(SSAOVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	//SSAO blur
	if (*mSSAOBlur) {
		glUseProgram(*mSSAOBlur);
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBuffer);
		GLint SSAOBLUR_INPUTS = glGetUniformLocation(*mSSAOBlur, "ssaoInput");//output from SSAO frag shader
		glUniform1i(SSAOBLUR_INPUTS, 0);
		glBindVertexArray(SSAOVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}

    // render scene
    if (*mSceneSP)
    {
        glUseProgram(*mSceneSP);
        // GL 4.1 = no shader-specified uniform locations. :( Darn you OSX!!!
        GLint SCENE_MODELWORLD_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "ModelWorld");
        GLint SCENE_NORMAL_MODELWORLD_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "Normal_ModelWorld");
        GLint SCENE_MODELVIEWPROJECTION_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "ModelViewProjection");
        GLint SCENE_CAMERAPOS_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "CameraPos");
        GLint SCENE_HAS_DIFFUSE_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "HasDiffuseMap");
		GLint SCENE_HAS_ALPHA_MASK_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "HasAlphaMask");
		GLint SCENE_HAS_SPECULAR_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "HasSpecularMap");
		GLint SCENE_HAS_BUMP_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "HasBumpMap");
        GLint SCENE_AMBIENT_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "Ambient");
        GLint SCENE_DIFFUSE_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "Diffuse");
        GLint SCENE_SPECULAR_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "Specular");
        GLint SCENE_SHININESS_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "Shininess");
        GLint SCENE_DIFFUSE_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "DiffuseMap");
		GLint SCENE_ALPHA_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "AlphaMap");
		GLint SCENE_SPECULAR_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "SpecularMap");
		GLint SCENE_BUMP_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "BumpMap");
		GLint SCENE_LIGHTMATRIX_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "lightMatrix");

        glm::vec3 eye = mainCamera.Eye;
        glm::vec3 up = mainCamera.Up;

        glm::mat4 worldView = glm::lookAt(eye, eye + mainCamera.Look, up);//world space to view space
        glm::mat4 viewProjection = glm::perspective(mainCamera.FovY, (float)mBackbufferWidth / mBackbufferHeight, 0.01f, 100.0f);//view space to clip space
        glm::mat4 worldProjection = viewProjection * worldView;

        glProgramUniform3fv(*mSceneSP, SCENE_CAMERAPOS_UNIFORM_LOCATION, 1, value_ptr(eye));

		bool gui = ImGui::Begin("Objects", 0, ImGuiWindowFlags_AlwaysAutoResize);
        glBindFramebuffer(GL_FRAMEBUFFER, mBackbufferFBO);
        glViewport(0, 0, mBackbufferWidth, mBackbufferHeight);
        glEnable(GL_FRAMEBUFFER_SRGB);
        glEnable(GL_DEPTH_TEST);
        for (uint32_t instanceID : mScene->Instances)
        {
            const Instance* instance = &mScene->Instances[instanceID];
            const Mesh* mesh = &mScene->Meshes[instance->MeshID];
            const Transform* transform = &mScene->Transforms[instance->TransformID];
			//added
			
			if (gui)
			{
				std::string label = std::string("Reset RO##") + std::to_string(instanceID);
				if (ImGui::Button(label.c_str()))
				{
					printf("asdf\n");
					((Transform*)transform)->RotationOrigin = glm::vec3(0.0f);
				}
			}

            glm::mat4 modelWorld;
			for (const Transform* curr_transform = transform; true;) {
				modelWorld = translate(-transform->RotationOrigin) * modelWorld;
				modelWorld = mat4_cast(transform->Rotation) * modelWorld;
				modelWorld = translate(transform->RotationOrigin) * modelWorld;
				modelWorld = scale(transform->Scale) * modelWorld;
				modelWorld = translate(transform->Translation) * modelWorld;
				
				if (curr_transform->ParentID == -1) {
					break;
				}
				curr_transform = &mScene->Transforms[curr_transform->ParentID];
			}
			//added2
			glm::mat4 lightOffsetMatrix = glm::mat4(
				0.5f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.5f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.5f, 0.0f,
				0.5f, 0.5f, 0.5f, 1.0f);
			glm::mat4 lightMatrix = lightOffsetMatrix * lightWorldProjection;
			lightMatrix = lightMatrix*modelWorld;
			glProgramUniformMatrix4fv(*mSceneSP, SCENE_LIGHTMATRIX_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(lightMatrix));//deliever into scene.vert

			if (gui)
			{
				ImGui::Text("%s", mesh->Name.c_str());
				ImGui::Text(
					"%f %f %f %f\n"
					"%f %f %f %f\n"
					"%f %f %f %f\n"
					"%f %f %f %f",
					modelWorld[0][0], modelWorld[0][1], modelWorld[0][2], modelWorld[0][3],
					modelWorld[1][0], modelWorld[1][1], modelWorld[1][2], modelWorld[1][3],
					modelWorld[2][0], modelWorld[2][1], modelWorld[2][2], modelWorld[2][3],
					modelWorld[3][0], modelWorld[3][1], modelWorld[3][2], modelWorld[3][3]);
				ImGui::Text("RO: %f %f %f", transform->RotationOrigin.x, transform->RotationOrigin.y, transform->RotationOrigin.z);
			}

            glm::mat3 normal_ModelWorld;
            normal_ModelWorld = mat3_cast(transform->Rotation) * normal_ModelWorld;
            normal_ModelWorld = glm::mat3(scale(1.0f / transform->Scale)) * normal_ModelWorld;

            glm::mat4 modelViewProjection = worldProjection * modelWorld;

            glProgramUniformMatrix4fv(*mSceneSP, SCENE_MODELWORLD_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(modelWorld));
            glProgramUniformMatrix3fv(*mSceneSP, SCENE_NORMAL_MODELWORLD_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(normal_ModelWorld));
            glProgramUniformMatrix4fv(*mSceneSP, SCENE_MODELVIEWPROJECTION_UNIFORM_LOCATION, 1, GL_FALSE, value_ptr(modelViewProjection));

            glBindVertexArray(mesh->MeshVAO);
			//added
			glActiveTexture(GL_TEXTURE0 + SCENE_SHADOW_MAP_TEXTURE_BINDING);
			GLint SCENE_SHADOW_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mSceneSP, "ShadowMap");
			glUniform1i(SCENE_SHADOW_MAP_UNIFORM_LOCATION, SCENE_SHADOW_MAP_TEXTURE_BINDING);
			glBindTexture(GL_TEXTURE_2D, mShadowDepthTO);

            for (size_t meshDrawIdx = 0; meshDrawIdx < mesh->DrawCommands.size(); meshDrawIdx++)
            {
                const GLDrawElementsIndirectCommand* drawCmd = &mesh->DrawCommands[meshDrawIdx];
                const Material* material = &mScene->Materials[mesh->MaterialIDs[meshDrawIdx]];
				//diffuse map
                glActiveTexture(GL_TEXTURE0 + SCENE_DIFFUSE_MAP_TEXTURE_BINDING);
                glProgramUniform1i(*mSceneSP, SCENE_DIFFUSE_MAP_UNIFORM_LOCATION, SCENE_DIFFUSE_MAP_TEXTURE_BINDING);
                if ((int)material->DiffuseMapID == -1)
                {
                    glBindTexture(GL_TEXTURE_2D, 0);
                    glProgramUniform1i(*mSceneSP, SCENE_HAS_DIFFUSE_MAP_UNIFORM_LOCATION, 0);
                }
                else
                {
                    const DiffuseMap* diffuseMap = &mScene->DiffuseMaps[material->DiffuseMapID];
                    glBindTexture(GL_TEXTURE_2D, diffuseMap->DiffuseMapTO);
                    glProgramUniform1i(*mSceneSP, SCENE_HAS_DIFFUSE_MAP_UNIFORM_LOCATION, 1);
                }
				//alpha masking
				glActiveTexture(GL_TEXTURE0 + SCENE_ALPHA_MAP_TEXTURE_BINDING);
				glProgramUniform1i(*mSceneSP, SCENE_ALPHA_MAP_UNIFORM_LOCATION, SCENE_ALPHA_MAP_TEXTURE_BINDING);
				if ((int)material->AlphaMapID == -1)
				{
					glBindTexture(GL_TEXTURE_2D, 0);
					glProgramUniform1i(*mSceneSP, SCENE_HAS_ALPHA_MASK_UNIFORM_LOCATION, 0);
				}
				else
				{
					const AlphaMap* alphaMap = &mScene->AlphaMaps[material->AlphaMapID];
					glBindTexture(GL_TEXTURE_2D, alphaMap->AlphaMapTO);
					glProgramUniform1i(*mSceneSP, SCENE_HAS_ALPHA_MASK_UNIFORM_LOCATION, 1);
				}
				
				// specular map
				glActiveTexture(GL_TEXTURE0 + SCENE_SPECULAR_MAP_TEXTURE_BINDING);
				glProgramUniform1i(*mSceneSP, SCENE_SPECULAR_MAP_UNIFORM_LOCATION, SCENE_SPECULAR_MAP_TEXTURE_BINDING);
				if ((int)material->SpecularMapID == -1)
				{
					glBindTexture(GL_TEXTURE_2D, 0);
					glProgramUniform1i(*mSceneSP, SCENE_HAS_SPECULAR_MAP_UNIFORM_LOCATION, 0);
				}
				else
				{
					const SpecularMap* specularMap = &mScene->SpecularMaps[material->SpecularMapID];
					glBindTexture(GL_TEXTURE_2D, specularMap->SpecularMapTO);
					glProgramUniform1i(*mSceneSP, SCENE_HAS_SPECULAR_MAP_UNIFORM_LOCATION, 1);
				}
				
				// bump map
				glActiveTexture(GL_TEXTURE0 + SCENE_BUMP_MAP_TEXTURE_BINDING);
				glProgramUniform1i(*mSceneSP, SCENE_BUMP_MAP_UNIFORM_LOCATION, SCENE_BUMP_MAP_TEXTURE_BINDING);
				if ((int)material->BumpMapID == -1)
				{
					glBindTexture(GL_TEXTURE_2D, 0);
					glProgramUniform1i(*mSceneSP, SCENE_HAS_BUMP_MAP_UNIFORM_LOCATION, 0);
				}
				else
				{
					const BumpMap* bumpMap = &mScene->BumpMaps[material->BumpMapID];
					glBindTexture(GL_TEXTURE_2D, bumpMap->BumpMapTO);
					glProgramUniform1i(*mSceneSP, SCENE_HAS_BUMP_MAP_UNIFORM_LOCATION, 1);
				}

                glProgramUniform3fv(*mSceneSP, SCENE_AMBIENT_UNIFORM_LOCATION, 1, material->Ambient);
                glProgramUniform3fv(*mSceneSP, SCENE_DIFFUSE_UNIFORM_LOCATION, 1, material->Diffuse);
                glProgramUniform3fv(*mSceneSP, SCENE_SPECULAR_UNIFORM_LOCATION, 1, material->Specular);
                glProgramUniform1f(*mSceneSP, SCENE_SHININESS_UNIFORM_LOCATION, material->Shininess);
				
                glDrawElementsBaseVertex(GL_TRIANGLES, drawCmd->count, GL_UNSIGNED_INT, (GLvoid*)(sizeof(GLuint) * drawCmd->firstIndex), drawCmd->baseVertex);
            }
            glBindVertexArray(0);
        }

		ImGui::End();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_FRAMEBUFFER_SRGB);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glUseProgram(0);
    }
	
    
	//skybox
	if (*mSky)
	{
	 // Draw skybox
		glUseProgram(*mSky);
        glDepthMask(GL_FALSE);

		glBindFramebuffer(GL_FRAMEBUFFER, mBackbufferFBO);
		glViewport(0, 0, mBackbufferWidth, mBackbufferHeight);
		glEnable(GL_FRAMEBUFFER_SRGB);

		glm::vec3 eye = mainCamera.Eye;
		glm::vec3 up = mainCamera.Up;

		glm::mat4 worldView = glm::lookAt(eye, eye + mainCamera.Look, up);	// Remove any translation component of the view matrix
		glm::mat4 viewProjection = glm::perspective(mainCamera.FovY, (float)mBackbufferWidth / mBackbufferHeight, 0.01f, 100.0f);
		glm::mat4 worldProjection = glm::mat4(glm::mat3(viewProjection * worldView));
		GLint SCENE_WORLDPROJECTION_UNIFORM_LOCATION = glGetUniformLocation(*mSky, "worldProjection");
        glUniformMatrix4fv(SCENE_WORLDPROJECTION_UNIFORM_LOCATION, 1, GL_FALSE, glm::value_ptr(worldProjection));
        // skybox cube
        glBindVertexArray(skyboxVAO);//now trying render into skybox
        glActiveTexture(GL_TEXTURE0 + SCENE_SKYBOX_TEXTURE_BINDING);//should be GL_TEXTURE0 + SCENE_SKYBOX_TEXTURE_BINDING(defined in preamble.glsl)
        glUniform1i(glGetUniformLocation(*mSky, "skybox"), SCENE_SKYBOX_TEXTURE_BINDING);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mScene->cubemapTexture);//passing the texture to 

		//added
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        glDrawArrays(GL_TRIANGLES, 0, 36);
		glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		glDepthFunc(GL_LESS);
		glDisable(GL_DEPTH_TEST);

        glBindVertexArray(0);
		glDisable(GL_FRAMEBUFFER_SRGB);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDepthMask(GL_TRUE);
	}

	//bloom
	if (*mBlur)
	{
		glUseProgram(*mBlur);
		GLboolean horizontal = true, first_iteration = true;
		GLuint amount = 10;
		GLint SCENE_IMAGE_UNIFORM_LOCATION = glGetUniformLocation(*mBlur, "image");
		glBindVertexArray(pingpongVAO);
		for (GLuint i = 0; i < amount; i++)
		{

			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(
				GL_TEXTURE_2D, first_iteration ? mBackbufferColorTO2 : pingpongBuffer[!horizontal]
			); 
			glUniform1i(SCENE_IMAGE_UNIFORM_LOCATION, 0);
			glUniform1i(glGetUniformLocation(*mBlur, "horizontal"), horizontal);
			//RenderQuad();
			glDrawArrays(GL_TRIANGLES, 0, 6);
			horizontal = !horizontal;
			if (first_iteration)
				first_iteration = false;
		}
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	if (*mComposite) {
		//added code
		glUseProgram(*mComposite);
		glBindVertexArray(compositeVAO);
		glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);


		GLint COMPOSITE_BLOOMBLUR_UNIFORM_LOCATION = glGetUniformLocation(*mComposite, "bloomBlur");
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, pingpongBuffer[0]);
		glUniform1i(COMPOSITE_BLOOMBLUR_UNIFORM_LOCATION, 0);


		GLint COMPOSITE_IMAGE1_UNIFORM_LOCATION = glGetUniformLocation(*mComposite, "scene");
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, mBackbufferColorTO);
		glUniform1i(COMPOSITE_IMAGE1_UNIFORM_LOCATION, 1);


		GLuint COMPOSITE_SSAO_UNIFORM_LOCATION = glGetUniformLocation(*mComposite, "ssao");//SSAO
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, ssaoColorBufferBlur);
		glUniform1i(COMPOSITE_SSAO_UNIFORM_LOCATION, 2);

		GLuint COMPOSITE_DIFFUSE_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mComposite, "DiffuseMap");
		glActiveTexture(GL_TEXTURE0 + 3);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		glUniform1i(COMPOSITE_DIFFUSE_MAP_UNIFORM_LOCATION, 3);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


	if (mShowDepthVis && *mDepthVisSP)
	{
		glUseProgram(*mDepthVisSP);
		GLint DEPTHVIS_TRANSFORM2D_UNIFORM_LOCATION = glGetUniformLocation(*mDepthVisSP,
			"Transform2D");
		GLint DEPTHVIS_ORTHO_PROJECTION_UNIFORM_LOCATION = glGetUniformLocation(*mDepthVisSP,
			"OrthoProjection");
		GLint DEPTHVIS_DEPTH_MAP_UNIFORM_LOCATION = glGetUniformLocation(*mDepthVisSP,
			"DepthMap");
		glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
		glViewport(0, 0, mBackbufferWidth, mBackbufferHeight);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		float depthBufferVisSize = 256.0f;
		glm::mat4 transform2D =
			glm::translate(glm::vec3((float)(mBackbufferWidth - depthBufferVisSize),
			(float)(mBackbufferHeight - depthBufferVisSize), 0.0f)) *
			glm::scale(glm::vec3(depthBufferVisSize, depthBufferVisSize, 0.0f));
		glProgramUniformMatrix4fv(*mDepthVisSP, DEPTHVIS_TRANSFORM2D_UNIFORM_LOCATION, 1,
			GL_FALSE, value_ptr(transform2D));
		glm::mat4 ortho = glm::ortho(0.0f, (float)mBackbufferWidth, 0.0f,
			(float)mBackbufferHeight);
		glUniformMatrix4fv(DEPTHVIS_ORTHO_PROJECTION_UNIFORM_LOCATION, 1, GL_FALSE,
			value_ptr(ortho));
		glActiveTexture(GL_TEXTURE0 + DEPTHVIS_DEPTH_MAP_TEXTURE_BINDING);
		glUniform1i(DEPTHVIS_DEPTH_MAP_UNIFORM_LOCATION, DEPTHVIS_DEPTH_MAP_TEXTURE_BINDING);
		glBindTexture(GL_TEXTURE_2D, mShadowDepthTO);
		// need to disable depth comparison before sampling with non-shadow sampler
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
		glBindVertexArray(mNullVAO);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindVertexArray(0);
		// re-enable depth comparison
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glDisable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ZERO);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(0);
	}


    // Render ImGui
    {
		
        glBindFramebuffer(GL_FRAMEBUFFER, compositeFBO);
        ImGui::Render();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // copy to window
    {
		//glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
		//glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
		//glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, compositeFBO);
		//glBindFramebuffer(GL_READ_FRAMEBUFFER, pingpongFBO[1]);
        //glBindFramebuffer(GL_READ_FRAMEBUFFER, mBackbufferFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(
            0, 0, mBackbufferWidth, mBackbufferHeight,
            0, 0, mBackbufferWidth, mBackbufferHeight,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void* Renderer::operator new(size_t sz)
{
    // zero out the memory initially, for convenience.
    void* mem = ::operator new(sz);
    memset(mem, 0, sz);
    return mem;
}


/*
1. create shadow map depth texture.
2. attach depth texture to framebuffer.
3. write new shader for the shadow map rendering
4. The first pass renders the shadow map¡¯s depth texture by rendering the scene from the point
of view of the light. The second pass renders the scene while using the shadow map depth texture as
input, which allows you to compute whether points are in shadow or not.
5. calculate clip space cords using light matrix in the scene shader, pass it to frag shader
6. sample the shadow map to determine if in shadow or not.
*/
# sponza

Sponza is a real-time renderer.
Author: Haotian Shen

Features:

1. Deferred shading: accelerate by storing objects per fragment detail as texture for gpu to use.

2. Shadow: done without a light pass, calculate the z value and keep the smallest (nearest) z value available.

3. Normal Mapping: for advanced lighting, normal map is used to calculate per fragment normal rather than the per face normal.

4. Bloom Blur: the original FBO is devided into two parts: HDR image and hight-lighted parts. A pingpong buffer is used to blur the image (one can set how many times it should be blurred). The blured image is then merged with the hdr image.

5. Sky box: it is really a cube that covers the whole scene, the distance between viewer and the skybox does not change since the z value for the skybox surface is always 1.

6. SSAO (80% implemented): use kernel in tangent space to stimulate noise. Generate a number of samples oriented along the normal of a surface(normal-oriented hemisphere). As we know if a point is inside the nearby object, we claim that the more points inside nearby object found, the ambient light at that fragment is more likely to be darker. And thus the shadow caused by nearby object could be created.

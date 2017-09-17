Real Time Rendering A2 - Danyon Edwards - s3547463

Run instructions:
$ make
$ ./a2

Commands
a - toggle animation
c - console
f - toggle gpu (fixed)/cpu lighting calculation mode
g - toggle shaders
l - lighting
m - Blinn-Phong or Phong specular lighting model
n - render normals: as line segments in fixed pipeline mode but as colours in programable pipeline i.e. shader mode
o - cycle through OSD options
p - per (vertex, pixel) lighting
s - cycle through shapes (grid, wave)
v - toggle VBOs
4 - toggle single/multi view
+/- - increase/decrease tesselation
w - wireframe (line) or fill mode
z - 2D/3D wave
1 - toggle ambient (No shaders + not fixed)
2 - toggle diffuse (No shaders + not fixed)
3 - toggle specular (No shaders + not fixed)

Working:
* Shader with per vertex ADS
* Performance meter OSD + console
* normalMatrix and modelViewMatrix passed from cpu to gpu (no built-in gl)
* Blinn-phong on both vertex and fragment shader
* Per pixel lighting using Blinn-Phong
* Shader based wave calculations
* Shader based normal calculations
* Shininess control

Partially working
* VBO
-> Is only working when shader is enabled.
-> Has an issue causing the end vertex interpolating with the center vertex (0, 0, 0)
-> This is causing a blanket effect

Not included
* Positional lighting
* Phong lighting


QUESTIONS!
1. <Is there any visual difference between fixed pipeline and shader lighting? Should there be?>
Yes, there is a difference between the two. When the lighting is calculated on
the GPU through the shader program, it is providing a sharper and smoother lighting
effect. This is making the specular more defined and decreasing the illumination around the specular.
This should happen as GPU's are developed to perform complex calculations in comparison to a CPU where the
fixed pipeline is being processed.

2.<Is there any performance difference between fixed pipeline and using shader lighting?>
Yes. There is a massive difference between the two in relation to frame rates. With a lower tesselation,
The difference is not as noticeable, but with more vertices to calculate, the GPU shows a greater
overall performance. At a tesselation of 1024X1024, using shader lighting decreases the frametime by 1/5th of
that seen in the fixed pipeline.

3.<What overhead/slowdown factor is there for performing animation compared to
static geometry using the vertex shader? For static i.e. geometry is it worth
pre-computing and storing the sine wave values in buffers?>
There is little to no difference performing animation compared to static geometry.
As both are being calculated in immediate mode, they are performing an identical number
of calculations each frame. It is more beneficial to use VBO's with animation as
the x and z positions remain static, therefore only the y position is required to be
calculated. This removes 2 unneccasary calculations per vertex in comparison to
immediate mode. When using VBO's frame rate decreased by 20% at a tesselation of 1024 when
animation was active. With static geometry however, there was no difference.

4. <Is there any difference in performance between per vertex and per pixel shader based lighting?>
There wasn't a difference when using per pixel lighting. I'm not sure if this because of my methods of
doing it or it's just a surprising result. I initially thought that performing interpolation would
require more work, however it seems as though it's very little when comparing to that of per-vertex lighting.
This may be because the fragment shader and the vertex shader are both using lighting calculations
that require the same amount of work to be done. E.g. calculation Blinn-Phong on both shaders.

5. <What is the main visual difference between Phong and Blinn-Phong lighting?>
Although I didn't get this implemented, through research I've noticed some key differences.
The most noticeable is the softer or more precise specular that is produced. The Phong
Technique also produces a more circular specular that results in noticeable curves. Whereas the blending of
specular and ambient with blinn-phong produces a softer result.

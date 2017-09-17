/*
* Simple 3D sine wave animation example using glm
* $Id: sinewave3D-glm.cpp,v 1.8 2017/08/23 12:56:02 gl Exp gl $
*/

/* To compile
clang -o sine_wave shaders.c sine_wave.cpp -framework Carbon -framework OpenGL -framework GLUT -Wno-deprecated
*/

#include <stdbool.h>
#include <stdio.h>
#include <math.h>

#if __APPLE__
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#endif

#define BUFFER_OFFSET(i) ((void*)(i))

#include "shaders.h"
#define GL_GLEXT_PROTOTYPES

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>

void buildVBO(int tess);

typedef enum {
    d_drawSineWave,
    d_mouse,
    d_key,
    d_animation,
    d_lighting,
    d_OSD,
    d_matrices,
    d_computeLighting,
    d_nflags
} DebugFlags;

bool debug[d_nflags] = { false, false, false, false, false, false, false, false };

float epsilon = std::numeric_limits<float>::epsilon();
typedef struct { float r, g, b; } color3f;

typedef enum { line, fill } polygonMode_t;
typedef enum { grid, wave } shape_t;

typedef struct {
    shape_t shape;
    bool animate;
    float t, lastT;
    polygonMode_t polygonMode;
    bool lighting;
    bool fixed;
    bool twoside;
    bool drawNormals;
    int width, height;
    int tess;
    int waveDim;
    int frameCount;
    float frameRate;
    float displayStatsInterval;
    int lastStatsDisplayT;
    bool displayOSD;
    bool consolePM;
    bool multiView;
    bool diffuse;
    bool specular;
    bool ambient;
    bool shader;
    bool lightingMode; // T Per-Pixel, F Per-Vertex
    bool vbo;
    bool lightingModel; // T blinn-Phong, F = Phong
    bool lightingPos; // T directional, F = positional.
} Global;
Global g = { wave, false, 0.0, 0.0, fill, true, false, false, false, 0, 0, 128, 3, 0, 0.0, 1.0, 0, false, true, false, true, true, true, false, false, false, true, true};

typedef struct {
    float x, y, z;
} Vec3f;

typedef struct {
    Vec3f pos, normal;
} Vertex;

typedef struct {
    Vertex* verts;
    unsigned int* indices;
    size_t numVerts, numIndices;
    GLuint vbo, ibo, tbo;
} Mesh;

Mesh* gridMesh;

typedef enum { inactive, rotate, pan, zoom } CameraControl;

struct camera_t {
    int lastX, lastY;
    float rotateX, rotateY;
    float scale;
    CameraControl control;
} camera = { 0, 0, 30.0, -30.0, 1.0, inactive };

glm::vec3 cyan(0.0, 1.0, 1.0);
glm::vec3 magenta(1.0, 0.0, 1.0);
glm::vec3 yellow(1.0, 1.0, 0.0);
glm::vec3 white(1.0, 1.0, 1.0);
glm::vec3 grey(0.8, 0.8, 0.8);
glm::vec3 black(0.0, 0.0, 0.0);
float shininess = 50.0;

const float milli = 1000.0;

glm::mat4 modelViewMatrix;
glm::mat3 normalMatrix;

int err;

GLuint shaderProgram;
//GLuint shaderProgram2;

void printVec(float *v, int n)
{
    int i;

    for (i = 0; i < n; i++)
    printf("%5.3f ", v[i]);
    printf("\n");
}

void printMatrixLinear(float *m, int n)
{
    int i;

    for (i = 0; i < n; i++)
    printf("%5.3f ", m[i]);
    printf("\n");
}

void printMatrixColumnMajor(float *m, int n)
{
    int i, j;

    for (j = 0; j < n; j++) {
        for (i = 0; i < n; i++) {
            printf("%5.3f ", m[i*4+j]);
        }
        printf("\n");
    }
    printf("\n");
}

Mesh* createMesh(size_t numVerts, size_t numIndices){
    Mesh* mesh = (Mesh*) malloc(sizeof(Mesh));
    mesh->numVerts = numVerts;
    mesh->numIndices = numIndices;
    mesh->verts = (Vertex*) calloc(numVerts, sizeof(Vertex));
    mesh->indices = (unsigned int*) calloc(numIndices, sizeof(int));

    return mesh;
}

void bindVBOs(GLuint vbo, GLuint ibo){
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
}

void unbindVBOs(){
    int buffer;

    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buffer);
    if (buffer != 0) glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &buffer);
    if (buffer != 0)glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisableClientState(GL_VERTEX_ARRAY);
}

void useVBO(){
    bindVBOs(gridMesh->vbo, gridMesh->ibo);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glPushAttrib(GL_CURRENT_BIT);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), BUFFER_OFFSET(0));
    glNormalPointer(GL_FLOAT, sizeof(Vertex), BUFFER_OFFSET(sizeof(Vec3f)));

    glDrawElements(GL_TRIANGLES, gridMesh->numIndices, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    glPopAttrib();
    unbindVBOs();
}

void buildVBO(int tess){
    gridMesh = createMesh((tess + 1) * (tess * 1), tess * tess * 6);
    float height = 2;
    float width = 2;
    float cols = tess;
    float rows = tess;

    // Vertices
    float x, x0 = 0.5 * width, dx = width / (float)cols;
    float z, z0 = 0.5 * height, dz = height / (float)rows;
    for (size_t i = 0; i <= cols; ++i) {
        x = i * dx - x0;
        for (size_t j = 0; j <= rows; ++j) {
            z = j * dz - z0;
            size_t index = i * (rows + 1) + j;
            gridMesh->verts[index].pos = (Vec3f) { x, 0, z };
            gridMesh->verts[index].normal.y = 1.0;
        }
    }

    // Indices i.e. elements
    size_t index = 0;
    for (size_t j = 0; j < cols; ++j) {
        for (size_t i = 0; i < rows ; ++i) {
            gridMesh->indices[index++] = j * (rows + 1) + i;
            gridMesh->indices[index++] = (j + 1) * (rows + 1) + i;
            gridMesh->indices[index++] = j * (rows + 1) + i + 1;
            gridMesh->indices[index++] = j * (rows + 1) + i + 1;
            gridMesh->indices[index++] = (j + 1) * (rows + 1) + i;
            gridMesh->indices[index++] = (j + 1) * (rows + 1) + i + 1;
        }
    }

    // BUILD THE VBO
    glGenBuffers(1, &gridMesh->vbo);
    glGenBuffers(1, &gridMesh->ibo);
    bindVBOs(gridMesh->vbo, gridMesh->ibo);
    glBufferData(GL_ARRAY_BUFFER, gridMesh->numVerts * sizeof(Vertex), gridMesh->verts, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, gridMesh->numIndices * sizeof(unsigned int), gridMesh->indices, GL_STATIC_DRAW);
    unbindVBOs();
}

void init(void)
{
    /* Orginal shader program
    shaderProgram = getShader("shader.vert", "shader.frag");
    shaderProgram2 = getShader("shader2.vert", "shader2.frag");
    */
    #if __APPLE__
    #else
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    #endif

    shaderProgram = getShader("shader.vert", "shader.frag");
    // printf("Shaderprogram: %i\n", shaderProgram);
    //GLint uni_loc = glGetUniformLocation(shaderProgram, "color"); // Uniform location


    glClearColor(0.0, 0.0, 0.0, 1.0);
    if (g.twoside)
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    buildVBO(g.tess);
}

void reshape(int w, int h)
{
    g.width = w;
    g.height = h;
    glViewport(0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -100.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void drawAxes(float length)
{
    glm::vec4 v;

    glPushAttrib(GL_CURRENT_BIT);
    glBegin(GL_LINES);

    /* x axis */
    glColor3f(1.0, 0.0, 0.0);
    v = modelViewMatrix * glm::vec4(-length, 0.0, 0.0, 1.0);
    glVertex3fv(&v[0]);
    v = modelViewMatrix * glm::vec4(length, 0.0, 0.0, 1.0);
    glVertex3fv(&v[0]);

    /* y axis */
    glColor3f(0.0, 1.0, 0.0);
    v = modelViewMatrix * glm::vec4(0.0, -length, 0.0, 1.0);
    glVertex3fv(&v[0]);
    v = modelViewMatrix * glm::vec4(0.0, length, 0.0, 1.0);
    glVertex3fv(&v[0]);

    /* z axis */
    glColor3f(0.0, 0.0, 1.0);
    v = modelViewMatrix * glm::vec4(0.0, 0.0, -length, 1.0);
    glVertex3fv(&v[0]);
    v = modelViewMatrix * glm::vec4(0.0, 0.0, length, 1.0);
    glVertex3fv(&v[0]);

    glEnd();
    glPopAttrib();
}

void drawVector(glm::vec3 & o, glm::vec3 & v, float s, bool normalize, glm::vec3 & c)
{
    glPushAttrib(GL_CURRENT_BIT);
    glColor3fv(&c[0]);
    glBegin(GL_LINES);
    if (normalize)
    v = glm::normalize(v);

    glVertex3fv(&o[0]);
    glm::vec3 e(o + s * v);
    glVertex3fv(&e[0]);
    glEnd();
    glPopAttrib();
}


// On screen display
void displayOSD()
{
    char buffer[30];
    char *bufp;
    int w, h;

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    /* Set up orthographic coordinate system to match the window,
    i.e. (0,0)-(w,h) */
    w = glutGet(GLUT_WINDOW_WIDTH);
    h = glutGet(GLUT_WINDOW_HEIGHT);
    glOrtho(0.0, w, 0.0, h, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    /* Frame rate */
    glColor3f(1.0, 1.0, 0.0);
    glRasterPos2i(10, 60);
    snprintf(buffer, sizeof buffer, "frame rate (f/s):  %5.0f", g.frameRate);
    for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);

    /* Frame time */
    glColor3f(1.0, 1.0, 0.0);
    glRasterPos2i(10, 40);
    snprintf(buffer, sizeof buffer, "frame time (ms/f): %5.0f", 1.0 / g.frameRate * milli);
    for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);

    /* Tesselation */
    glColor3f(1.0, 1.0, 0.0);
    glRasterPos2i(10, 20);
    snprintf(buffer, sizeof buffer, "tesselation:       %5d", g.tess);
    for (bufp = buffer; *bufp; bufp++)
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *bufp);

    glPopMatrix();  /* Pop modelview */
    glMatrixMode(GL_PROJECTION);

    glPopMatrix();  /* Pop projection */
    glMatrixMode(GL_MODELVIEW);

    glPopAttrib();
}


/* Perform ADS - ambient, diffuse and specular - lighting calculation
* in eye coordinates (EC).
*/
glm::vec3 computeLighting(glm::vec3 & rEC, glm::vec3 & nEC)
{
    if (debug[d_computeLighting]) {
        printf("rEC ");
        printVec(&rEC[0], 3);
        printf("nEC ");
        printVec(&nEC[0], 3);
    }

    // Used to accumulate ambient, diffuse and specular contributions
    // Note: it is a vec3 being constructed with a single value which
    // is used for all 3 components
    glm::vec3 color(0.0);
    if (g.ambient){
        // Ambient contribution: A=LaÃ—Ma
        // Default light ambient color and default ambient material color
        // are both (0.2, 0.2, 0.2)
        glm::vec3 La(0.2);
        glm::vec3 Ma(0.2);
        glm::vec3 ambient(La * Ma);
        color += ambient;
    }
    // Light direction vector. Default for LIGHT0 is a directional light
    // along z axis for all vertices, i.e. <0, 0, 1>
    glm::vec3 lEC(0.5);
    if(g.lightingModel == 1.0) lEC = lEC;
    else lEC = normalize(lEC);

    // Test if normal points towards light source, i.e. if polygon
    // faces toward the light - if not then no diffuse or specular
    // contribution
    float dp = glm::dot(nEC, lEC);
    if (dp > 0.0) {
        // Calculate diffuse and specular contribution
        if(g.diffuse){
            // Lambert diffuse: D=LdÃ—MdÃ—cosÎ¸
            // Ld: default diffuse light color for GL_LIGHT0 is white (1.0, 1.0, 1.0).
            // Md: default diffuse material color is grey (0.8, 0.8, 0.8).
            glm::vec3 Ld(1.0);
            glm::vec3 Md(0.0, 0.5, 0.5);
            // Need normalized normal to calculate cosÎ¸,
            // light vector <0, 0, 1> is already normalized
            nEC = glm::normalize(nEC);
            float NdotL = glm::dot(nEC, lEC);
            glm::vec3 diffuse(Ld * Md * NdotL);
            color += diffuse;
        }
        // Blinn-Phong specular: S=LsÃ—MsÃ—cosâ¿Î±
        // Ls: default specular light color for LIGHT0 is white (1.0, 1.0, 1.0)
        // Ms: specular material color, also set to white (1.0, 1.0, 1.0),
        // but default for fixed pipeline is black, which means can't see
        // specular reflection. Need to set it to same value for fixed
        // pipeline lighting otherwise will look different.
        if(g.specular){
            glm::vec3 Ls(1.0);
            glm::vec3 Ms(0.8);
            // Default viewer is at infinity along z axis <0, 0, 1> i.e. a
            // non local viewer (see glLightModel and GL_LIGHT_MODEL_LOCAL_VIEWER)
            glm::vec3 vEC(0.0, 0.0, 1.0);
            // Blinn-Phong half vector (using a single capital letter for
            // variable name!). Need normalized H (and nEC, above) to calculate cosÎ±.
            glm::vec3 specular;
            if(g.lightingModel == 1.0){ // Blinn Phong
                glm::vec3 H = glm::vec3(lEC + vEC);
                H = normalize(H);
                float NdotH = dot(nEC, H);
                if (NdotH < 0.0) // Prevent negative
                NdotH = 0.0;
                specular = glm::vec3(Ls * Ms * powf(NdotH, shininess));
            }
            else{ // Phong
                glm::vec3 viewDir = normalize(vEC);
                glm::vec3 reflectDir = reflect(-lEC, nEC);
                float spec = fmax(dot(viewDir, reflectDir), 0.0);
                specular = glm::vec3(Ls * Ms * powf(spec, shininess));
            }
            color += specular;
        }
    }

    return color;
}

void setupShader(){
    glUseProgram(shaderProgram);
    GLint loc = glGetUniformLocation(shaderProgram, "uColor");
    glUniform3f(loc, 1, 0, 0);

    GLint mvMat_loc = glGetUniformLocation(shaderProgram, "mvMat");
    glUniformMatrix4fv(mvMat_loc, 1, false, &modelViewMatrix[0][0]);

    GLint nMat_loc = glGetUniformLocation(shaderProgram, "nMat");
    glUniformMatrix3fv(nMat_loc, 1, false, &normalMatrix[0][0]);

    GLint t_loc = glGetUniformLocation(shaderProgram, "t");
    glUniform1f(t_loc, g.t);

    GLint shininess_loc = glGetUniformLocation(shaderProgram, "shininess");
    glUniform1f(shininess_loc, shininess);

    GLint waveDim = glGetUniformLocation(shaderProgram, "waveDim");
    glUniform1f(waveDim, float(g.waveDim));

    // Used so the shader can determine if fixed mode is active
    GLint fixed_toggle_loc = glGetUniformLocation(shaderProgram, "fixed_toggle");
    glUniform1f(fixed_toggle_loc, float(g.fixed));

    GLint pp_toggle_loc = glGetUniformLocation(shaderProgram, "pp_toggle");
    glUniform1f(pp_toggle_loc, float(g.lightingMode));

    GLint vbo_toggle_loc = glGetUniformLocation(shaderProgram, "vbo_toggle");
    glUniform1f(vbo_toggle_loc, float(g.vbo));

    GLint light_model_loc = glGetUniformLocation(shaderProgram, "light_model");
    glUniform1f(light_model_loc, float(g.lightingModel));

    GLint light_pos = glGetUniformLocation(shaderProgram, "light_pos");
    glUniform1f(light_pos, float(g.lightingPos));


}

void drawGrid(int tess)
{
    float stepSize = 2.0 / tess;
    glm::vec3 r, n, rEC, nEC;
    int i, j;

    if (g.lighting && g.fixed) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_NORMALIZE);
        glShadeModel(GL_SMOOTH);
        if (g.twoside)
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
        glMaterialfv(GL_FRONT, GL_SPECULAR, &white[0]);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess);
    } else {
        glDisable(GL_LIGHTING);
        glColor3fv(&cyan[0]);
    }

    if (g.polygonMode == line)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    for (j = 0; j < tess; j++) {
        glBegin(GL_QUAD_STRIP);
        for (i = 0; i <= tess; i++) {
            r.x = -1.0 + i * stepSize;
            r.y = 0.0;
            r.z = -1.0 + j * stepSize;

            if (g.lighting) {
                n.x = 0.0;
                n.y = 1.0;
                n.z = 0.0;
            }
            // IF shader is used use modelViewMatrix on GPU
            if(g.shader) rEC = glm::vec3(glm::vec4(r, 1.0));
            else rEC = glm::vec3(modelViewMatrix * glm::vec4(r, 1.0));
            if (g.lighting) {
                // Only use normals on GPU if fixed is enabled
                if(g.shader && g.fixed) nEC = glm::normalize(n);
                else nEC = normalMatrix * glm::normalize(n);
                if (g.fixed) {

                    glNormal3fv(&nEC[0]);
                } else {
                    glm::vec3 c = computeLighting(rEC, nEC);
                    glColor3fv(&c[0]);
                }
            }
            glVertex3fv(&rEC[0]);

            r.z += stepSize;

            // IF shader is used use modelViewMatrix on GPU
            if(g.shader) rEC = glm::vec3(glm::vec4(r, 1.0));
            else rEC = glm::vec3(modelViewMatrix * glm::vec4(r, 1.0));
            if (g.lighting) {
                // Only use normals on GPU if fixed is enabled
                if(g.shader && g.fixed) nEC = glm::normalize(n);
                else nEC = normalMatrix * glm::normalize(n);
                if (g.fixed) {
                    glNormal3fv(&nEC[0]);
                } else {
                    glm::vec3 c = computeLighting(rEC, nEC);
                    glColor3fv(&c[0]);
                }
            }
            glVertex3fv(&rEC[0]);
        }

        glEnd();

    }

    if (g.lighting) {
        glDisable(GL_LIGHTING);
    }

    // Normals
    if (g.drawNormals) {
        for (j = 0; j <= tess; j++) {
            for (i = 0; i <= tess; i++) {
                r.x = -1.0 + i * stepSize;
                r.y = 0.0;
                r.z = -1.0 + j * stepSize;

                n.y = 1.0;
                n.x = 0.0;
                n.z = 0.0;
                if(g.shader) rEC = glm::vec3(glm::vec4(r, 1.0));
                else rEC = glm::vec3(modelViewMatrix * glm::vec4(r, 1.0));

                if(g.shader) nEC = glm::normalize(n);
                else nEC = normalMatrix * glm::normalize(n);
                drawVector(rEC, nEC, 0.05, true, yellow);
            }
        }
    }

    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("%s %d\n", __FILE__, __LINE__);
        printf("displaySineWave: %s\n", gluErrorString(err));
    }
}

void drawSineWave(int tess)
{
    const float A1 = 0.25, k1 = 2.0 * M_PI, w1 = 0.25;
    const float A2 = 0.25, k2 = 2.0 * M_PI, w2 = 0.25;
    float stepSize = 2.0 / tess;
    glm::vec3 r, n, rEC, nEC;
    int i, j;
    float t = g.t;

    if (g.lighting && g.fixed) {
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_NORMALIZE);
        glShadeModel(GL_SMOOTH);
        if (g.twoside)
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
        glMaterialfv(GL_FRONT, GL_SPECULAR, &white[0]);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess);
    } else {
        glDisable(GL_LIGHTING);
        glColor3fv(&cyan[0]);
    }

    if (g.polygonMode == line)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Sine wave
    for (j = 0; j < tess; j++) {
        glBegin(GL_QUAD_STRIP);
        for (i = 0; i <= tess; i++) {
            r.x = -1.0 + i * stepSize;
            r.z = -1.0 + j * stepSize;

            if (g.waveDim == 2) {
                if(!g.shader || !g.fixed)
                r.y = A1 * sinf(k1 * r.x + w1 * t);
                if (g.lighting) {
                    if(!g.shader || !g.fixed){
                        n.x = - A1 * k1 * cosf(k1 * r.x + w1 * t);
                        n.y = 1.0;
                        n.z = 0.0;
                    }
                }
            } else if (g.waveDim == 3) {
                if(!g.shader || !g.fixed)
                r.y = A1 * sinf(k1 * r.x + w1 * t) + A2 * sinf(k2 * r.z + w2 * t);
                if (g.lighting) {
                    if(!g.shader || !g.fixed){
                        n.x = - A1 * k1 * cosf(k1 * r.x + w1 * t);
                        n.y = 1.0;
                        n.z = - A2 * k2 * cosf(k2 * r.z + w2 * t);
                    }
                }
            }
            // IF shader is used use modelViewMatrix on GPU
            if(g.shader) rEC = glm::vec3(glm::vec4(r, 1.0));
            else rEC = glm::vec3(modelViewMatrix * glm::vec4(r, 1.0));
            if (g.lighting) {
                // Only use normals on GPU if fixed is enabled
                if(g.shader && g.fixed) nEC = glm::normalize(n);
                else nEC = normalMatrix * glm::normalize(n);

                if (g.fixed) { // If Fixed use pipeline
                    glNormal3fv(&nEC[0]);
                } else { // Else calculate lighting
                    glm::vec3 c = computeLighting(rEC, nEC);
                    glColor3fv(&c[0]);
                }
            }
            glVertex3fv(&rEC[0]);

            r.z += stepSize;

            if (g.waveDim == 3) {
                r.y = A1 * sinf(k1 * r.x + w1 * t) + A2 * sinf(k2 * r.z + w2 * t);
                if (g.lighting) {
                    n.z = - A2 * k2 * cosf(k2 * r.z + w2 * t);
                }
            }
            // IF shader is used use modelViewMatrix on GPU
            if(g.shader) rEC = glm::vec3(glm::vec4(r, 1.0));
            else rEC = glm::vec3(modelViewMatrix * glm::vec4(r, 1.0));
            if (g.lighting) {
                // Only use normals on GPU if fixed is enabled
                if(g.shader && g.fixed) nEC = glm::normalize(n);
                else nEC = normalMatrix * glm::normalize(n);
                if (g.fixed) {
                    glNormal3fv(&nEC[0]);
                } else {
                    glm::vec3 c = computeLighting(rEC, nEC);
                    glColor3fv(&c[0]);
                }
            }
            glVertex3fv(&rEC[0]);
        }

        glEnd();

    }

    if (g.lighting) {
        glDisable(GL_LIGHTING);
    }

    // Normals
    if (g.drawNormals) {
        for (j = 0; j <= tess; j++) {
            for (i = 0; i <= tess; i++) {
                r.x = -1.0 + i * stepSize;
                r.z = -1.0 + j * stepSize;

                n.y = 1.0;
                n.x = - A1 * k1 * cosf(k1 * r.x + w1 * t);
                if (g.waveDim == 2) {
                    r.y = A1 * sinf(k1 * r.x + w1 * t);
                    n.z = 0.0;
                } else {
                    r.y = A1 * sinf(k1 * r.x + w1 * t) + A2 * sinf(k2 * r.z + w2 * t);
                    n.z = - A2 * k2 * cosf(k2 * r.z + w2 * t);
                }

                if(g.shader) rEC = glm::vec3(glm::vec4(r, 1.0));
                else rEC = glm::vec3(modelViewMatrix * glm::vec4(r, 1.0));

                if(g.shader) nEC = glm::normalize(n);
                else nEC = normalMatrix * glm::normalize(n);

                drawVector(rEC, nEC, 0.05, true, yellow);
            }
        }
    }

    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("%s %d\n", __FILE__, __LINE__);
        printf("displaySineWave: %s\n", gluErrorString(err));
    }
}

void showInfo(){
    printf("\n\nTesselation \t\t(%i)\nFrameRate \t\t(%.0f)\n", g.tess, g.frameRate);
    printf("Frametime \t\t(%.0f)\nPer Pixel \t\t(%i)\n", 1.0 / g.frameRate * 1000.0, g.lightingMode);
    printf("Fixed \t\t\t(%i)\nVBO \t\t\t(%i)\n",g.fixed, g.vbo);
    printf("Shaders \t\t(%i)\nWaveDim \t\t(%i)\n", g.shader, g.waveDim);
    printf("Shininess \t\t(%.0f)\n", shininess);
    printf("Model \t\t\t(%s)\n", g.lightingModel ? "Blinn-Phong" : "Phong");
    printf("Lightpos \t\t(%s)\n", g.lightingPos ? "Directional" : "Positional");
}

void idle()
{
    float t, dt;

    t = glutGet(GLUT_ELAPSED_TIME) / milli;

    // Accumulate time if animation enabled
    if (g.animate) {
        dt = t - g.lastT;
        g.t += dt;
        g.lastT = t;
        if (debug[d_animation])
        printf("idle: animate %f\n", g.t);
    }

    // Update stats, although could make conditional on a flag set interactively
    dt = (t - g.lastStatsDisplayT);
    if (dt > g.displayStatsInterval) {
        g.frameRate = g.frameCount / dt;
        if (debug[d_OSD])
        printf("dt %f framecount %d framerate %f\n", dt, g.frameCount, g.frameRate);
        g.lastStatsDisplayT = t;
        g.frameCount = 0;

        if(g.consolePM) showInfo();
    }

    glutPostRedisplay();
}

void displayMultiView()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glm::mat4 modelViewMatrixSave(modelViewMatrix);
    glm::mat3 normalMatrixSave(normalMatrix);


    // Front view
    modelViewMatrix = glm::mat4(1.0);
    glViewport(g.width / 16.0, g.height * 9.0 / 16.0, g.width * 6.0 / 16.0, g.height * 6.0 / 16.0);
    drawAxes(5.0);
    if (g.shape == grid)
    drawGrid(g.tess);
    else
    drawSineWave(g.tess);


    // Top view
    modelViewMatrix = glm::mat4(1.0);
    modelViewMatrix = glm::rotate(modelViewMatrix, glm::pi<float>() / 2.0f, glm::vec3(1.0, 0.0, 0.0));
    glViewport(g.width / 16.0, g.height / 16.0, g.width * 6.0 / 16.0, g.height * 6.0 / 16);
    drawAxes(5.0);
    if (g.shape == grid)
    drawGrid(g.tess);
    else
    drawSineWave(g.tess);

    // Left view
    modelViewMatrix = glm::mat4(1.0);
    modelViewMatrix = glm::rotate(modelViewMatrix, glm::pi<float>() / 2.0f, glm::vec3(0.0, 1.0, 0.0));
    glViewport(g.width * 9.0 / 16.0, g.height * 9.0 / 16.0, g.width * 6.0 / 16.0, g.height * 6.0 / 16.0);
    drawAxes(5.0);
    if (g.shape == grid)
    drawGrid(g.tess);
    else
    drawSineWave(g.tess);

    // General view
    modelViewMatrix = glm::rotate(modelViewMatrix, camera.rotateX * glm::pi<float>() / 180.0f, glm::vec3(1.0, 0.0, 0.0));
    modelViewMatrix = glm::rotate(modelViewMatrix, camera.rotateY * glm::pi<float>() / 180.0f, glm::vec3(0.0, 1.0, 0.0));
    modelViewMatrix = glm::scale(modelViewMatrix, glm::vec3(camera.scale));
    normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelViewMatrix)));
    glViewport(g.width * 9.0 / 16.0, g.width / 16.0, g.width * 6.0 / 16.0, g.height * 6.0 / 16.0);
    drawAxes(5.0);
    if (g.shape == grid)
    drawGrid(g.tess);
    else
    drawSineWave(g.tess);

    if (g.displayOSD)
    displayOSD();

    g.frameCount++;

    glutSwapBuffers();

}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);

    glViewport(0, 0, g.width, g.height);

    // General view

    modelViewMatrix = glm::mat4(1.0);
    normalMatrix = glm::mat3(1.0);

    modelViewMatrix = glm::rotate(modelViewMatrix, camera.rotateX * glm::pi<float>() / 180.0f, glm::vec3(1.0, 0.0, 0.0));
    modelViewMatrix = glm::rotate(modelViewMatrix, camera.rotateY * glm::pi<float>() / 180.0f, glm::vec3(0.0, 1.0, 0.0));
    modelViewMatrix = glm::scale(modelViewMatrix, glm::vec3(camera.scale));

    normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelViewMatrix)));



    if (debug[d_matrices]) {
        printf("modelViewMatrix\n");
        printMatrixColumnMajor(&modelViewMatrix[0][0], 4);
        printf("normalMatrix\n");
        printMatrixColumnMajor(&normalMatrix[0][0], 3);
    }

    drawAxes(5.0);

    // SHADERS
    if (g.shader == 1) setupShader();
    else glUseProgram(0);

    if (g.vbo == true){
        buildVBO(g.tess);
        useVBO();
    }
    else{
        if (g.shape == grid)
        drawGrid(g.tess);
        else
        drawSineWave(g.tess);
    }
    glUseProgram(0);
    // END SHADERS

    if (g.displayOSD)
    displayOSD();

    glutSwapBuffers();

    g.frameCount++;

    while ((err = glGetError()) != GL_NO_ERROR) {
        printf("%s %d\n", __FILE__, __LINE__);
        printf("display: %s\n", gluErrorString(err));
    }
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case 27:
        case 'q':
        printf("exit\n");
        exit(0);
        break;
        case 'a':
        g.animate = !g.animate;
        if (g.animate) {
            g.lastT = glutGet(GLUT_ELAPSED_TIME) / milli;
        }
        break;
        case 'l':
        g.lighting = !g.lighting;
        glutPostRedisplay();
        break;
        case 'f':
        g.fixed = !g.fixed;
        glutPostRedisplay();
        break;
        case 'w':
        printf("polygonMode: %d\n", g.polygonMode);
        if (g.polygonMode == line)
        g.polygonMode = fill;
        else
        g.polygonMode = line;
        glutPostRedisplay();
        break;
        case 'n':
        g.drawNormals = !g.drawNormals;
        glutPostRedisplay();
        break;
        case 'c':
        g.consolePM = !g.consolePM;
        glutPostRedisplay();
        break;
        case 'o':
        g.displayOSD = !g.displayOSD;
        glutPostRedisplay();
        break;
        case 's':
        g.shape = g.shape == grid ? wave : grid;
        g.animate = false;
        break;
        case '4':
        g.multiView = !g.multiView;
        if (g.multiView)
        glutDisplayFunc(displayMultiView);
        else
        glutDisplayFunc(display);
        glutPostRedisplay();
        break;
        case 'v':
        g.vbo = !g.vbo;
        printf("vbos partially implemented\n");
        break;
        case '+':
        g.tess *= 2;
        glutPostRedisplay();
        break;
        case '-':
        g.tess /= 2;
        if (g.tess < 8)
        g.tess = 8;
        glutPostRedisplay();
        break;
        case 'z':
        g.waveDim++;
        if (g.waveDim > 3)
        g.waveDim = 2;
        glutPostRedisplay();
        break;
        case 'g': // Toggle Shaders
        g.shader = !g.shader;
        break;
        case 'p': // Toggle Lighting mode
        g.lightingMode = !g.lightingMode;
        break;
        case 'h':
        shininess--;
        break;
        case 'H':
        shininess++;
        break;
        case 'm':
        g.lightingModel = !g.lightingModel;
        break;
        case 'd':
        g.lightingPos = !g.lightingPos;
        break;
        case '1':
        g.ambient = !g.ambient;
        printf("ambient on? %i\n", g.ambient);
        break;
        case '2':
        g.diffuse = !g.diffuse;
        printf("diffuse on? %i\n", g.diffuse);
        break;
        case '3':
        g.specular = !g.specular;
        printf("specular on? %i\n", g.specular);
        break;
        default:
        break;
    }
}

void mouse(int button, int state, int x, int y)
{
    if (debug[d_mouse])
    printf("mouse: %d %d %d\n", button, x, y);

    camera.lastX = x;
    camera.lastY = y;

    if (state == GLUT_DOWN)
    switch(button) {
        case GLUT_LEFT_BUTTON:
        camera.control = rotate;
        break;
        case GLUT_MIDDLE_BUTTON:
        camera.control = pan;
        break;
        case GLUT_RIGHT_BUTTON:
        camera.control = zoom;
        break;
    }
    else if (state == GLUT_UP)
    camera.control = inactive;
}

void motion(int x, int y)
{
    float dx, dy;

    if (debug[d_mouse]) {
        printf("motion: %d %d\n", x, y);
        printf("camera.rotate: %f %f\n", camera.rotateX, camera.rotateY);
        printf("camera.scale:%f\n", camera.scale);
    }

    dx = x - camera.lastX;
    dy = y - camera.lastY;
    camera.lastX = x;
    camera.lastY = y;

    switch (camera.control) {
        case inactive:
        break;
        case rotate:
        camera.rotateX += dy;
        camera.rotateY += dx;
        break;
        case pan:
        break;
        case zoom:
        camera.scale += dy / 100.0;
        break;
    }

    glutPostRedisplay();
}



int main(int argc, char** argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize (1024, 1024);
    glutInitWindowPosition (0, 100);
    glutCreateWindow (argv[0]);
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMainLoop();
    return 0;
}

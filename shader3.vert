// This is the vertex shader from the week 7 lab
uniform vec3 uColor;
varying vec3 vColor;

uniform mat4 mvMat;

void main(void){
    vec4 esVert = mvMat * gl_Vertex;
    vec4 csVert = gl_ProjectionMatrix * esVert;
    gl_Position = csVert;

    vColor = uColor;
}

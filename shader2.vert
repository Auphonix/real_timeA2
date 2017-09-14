/* !!GLSL */

varying vec4 eposition;
varying vec3 normal;
varying vec3 diffuseColor;
varying vec3 specularColor;
varying vec3 emissiveColor;
varying vec3 ambientColor;
varying float shininess;

void main()
{
    vec4 osVert = gl_Vertex;
    vec4 esVert = gl_ModelViewMatrix * osVert;
    vec4 csVert = gl_ProjectionMatrix * esVert;
    // Position in clip space
    gl_Position = csVert;

    // Position in eye space
    eposition = csVert;

    // Normal in eye space
    normal = mat3(gl_ModelViewMatrix[0].xyz, gl_ModelViewMatrix[1].xyz, gl_ModelViewMatrix[2].xyz) * gl_Normal;

    // Retrieves diffuse, specular emissive, and ambient color from the OpenGL state.
    diffuseColor = vec3(gl_FrontMaterial.diffuse);
    specularColor = vec3(gl_FrontMaterial.specular);
    emissiveColor = vec3(gl_FrontMaterial.emission);
    ambientColor = vec3(gl_FrontMaterial.ambient);
    shininess = gl_FrontMaterial.shininess;
}

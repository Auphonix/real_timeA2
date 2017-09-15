float shininess = 50.0;
float NdotL;

// ModelViewMatrix and Normal Matrix
uniform mat4 mvMat;
uniform mat3 nMat;

uniform float passthrough; // Fixed mode toggle
uniform float pp_toggle; // Per pixel mode

// Vector attributes
varying vec3 vNormal;
varying vec3 vLight;
varying vec3 vViewer;
varying vec4 vColor;

vec3 blinnPhong(){
    vec3 nEC = vNormal;
    // vec3 nEC = gl_Normal;
    // Used to accumulate ambient, diffuse and specular contributions
    // Note: it is a vec3 being constructed with a single value which
    // is used for all 3 components
    vec3 color = vec3(0);
    // Ambient contribution: A=LaÃ—Ma
    // Default light ambient color and default ambient material color
    // are both (0.2, 0.2, 0.2)
    vec3 La = vec3(0.2);
    vec3 Ma = vec3(0.2);
    vec3 ambient = vec3(La * Ma);
    color += ambient;

    // Light direction vector. Default for LIGHT0 is a directional light
    // along z axis for all vertices, i.e. <0, 0, 1>
    vec3 lEC = vLight;

    // Test if normal points towards light source, i.e. if polygon
    // faces toward the light - if not then no diffuse or specular
    // contribution
    float dp = dot(nEC, lEC);

    if (dp > 0.0) {
        // Calculate diffuse and specular contribution
        // Lambert diffuse: D=LdÃ—MdÃ—cosÎ¸
        // Ld: default diffuse light color for GL_LIGHT0 is white (1.0, 1.0, 1.0).
        // Md: default diffuse material color is grey (0.8, 0.8, 0.8).
        vec3 Ld = vec3(1.0);
        vec3 Md = vec3(0.0, 0.5, 0.5);
        // Need normalized normal to calculate cosÎ¸,
        // light vector <0, 0, 1> is already normalized
        nEC = normalize(nEC);
        NdotL = dot(nEC, lEC);
        vec3 diffuse = vec3(Ld * Md * NdotL);
        color += diffuse;
        // Blinn-Phong specular: S=LsÃ—MsÃ—cosâ¿Î±
        // Ls: default specular light color for LIGHT0 is white (1.0, 1.0, 1.0)
        // Ms: specular material color, also set to white (1.0, 1.0, 1.0),
        // but default for fixed pipeline is black, which means can't see
        // specular reflection. Need to set it to same value for fixed
        // pipeline lighting otherwise will look different.
        vec3 Ls = vec3(1.0);
        vec3 Ms = vec3(0.8);
        // Default viewer is at infinity along z axis <0, 0, 1> i.e. a
        // non local viewer (see glLightModel and GL_LIGHT_MODEL_LOCAL_VIEWER)
        vec3 vEC = vViewer;
        // Blinn-Phong half vector (using a single capital letter for
        // variable name!). Need normalized H (and nEC, above) to calculate cosÎ±.
        vec3 H = vec3(lEC + vEC);
        H = normalize(H);
        float NdotH = dot(nEC, H);
        if (NdotH < 0.0) // Prevent negative
        NdotH = 0.0;
        vec3 specular = vec3(Ls * Ms * pow(NdotH, shininess));
        color += specular;
    }
    return color;
}

// MAIN
void main(void)
{
    // Equivalent to: gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex
    // os - object space, es - eye space, cs - clip space
    vec4 osVert = gl_Vertex;
    vec4 esVert = mvMat * osVert;
    vec4 csVert = gl_ProjectionMatrix * esVert;
    gl_Position = csVert;

    if(passthrough == 0.0){
        vColor = gl_Color;
    }
    else{

        if(pp_toggle == 1.0){ // Per pixel
            // Interpolate values
            vNormal = normalize(nMat * gl_Normal);
            vLight = normalize(vec3(0, 0, 1));
            vViewer = normalize(vec3(0, 0, 1));
        }
        else{ // Per vertex
            // Calculate lighting using blinnPhong
            vNormal = nMat * normalize(gl_Normal);
            vLight = vec3(0, 0, 1);
            vViewer = vec3(0, 0, 1);
            vColor = vec4(blinnPhong(), 1);
        }
    }
}

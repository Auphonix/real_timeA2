// ModelViewMatrix and Normal Matrix
uniform mat4 mvMat;
uniform mat3 nMat;

uniform float fixed_toggle; // Fixed mode toggle
uniform float pp_toggle; // Per pixel mode
uniform float vbo_toggle; // Toggle VBOs

// T blinn-Phong, F = Phong
uniform float light_model; // Toggle model
// T directional, F = positional.
uniform float light_pos; // Toggle light pos


// Vector attributes
varying vec3 vNormal;
varying vec3 vLight;
varying vec3 vViewer;
varying vec4 vColor;

// Vertex + Normal calculations
vec3 n;
vec4 vert = gl_Vertex;
varying vec3 vLight_pos;

vec3 surfaceToLight;
float attenuation = 1.0;

varying vec4 esVert;

// Values to calculate sine wave
float M_PI = acos(-1.0), A1 = 0.25, k1 = 2.0 * M_PI, w1 = 0.25;
float A2 = 0.25, k2 = 2.0 * M_PI, w2 = 0.25;
uniform float t; // Get time from main application
uniform float waveDim;

uniform float shininess;

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
    // vLight_pos = (mvMat * gl_Vertex).xyz;
    // vec3 tmp = vLight + vLight_pos;
    // vec3 lEC = normalize(tmp);
    vec3 lEC;
    if(light_model == 1.0) lEC = vLight;
    else lEC = normalize(vLight);


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
        float NdotL = dot(nEC, lEC);
        vec3 diffuse = vec3(Ld * Md * NdotL);
        //color += diffuse;
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
        vec3 specular;
        if(light_model == 1.0){ // Blinn Phong
            vec3 H = vec3(lEC + vEC);
            H = normalize(H);
            float NdotH = dot(nEC, H);
            if (NdotH < 0.0) // Prevent negative
            NdotH = 0.0;
            specular = vec3(Ls * Ms * pow(NdotH, shininess));
        }
        else{ // Phong
            vec3 viewDir = normalize(vEC);
            vec3 reflectDir = reflect(-lEC, nEC);
            float spec = max(dot(viewDir, reflectDir), 0.0);
            specular = vec3(Ls * Ms * pow(spec, shininess));
        }
        color += attenuation * (diffuse + specular);
    }
    return color;
}

void calcVertex(){
    // Only calculate normals and y pos for fixed mode
    if(fixed_toggle == 1.0){
        if (waveDim == 2.0){
            vert.y = A1 * sin(k1 * vert.x + w1 * t);
            n.x = - A1 * k1 * cos(k1 * vert.x + w1 * t);
            n.y = 1.0;
            n.z = 0.0;
        }
        if (waveDim == 3.0){
            vert.y =  A1 * sin(k1 * vert.x + w1 * t) + A2 * sin(k2 * vert.z + w2 * t);
            n.x = - A1 * k1 * cos(k1 * vert.x + w1 * t);
            n.y = 1.0;
            n.z = - A2 * k2 * cos(k2 * vert.z + w2 * t);
        }
    }
}

// MAIN
void main(void)
{
    // Calculate y pos and normals on GPU
    calcVertex();

    // Equivalent to: gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex
    // os - object space, es - eye space, cs - clip space
    vec4 osVert = vert;
    esVert = mvMat * osVert;
    vec4 csVert = gl_ProjectionMatrix * esVert;
    gl_Position = csVert;

    if(fixed_toggle == 0.0){ // not fixed, pass color from main app
        vColor = gl_Color;
    }
    else{
        if(pp_toggle == 1.0){ // Per pixel
            // Interpolate values
            vNormal = normalize(nMat * n);
            vLight = vec3(0.5, 0.5, 0.5);
            vViewer = normalize(vec3(0, 0, 1));
        }
        else{ // Per vertex
            // Calculate lighting using blinnPhong
            vNormal = nMat * normalize(n);
            vLight = vec3(0.5, 0.5, 0.5);
            vViewer = vec3(0, 0, 1);
            vColor = vec4(blinnPhong(), 1);
        }
    }
}

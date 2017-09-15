varying vec4 n_color;

varying vec3 pp_normal; // Per pixel Normal
varying vec3 pp_light; // Per pixel Light_pos
uniform float passthrough; // Fixed mode toggle

uniform float pp_toggle;

void main (void)
{
    if(passthrough == 1.0 && pp_toggle == 1.0){
        float diffuse_value = max(dot(pp_normal, pp_light), 0.0);
        gl_FragColor = n_color * diffuse_value;
    }
    else {
        gl_FragColor = n_color;
    }
}

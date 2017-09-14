// This is the fragment shader from the week 7 lab

varying vec3 vColor;

void main(void){
    gl_FragColor = vec4(vColor, 1);
}

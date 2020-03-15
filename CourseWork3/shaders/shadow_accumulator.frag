#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sourceTexture1;
uniform sampler2D sourceTexture2;

void main()
{
    vec3 color1 = texture(sourceTexture1, TexCoords).rgb;
    vec3 color2 = texture(sourceTexture2, TexCoords).rgb;
    //vec3 color = mix(color1, color2, 0.5);
    FragColor = vec4(color1 + color2, 1.0);
} 
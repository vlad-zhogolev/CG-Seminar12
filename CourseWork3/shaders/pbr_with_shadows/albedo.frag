#version 330 core

// output color
out vec4 FragColor;

// input data
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// material maps
uniform sampler2D texture_albedo1;

void main()
{		
    vec3 albedo = pow(texture(texture_albedo1, TexCoords).rgb, vec3(2.2));

    vec3 color = vec3(0.003) * albedo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}
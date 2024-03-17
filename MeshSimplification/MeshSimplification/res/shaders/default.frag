#version 330 core
out vec4 FragColor;

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;

uniform sampler2D texture1;
uniform sampler2D texture2;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    // Ambient
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  

    vec3 lightingResult = (ambient + diffuse + specular) * objectColor;
    vec4 textureResult = mix(texture(texture1, texCoord), texture(texture2, texCoord), 0.2);
    // Returns 80% of the first texture, 20% (0.2) of the second
    FragColor = textureResult * vec4(lightingResult, 1.0);
} 
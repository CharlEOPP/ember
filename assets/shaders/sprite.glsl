#type vertex
#version 410 core
layout(location = 0) in vec3  a_Position;
layout(location = 1) in vec4  a_Color;
layout(location = 2) in vec2  a_UV;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in float a_EntityID;

uniform mat4 u_ViewProjection;

out vec4      v_Color;
out vec2      v_UV;
flat out int  v_TexIndex;

void main() {
    v_Color    = a_Color;
    v_UV       = a_UV;
    v_TexIndex = int(a_TexIndex);
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 410 core
in vec4      v_Color;
in vec2      v_UV;
flat in int  v_TexIndex;

uniform sampler2D u_Textures[16];

out vec4 fragColor;

void main() {
    // Static switch indexing keeps this valid on GL 4.1 (no dynamic sampler indexing).
    vec4 sampled = vec4(1.0);
    switch (v_TexIndex) {
        case  0: sampled = texture(u_Textures[ 0], v_UV); break;
        case  1: sampled = texture(u_Textures[ 1], v_UV); break;
        case  2: sampled = texture(u_Textures[ 2], v_UV); break;
        case  3: sampled = texture(u_Textures[ 3], v_UV); break;
        case  4: sampled = texture(u_Textures[ 4], v_UV); break;
        case  5: sampled = texture(u_Textures[ 5], v_UV); break;
        case  6: sampled = texture(u_Textures[ 6], v_UV); break;
        case  7: sampled = texture(u_Textures[ 7], v_UV); break;
        case  8: sampled = texture(u_Textures[ 8], v_UV); break;
        case  9: sampled = texture(u_Textures[ 9], v_UV); break;
        case 10: sampled = texture(u_Textures[10], v_UV); break;
        case 11: sampled = texture(u_Textures[11], v_UV); break;
        case 12: sampled = texture(u_Textures[12], v_UV); break;
        case 13: sampled = texture(u_Textures[13], v_UV); break;
        case 14: sampled = texture(u_Textures[14], v_UV); break;
        case 15: sampled = texture(u_Textures[15], v_UV); break;
    }
    fragColor = sampled * v_Color;
}

#include "Common_Lit.hlsli"

struct VertexShaderInput
{
    float4 m_pos : POSITION;
    float4 m_normal : NORMAL;
    float2 m_uv0 : TEXCOORD0;
    float2 m_uv1 : TEXCOORD1;
};
 
PixelShaderInput main( VertexShaderInput vsInput )
{
    return GeneratePixelShaderInput(vsInput.m_pos.xyz, vsInput.m_normal.xyz, vsInput.m_uv0);
}
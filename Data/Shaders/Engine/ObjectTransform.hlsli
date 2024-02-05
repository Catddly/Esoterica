#pragma once

[[vk::binding(0, 1)]]
cbuffer Transforms
{
    matrix m_worldTransform; // TODO: move to per instance data and apply camera centric world transform in veretx shader(campos=(0, 0, 0)), currently done on CPU
    matrix m_normalTransform;
    matrix m_viewprojTransform;
};

struct PixelShaderInput
{
    float4 m_pos : SV_POSITION;
    // float3 m_wpos : POSITION;
    // float3 m_normal : NORMAL;
    // float2 m_uv : TEXCOORD;
};

PixelShaderInput GeneratePixelShaderInput(float3 objectPos, float3 objectNormal, float2 uv)
{
    PixelShaderInput output;
    float3 const objectPosWs = mul( m_worldTransform, float4(objectPos, 1.0) ).xyz;
    output.m_pos = mul( m_viewprojTransform, float4(objectPosWs, 1.0) );
    // output.m_wpos = mul( m_worldTransform, float4(objectPos, 1.0) ).xyz;
    // output.m_normal = mul( m_normalTransform, float4(objectNormal, 0.0) ).xyz;
    // output.m_uv = uv;
    return output;
}

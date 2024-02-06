[[vk::binding(0)]]
cbuffer ubo
{
    float4x4 ProjectionMatrix;
    // float2 Scale;
    // float2 Translate;
};

// The member order of this struct must be the same as ImDrawVert
struct VS_INPUT
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

PS_INPUT main( VS_INPUT input )
{
    PS_INPUT output;
    output.pos = mul( ProjectionMatrix, float4( input.pos.xy, 0.f, 1.f ) );
    // output.pos = float4( input.pos.xy * Scale + Translate, 0.f, 1.f );
    output.col = input.col;
    output.uv = input.uv;
    return output;
}
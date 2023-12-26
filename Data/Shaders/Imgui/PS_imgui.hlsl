struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

[[vk::binding(1)]] sampler sampler_llr;
[[vk::binding(2)]] Texture2D fontTexture;

float4 main( PS_INPUT input ) : SV_TARGET
{
    float4 out_col = input.col * fontTexture.Sample(sampler_llr, input.uv);
    return out_col;
}
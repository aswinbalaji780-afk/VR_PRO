// vr_shader.hlsl
// DirectX 11 Pixel Shader for VR Stereoscopic Split and Lens Distortion

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

// Barrel distortion parameters (adjust these based on your specific VR lenses)
static const float k1 = 0.22f; // Distortion coefficient 1
static const float k2 = 0.24f; // Distortion coefficient 2
static const float IPD_SHIFT = 0.05f; // Inter-pupillary distance shift

// Helper function to apply barrel distortion
float2 ApplyDistortion(float2 uv, float2 center)
{
    // Shift UV to be relative to the center of the lens
    float2 r = uv - center;
    float r2 = r.x * r.x + r.y * r.y; // r squared
    
    // Apply barrel distortion formula: r_distorted = r * (1 + k1*r^2 + k2*r^4)
    float distortion = 1.0f + (k1 * r2) + (k2 * r2 * r2);
    
    // Return to 0-1 UV space
    return center + (r * distortion);
}

float4 VR_StereoPixelShader(PixelInputType input) : SV_TARGET
{
    float2 texCoord = input.tex;
    float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Determine if we are rendering the left or right half of the screen
    bool isLeftEye = texCoord.x < 0.5f;
    
    float2 distortedUV;
    float2 center;
    
    if (isLeftEye)
    {
        // Left Eye Processing
        
        // Map the left half of the screen (0.0 to 0.5) to a full 0.0 to 1.0 UV space for sampling
        float2 localUV = float2(texCoord.x * 2.0f, texCoord.y);
        
        // Define the center of the left lens (typically slightly offset to the right due to nose bridge)
        center = float2(0.5f + IPD_SHIFT, 0.5f);
        
        distortedUV = ApplyDistortion(localUV, center);
        
        // Check if the distorted UV falls outside the rendering bounds
        if (distortedUV.x < 0.0f || distortedUV.x > 1.0f || distortedUV.y < 0.0f || distortedUV.y > 1.0f)
        {
            return float4(0.0f, 0.0f, 0.0f, 1.0f); // Render black vignette
        }
        
        // Sample the original texture. 
        // We do not divide by 2 here because we assume the input texture is a standard 16:9 game frame,
        // and we want to render the entire game frame to the left eye (just slightly shifted).
        float2 sampleUV = float2(distortedUV.x, distortedUV.y);
        color = shaderTexture.Sample(SampleType, sampleUV);
    }
    else
    {
        // Right Eye Processing
        
        // Map the right half of the screen (0.5 to 1.0) to a full 0.0 to 1.0 UV space for sampling
        float2 localUV = float2((texCoord.x - 0.5f) * 2.0f, texCoord.y);
        
        // Define the center of the right lens (slightly offset to the left)
        center = float2(0.5f - IPD_SHIFT, 0.5f);
        
        distortedUV = ApplyDistortion(localUV, center);
        
        if (distortedUV.x < 0.0f || distortedUV.x > 1.0f || distortedUV.y < 0.0f || distortedUV.y > 1.0f)
        {
            return float4(0.0f, 0.0f, 0.0f, 1.0f); // Render black vignette
        }
        
        // Sample the original game frame
        float2 sampleUV = float2(distortedUV.x, distortedUV.y);
        color = shaderTexture.Sample(SampleType, sampleUV);
    }

    return color;
}

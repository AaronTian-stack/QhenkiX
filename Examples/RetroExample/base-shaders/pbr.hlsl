#ifndef PBR_HLSL
#define PBR_HLSL

#define PI 3.14159265359

// Adapted from https://seblagarde.wordpress.com/wp-content/uploads/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf

float3 F_schlick(float3 f0, float f90, float u)
{
    return f0 + (f90 - f0) * pow(1.0 - u, 5.0);
}

float3 F_fresnel(float3 specular_color, float VoH)
{
    float3 specular_color_sqrt = sqrt(min(specular_color, float3(0.99, 0.99, 0.99)));
    float3 n = (1.0 + specular_color_sqrt) / (1.0 - specular_color_sqrt);
    float3 g = sqrt(n * n + VoH * VoH - 1.0);
    return 0.5 * sqrt((g - VoH) / (g + VoH)) * (1.0 + sqrt(((g + VoH) * VoH - 1.0) / ((g - VoH) * VoH + 1.0)));
}

float Fr_disney_diffuse(float NdotV, float NdotL, float LdotH, float linear_roughness)
{
    float energy_bias = lerp(0.0, 0.5, linear_roughness);
    float energy_factor = lerp(1.0, 1.0 / 1.51, linear_roughness);
    float fd90 = energy_bias + 2.0 * LdotH * LdotH * linear_roughness;
    float3 f0 = float3(1.0, 1.0, 1.0);
    float light_scatter = F_schlick(f0, fd90, NdotL).r;
    float view_scatter = F_schlick(f0, fd90, NdotV).r;

    return light_scatter * view_scatter * energy_factor;
}

float V_smith_GGX_correlated(float NdotL, float NdotV, float alpha_g)
{
    float alpha_g2 = alpha_g * alpha_g;
    alpha_g2 = alpha_g2 + 0.0000001;
    float lambda_ggxv = NdotL * sqrt((-NdotV * alpha_g2 + NdotV) * NdotV + alpha_g2);
    float lambda_ggxl = NdotV * sqrt((-NdotL * alpha_g2 + NdotL) * NdotL + alpha_g2);

    return 0.5f / (lambda_ggxv + lambda_ggxl);
}

float D_GGX(float NdotH, float m)
{
    // Divide by PI is applied later 
    float m2 = m * m;
    float f = (NdotH * m2 - NdotH) * NdotH + 1.0;
    return m2 / (f * f);
}

float3 get_F(float LdotH, float3 f0)
{
    float f90 = clamp(50.0 * dot(f0, float3(0.33, 0.33, 0.33)), 0.0, 1.0);
    return F_schlick(f0, f90, LdotH);
}

float3 get_specular(float NdotV, float NdotL, float LdotH, float NdotH, float roughness, float3 f0, out float3 F)
{
    F = get_F(LdotH, f0);
    float Vis = V_smith_GGX_correlated(NdotV, NdotL, roughness);
    float D = D_GGX(NdotH, roughness);
    float3 Fr = D * F * Vis / PI;
    return Fr;
}

float get_diffuse(float NdotV, float NdotL, float LdotH, float linear_roughness)
{
    float Fd = Fr_disney_diffuse(NdotV, NdotL, LdotH, linear_roughness) / PI;
    return Fd;
}

struct Material
{
    float3 albedo;
    float metallic;
    float roughness;
    float ao;
    float3 f0;
};

#define BRDF_MAKE( N, L, V )								\
	const float3	H = normalize(L + V);			  	    \
	const float		VdotN = abs(dot(N, V)) + 1e-5;			\
	const float		LdotN = max(0.0, dot(L, N));  			\
	const float		HdotV = max(0.0, dot(H, V));			\
	const float		HdotN = max(0.0, dot(H, N)); 			\
	const float		NdotV = VdotN;					  		\
	const float		NdotL = LdotN;					  		\
	const float		VdotH = HdotV;					  		\
	const float		NdotH = HdotN;					  		\
	const float		LdotH = HdotV;					  		\
	const float		HdotL = LdotH;

#define BRDF_SPECULAR( ROUGHNESS, F0, F )					\
	get_specular(NdotV, NdotL, LdotH, NdotH, ROUGHNESS, F0, F)

float3 BRDF_CALCULATE(Material material, float illuminance, float3 light_color, float3 N, float3 L, float3 V)
{
    float roughness = material.roughness;
    float3 f0 = material.f0;
    BRDF_MAKE(N, L, V);
    // illuminace accounts for NdotL term
    float3 F;
    float3 specular = BRDF_SPECULAR(roughness, f0, F);

    float3 kD = float3(1.0, 1.0, 1.0) - F;
    kD *= 1.0 - material.metallic;

    float3 diffuse = kD * material.albedo / PI;

    diffuse = max(float3(0.0, 0.0, 0.0), diffuse);
    specular = max(float3(0.0, 0.0, 0.0), specular);

    return (diffuse + specular) * light_color * illuminance;
}

#endif // PBR_HLSL

// Compucolor Pictures, Authored by Graham Dianaty 2019
#include "BaseVSShader.h"
#include "SDK_screenspaceeffect_vs20.inc"
#include "post_tonemapping_ps30.inc"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar mat_post_tonemapping_exposure("mat_post_tonemapping_exposure", "0.96", FCVAR_CHEAT);
ConVar mat_post_tonemapping_whitepoint("mat_post_tonemapping_whitepoint", "5.0", FCVAR_CHEAT);

BEGIN_VS_SHADER_FLAGS(POST_TONEMAPPING, "GOOD TONEMAPPER GRAPHICS", SHADER_NOT_EDITABLE)
	BEGIN_SHADER_PARAMS
		SHADER_PARAM(BASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "_rt_FullFrameFB", "")
	END_SHADER_PARAMS

SHADER_FALLBACK
{
	return 0;
}

SHADER_INIT
{
	if (params[BASETEXTURE]->IsDefined())
	{
		LoadTexture(BASETEXTURE);
	}
}

SHADER_DRAW
{
	SHADOW_STATE
	{
		pShaderShadow->EnableDepthWrites(false);
		pShaderShadow->EnableAlphaWrites(true);
		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION, 1, 0, 0);
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, true);
		pShaderShadow->EnableSRGBWrite(true);
		DECLARE_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
		SET_STATIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

		DECLARE_STATIC_PIXEL_SHADER(post_tonemapping_ps30);
		SET_STATIC_PIXEL_SHADER(post_tonemapping_ps30);
	}

	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, BASETEXTURE, -1);

		DECLARE_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);
		SET_DYNAMIC_VERTEX_SHADER(sdk_screenspaceeffect_vs20);

		float exposure[2] = {mat_post_tonemapping_exposure.GetFloat(), mat_post_tonemapping_whitepoint.GetFloat()};
		pShaderAPI->SetPixelShaderConstant(0, exposure, 1);

		DECLARE_DYNAMIC_PIXEL_SHADER(post_tonemapping_ps30);
		SET_DYNAMIC_PIXEL_SHADER(post_tonemapping_ps30);
	}
	
	Draw();
}
END_SHADER

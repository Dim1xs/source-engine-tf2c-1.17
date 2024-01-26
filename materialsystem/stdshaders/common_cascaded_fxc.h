#include "common_ps_fxc.h"

float DoShadowNvidiaPCF5x5Gaussian( sampler DepthSampler, const float3 shadowMapPos, const float2 vShadowTweaks )
{
	float fEpsilonX    = vShadowTweaks.x;
	float fTwoEpsilonX = 2.0f * fEpsilonX;
	float fEpsilonY    = vShadowTweaks.y;
	float fTwoEpsilonY = 2.0f * fEpsilonY;

	// I guess we don't need this one.
	// float ooW = 1.0f / shadowMapPos.w;							// 1 / w
	float3 shadowMapCenter_objDepth = shadowMapPos;					// Do both projections at once


	float2 shadowMapCenter = shadowMapCenter_objDepth.xy;			// Center of shadow filter
	float objDepth = shadowMapCenter_objDepth.z;					// Object depth in shadow space

	float4 vOneTaps;
	vOneTaps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTwoEpsilonX,  fTwoEpsilonY ), objDepth, 1 ) ).x;
	vOneTaps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTwoEpsilonX,  fTwoEpsilonY ), objDepth, 1 ) ).x;
	vOneTaps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTwoEpsilonX, -fTwoEpsilonY ), objDepth, 1 ) ).x;
	vOneTaps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTwoEpsilonX, -fTwoEpsilonY ), objDepth, 1 ) ).x;
	float flOneTaps = dot( vOneTaps, float4(1.0f / 331.0f, 1.0f / 331.0f, 1.0f / 331.0f, 1.0f / 331.0f));

	float4 vSevenTaps;
	vSevenTaps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTwoEpsilonX,  0 ), objDepth, 1 ) ).x;
	vSevenTaps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTwoEpsilonX,  0 ), objDepth, 1 ) ).x;
	vSevenTaps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  0, fTwoEpsilonY ), objDepth, 1 ) ).x;
	vSevenTaps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  0, -fTwoEpsilonY ), objDepth, 1 ) ).x;
	float flSevenTaps = dot( vSevenTaps, float4( 7.0f / 331.0f, 7.0f / 331.0f, 7.0f / 331.0f, 7.0f / 331.0f ) );

	float4 vFourTapsA, vFourTapsB;
	vFourTapsA.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTwoEpsilonX,  fEpsilonY    ), objDepth, 1 ) ).x;
	vFourTapsA.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fEpsilonX,     fTwoEpsilonY ), objDepth, 1 ) ).x;
	vFourTapsA.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fEpsilonX,     fTwoEpsilonY ), objDepth, 1 ) ).x;
	vFourTapsA.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTwoEpsilonX,  fEpsilonY    ), objDepth, 1 ) ).x;
	vFourTapsB.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fTwoEpsilonX, -fEpsilonY    ), objDepth, 1 ) ).x;
	vFourTapsB.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fEpsilonX,    -fTwoEpsilonY ), objDepth, 1 ) ).x;
	vFourTapsB.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fEpsilonX,    -fTwoEpsilonY ), objDepth, 1 ) ).x;
	vFourTapsB.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fTwoEpsilonX, -fEpsilonY    ), objDepth, 1 ) ).x;
	float flFourTapsA = dot( vFourTapsA, float4( 4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f ) );
	float flFourTapsB = dot( vFourTapsB, float4( 4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f ) );

	float4 v20Taps;
	v20Taps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fEpsilonX,  fEpsilonY ), objDepth, 1 ) ).x;
	v20Taps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fEpsilonX,  fEpsilonY ), objDepth, 1 ) ).x;
	v20Taps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fEpsilonX, -fEpsilonY ), objDepth, 1 ) ).x;
	v20Taps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fEpsilonX, -fEpsilonY ), objDepth, 1 ) ).x;
	float fl20Taps = dot( v20Taps, float4(20.0f / 331.0f, 20.0f / 331.0f, 20.0f / 331.0f, 20.0f / 331.0f));

	float4 v33Taps;
	v33Taps.x = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  fEpsilonX,  0 ), objDepth, 1 ) ).x;
	v33Taps.y = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2( -fEpsilonX,  0 ), objDepth, 1 ) ).x;
	v33Taps.z = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  0, fEpsilonY ), objDepth, 1 ) ).x;
	v33Taps.w = tex2Dproj( DepthSampler, float4( shadowMapCenter + float2(  0, -fEpsilonY ), objDepth, 1 ) ).x;
	float fl33Taps = dot( v33Taps, float4(33.0f / 331.0f, 33.0f / 331.0f, 33.0f / 331.0f, 33.0f / 331.0f));

	float flCenterTap = tex2Dproj( DepthSampler, float4( shadowMapCenter, objDepth, 1 ) ).x * (55.0f / 331.0f);

	// Sum all 25 Taps
	return flOneTaps + flSevenTaps + +flFourTapsA + flFourTapsB + fl20Taps + fl33Taps + flCenterTap;
}

float DoCascadedShadow( sampler depthSampler, sampler randomSampler, float3 worldNormal, float3 lightDirection,
	float3 closePosition, float3 worldPosition, int nShadowLevel, float3 cascadedStepData,
	float2 vScreenPos, float4 vShadowTweaks, const bool bCheckDot = true )
{
	float cascadedDot = bCheckDot ? dot( -lightDirection, worldNormal ) : abs( dot( -lightDirection, worldNormal ) );

	// 0.0 samples far cascade, 1.0 samples close cascade.
	float blendCascades = step(closePosition.x, 0.49) * step(0.01, closePosition.x) * step(closePosition.y, 0.99) * step(0.01, closePosition.y);

	// Select the cascade.
	closePosition.xy = lerp(closePosition.xy * cascadedStepData.x + cascadedStepData.yz, closePosition.xy, blendCascades);

	float shadow = DoShadowNvidiaPCF5x5Gaussian( depthSampler, closePosition, float2( 1.0 / 2048.0, 1.0 / 1024.0 ) );

	float weight = lerp( saturate( saturate( abs( closePosition.x * 4.0 - 3.0 ) - 0.9 ) * 10.0 +
		saturate( abs( closePosition.y * 2.0 - 1.0 ) - 0.9 ) * 10.0 ), 0.0f, blendCascades);
	shadow = lerp( shadow, 1.0, weight ); // * saturate( cascadedDot * 12.0 );

	return lerp(1.0, shadow, smoothstep(-0.1, 0.1, cascadedDot)); //lerp(0.0, shadow, step(0.0, cascadedDot));
}
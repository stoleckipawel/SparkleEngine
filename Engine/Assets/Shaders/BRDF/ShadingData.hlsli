#pragma once







namespace BRDF
{






	struct ShadingData
	{
		float3 N;
		float3 V;
		float3 L;
		float3 H;

		float NoL;
		float NoV;
		float NoH;
		float VoH;
		float LoH;
	};



	ShadingData ComputeShadingData(float3 N, float3 V, float3 L)
	{
		ShadingData sd;
		sd.N = normalize(N);
		sd.V = normalize(V);
		sd.L = normalize(L);
		sd.H = normalize(sd.V + sd.L);

		sd.NoL = saturate(dot(sd.N, sd.L));
		sd.NoV = saturate(dot(sd.N, sd.V));
		sd.NoH = saturate(dot(sd.N, sd.H));
		sd.VoH = saturate(dot(sd.V, sd.H));
		sd.LoH = saturate(dot(sd.L, sd.H));

		return sd;
	}

}

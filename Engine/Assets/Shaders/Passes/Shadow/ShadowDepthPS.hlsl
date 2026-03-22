





float main(float4 Position : SV_POSITION, float Depth : TEXCOORD0) : SV_Target0
{
	(void) Position;
	return saturate(Depth);
}
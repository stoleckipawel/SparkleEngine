[shader("intersection")] void HelloIntersection()
{
	BuiltInTriangleIntersectionAttributes Attributes;
	Attributes.barycentrics = float2(0.0f, 0.0f);
	ReportHit(1.0f, 0, Attributes);
}

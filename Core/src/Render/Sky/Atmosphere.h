#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

#include "Math/Vector3.h"

struct Atmosphere
{
	float BottomRadius = 6360.0f;
	float TopRadius = 6460.0f;
	Vector3 Center = Vector3(0.0f, -6360.0f, 0.0f);

	Vector3 RayleighScattering = Vector3(0.06f, 0.14f, 0.33f);
	float RayleighDensityExpScale = -1.0f / 8.0f;   // -1/H_R, H_R = 8 km

	Vector3 MieScattering = Vector3(0.4f, 0.4f, 0.4f);
	float MieDensityExpScale = -1.0f / 1.2f;   // -1/H_M, H_M = 1.2 km
	Vector3 MieExtinction = Vector3(0.44f, 0.44f, 0.44f);
	float MiePhaseParam = 0.8f; // Cornette-Shanks

	float LayerHeight = 25.0f;
	float LinearTerm = 1.0f / 15.0f;
	float ConstantTerm = -2.0f / 3.0f;

	float LinearTerm1 = -1.0f / 15.0f;
	float ConstantTerm1 = 8.0f / 3.0f;

	Vector3 AbsorptionExtinction = Vector3(0.00065f, 0.001881f, 0.000085f);
	Vector3 GroundAlbedo = Vector3(0.1f, 0.1f, 0.1f);
	float MultipleScatteringFactor = 1.0f;
};

#endif
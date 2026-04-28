#ifndef LIGHT_RENDER_SYSTEM_H
#define LIGHT_RENDER_SYSTEM_H

#include "ECS/Scene.h"
#include "ECS/Components.h"
#include "Render/SceneRenderer.h"
#include "Render/SubmittedLight.h"
#include "Math/Matrix4x4.h"

namespace LightRenderSystem
{
	// Build a GPU-friendly array of SubmittedLightData from the scene's
	// LightComponents, and submit it to the renderer.
	//
	// Call once per frame between SceneRenderer::BeginFrame() and
	// SceneRenderer::RenderView().
	inline void CollectAndSubmit(Scene& scene, SceneRenderer& renderer)
	{
		auto view = scene.View<TransformComponent, LightComponent>();

		view.each([&](const TransformComponent& transform, const LightComponent& light)
		{
			SubmittedLight data;
			data.Type = light.Type;
			data.Color = light.Color * light.Intensity;
			data.Position = transform.Position;
			data.Range = light.Range;
			data.CastsShadows = light.CastsShadows;
			data.InnerConeCos = std::cosf(light.InnerConeAngle * (3.14159265f / 180.0f));
			data.OuterConeCos = std::cosf(light.OuterConeAngle * (3.14159265f / 180.0f));

			Matrix4x4 rot = Matrix4x4::RotateEuler(transform.Rotation.x, transform.Rotation.y, transform.Rotation.z);
			data.Direction = Vector3(-rot[2].x, -rot[2].y, -rot[2].z); // forward vector
			if (light.Type != LightType::Directional)
			{
				data.BoundsMin = transform.Position - Vector3(light.Range);
				data.BoundsMax = transform.Position + Vector3(light.Range);
			}

			renderer.SubmitLight(data);
			});
	}
}

#endif // LIGHT_RENDER_SYSTEM_H
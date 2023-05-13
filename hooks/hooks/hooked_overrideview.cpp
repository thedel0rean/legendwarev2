// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "..\hooks.hpp"
#include "..\..\cheats\visuals\GrenadePrediction.h"
#include "..\..\cheats\misc\fakelag.h"
#include "..\..\cheats\lagcompensation\lagcompensation.h"
#include "..\..\cheats\lagcompensation\local_animations.h"

using OverrideView_t = void(__stdcall*)(CViewSetup*);

void thirdperson(bool fakeducking);

void __stdcall hooks::hooked_overrideview(CViewSetup* viewsetup)
{
	static auto original_fn = clientmode_hook->get_func_address <OverrideView_t> (18);

	if (!viewsetup)
		return original_fn(viewsetup);

	if (g_ctx.local())
	{
		static auto fakeducking = false;

		if (!fakeducking && key_binds::get().get_key_bind_state(20) && !fakelag::get().condition)
			fakeducking = true;
		else if (fakeducking && (!key_binds::get().get_key_bind_state(20) || fakelag::get().condition) && (!g_ctx.local()->get_animation_state()->m_fDuckAmount || g_ctx.local()->get_animation_state()->m_fDuckAmount == 1.0f)) //-V550
			fakeducking = false;

		if (!g_ctx.local()->is_alive()) //-V807
			fakeducking = false;

		auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

		if (weapon)
		{
			if (!g_ctx.local()->m_bIsScoped() && g_cfg.player.enable)
				viewsetup->fov += g_cfg.esp.fov;
			else if (g_cfg.esp.removals[REMOVALS_ZOOM] && g_cfg.player.enable)
			{
				if (weapon->m_zoomLevel() == 1)
					viewsetup->fov = 90.0f + (float)g_cfg.esp.fov;
				else
					viewsetup->fov += (float)g_cfg.esp.fov;
			}
		}
		else if (g_cfg.player.enable)
			viewsetup->fov += g_cfg.esp.fov;

		if (weapon)
		{
			auto viewmodel = (entity_t*)m_entitylist()->GetClientEntityFromHandle(g_ctx.local()->m_hViewModel());

			if (viewmodel)
			{
				auto eyeAng = viewsetup->angles;
				eyeAng.z -= (float)g_cfg.esp.viewmodel_roll;

				viewmodel->set_abs_angles(eyeAng);
			}

			if (weapon->is_grenade() && g_cfg.esp.grenade_prediction && g_cfg.player.enable)
				GrenadePrediction::get().View(viewsetup, weapon);
		}

		if (g_cfg.player.enable && (g_cfg.misc.thirdperson_toggle.key > KEY_NONE && g_cfg.misc.thirdperson_toggle.key < KEY_MAX || g_cfg.misc.thirdperson_when_spectating))
			thirdperson(fakeducking);
		else
		{
			g_ctx.globals.in_thirdperson = false;
			m_input()->m_fCameraInThirdPerson = false;
		}

		original_fn(viewsetup);

		if (fakeducking)
		{
			viewsetup->origin = g_ctx.local()->GetAbsOrigin() + Vector(0.0f, 0.0f, m_gamemovement()->GetPlayerViewOffset(false).z);

			if (m_input()->m_fCameraInThirdPerson)
			{
				auto camera_angles = Vector(m_input()->m_vecCameraOffset.x, m_input()->m_vecCameraOffset.y, 0.0f); //-V807
				auto camera_forward = ZERO;

				math::angle_vectors(camera_angles, camera_forward);
				math::VectorMA(viewsetup->origin, -m_input()->m_vecCameraOffset.z, camera_forward, viewsetup->origin);
			}
		}
	}
	else
		return original_fn(viewsetup);
}

void thirdperson(bool fakeducking)
{
	if (g_ctx.local()->is_alive() && g_ctx.globals.in_thirdperson)
	{
		Vector angles;
		m_engine()->GetViewAngles(angles);

		auto forward = ZERO;
		math::angle_vectors(angles, forward);

		angles.z = g_cfg.misc.thirdperson_distance;

		trace_t trace;
		CTraceFilterWorldAndPropsOnly filter;

		Ray_t ray;

		auto eye_pos = fakeducking ? g_ctx.globals.stand_eye_pos : g_ctx.globals.eye_pos;
		ray.Init(eye_pos, eye_pos - forward * angles.z, Vector(-16.0f, -16.0f, -16.0f), Vector(16.0f, 16.0f, 16.0f));

		m_trace()->TraceRay(ray, MASK_NPCWORLDSTATIC, &filter, &trace);

		math::clamp(trace.fraction, 0.0f, 1.0f);
		angles.z *= trace.fraction;
		
		m_input()->m_fCameraInThirdPerson = true;
		m_input()->m_vecCameraOffset = angles;
	}
	else if (m_input()->m_fCameraInThirdPerson)
	{
		g_ctx.globals.in_thirdperson = false;
		m_input()->m_fCameraInThirdPerson = false;
	}

	static auto require_reset = false;

	if (g_ctx.local()->is_alive())
	{
		require_reset = false;
		return;
	}

	if (g_cfg.misc.thirdperson_when_spectating)
	{
		if (require_reset)
			g_ctx.local()->m_iObserverMode() = OBS_MODE_CHASE;

		if (g_ctx.local()->m_iObserverMode() == OBS_MODE_IN_EYE)
			require_reset = true;
	}
}
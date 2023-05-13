#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"
#include "..\lagcompensation\lagcompensation.h"
#include "..\lagcompensation\backtrack.h"
#include "..\fakewalk\slowwalk.h"

enum hitgroup
{
	UNKNOWN,
	HEAD,
	BODY,
	LEGS,
	ARMS
};

class aimbot : public singleton <aimbot>
{
	struct targets
	{
		int can_hurt_ticks = 0;
		adjust_data best_record;
		Vector aim_point = ZERO;
		int hitbox = -1;
		float fov = 0.0f;
		float distance = 0.0f;
		int health = 0;
		int damage = -1;

		void init(float fov_t, float distance_t, int health_t)
		{
			fov = fov_t;
			distance = distance_t;
			health = health_t;
		}

		void reset()
		{
			can_hurt_ticks = 0;
			best_record.reset();
			aim_point.Zero();
			hitbox = -1;
			fov = 0.0f;
			distance = 0.0f;
			health = 0;
			damage = -1;
		}
	};

	struct aimbotconfig_t
	{
		bool silent_aim;
		int selection_type;
		int fov;
		bool autoshoot;
		bool autowall;
		bool autoscope;
		bool ignore_limbs;
		bool double_tap_hitchance;
		int double_tap_hitchance_amount;
		bool skip_hitchance_if_low_inaccuracy;
		bool hitchance;
		int hitchance_amount;
		bool accuracy_boost;
		int accuracy_boost_amount;
		int minimum_visible_damage;
		int minimum_damage;
		int minimum_override_damage;
		std::vector <int> hitscan;
		bool safe_body_points;
		float pointscale_head;
		float pointscale_chest;
		float pointscale_stomach;
		float pointscale_pelvis;
		float pointscale_legs;
		bool autostop;
		std::vector <int> autostop_modifiers;
		int baim_level;
		std::vector <int> baim;
	};
public:
	struct point
	{
		Vector position = ZERO;
		bool center = false;
	};

	struct scan_data
	{
		Vector aim_point = ZERO;
		int hitbox = -1;
		int damage = -1;
	};

	int target_id = -1;
	resolver_side target_side[65];

	bool should_stop = false;

	aimbotconfig_t config;
	targets target[65];

	Vector engine_angles = ZERO;
	Vector shoot_poisiton = ZERO;

	void create_move(CUserCmd* m_pcmd);
	void iterate_players(CUserCmd* m_pcmd);
	void aim(CUserCmd* m_pcmd, const Vector& aim_point);
	void best_point(player_t* e, scan_data& data, bool optimize_points, Vector eye_pos = g_ctx.globals.eye_pos);
	void getpoints(player_t* e, int hitbox_id, std::vector <point>& points, bool optimize_points);
	bool hitchance(player_t* e, Vector angles, const Vector& point, int hitbox, float chance, bool accuracy_boost_amount, bool intersection);
	bool can_hit_hitgroup(player_t* e, const Vector& start, const Vector& end, int hitbox);
	hitgroup get_hitgroup(int hitbox);
	std::vector <int> get_hitboxes(hitgroup hitgroup);
	void get_target(CUserCmd* m_pcmd);
	std::vector <int> hitboxes_from_vector(player_t* e, const std::vector <int>& arr, bool esp = false);
	void autostop(CUserCmd* m_pcmd);
	void update_config();
};
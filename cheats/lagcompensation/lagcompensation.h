#pragma once

#include "..\..\includes.hpp"
#include "..\..\sdk\structs.hpp"

enum
{
	SIDE_RESOLVER_FIRST,
	SIDE_RESOLVER_SECOND
};

enum resolver_side
{
	NONE,
	RIGHT,
	LEFT
};

class c_player_resolver
{
	void resolve();
	void check_aa();
	void check_side(int resolve_index);
	void check_lby();
	void pitch_resolve();
public:
	player_t* m_e = nullptr;
	float pitch = FLT_MAX;
	float resolve_yaw = 0.0f;
	float yaw = FLT_MAX;

	void run();
};

struct player_record_t
{
	player_t* m_e = nullptr;
	c_player_resolver* m_resolver = new c_player_resolver();
};

class adjust_data //-V730
{
public:
	player_t* player = nullptr;
	int i = -1;

	resolver_side side = NONE;

	AnimationLayer layers[15];
	matrix3x4_t matrix[MAXSTUDIOBONES];

	bool dormant = false;

	int flags = 0;
	int bone_count = 0;

	float simulation_time = 0.0f;
	float old_simulation_time = 0.0f;

	float duck_amount = 0.0f;
	float lby = 0.0f;

	Vector angles = ZERO;
	Vector abs_angles = ZERO;
	Vector velocity = ZERO;
	Vector origin = ZERO;
	Vector mins = ZERO;
	Vector maxs = ZERO;

	adjust_data() //-V730
	{
		reset();
	}

	adjust_data(const adjust_data& data) //-V730
	{
		player = data.player;
		i = data.i;

		side = data.side;

		memcpy(layers, data.layers, 15 * sizeof(AnimationLayer));
		memcpy(matrix, data.matrix, MAXSTUDIOBONES * sizeof(matrix3x4_t));

		dormant = data.dormant;

		flags = data.flags;
		bone_count = data.bone_count;

		simulation_time = data.simulation_time;
		old_simulation_time = data.old_simulation_time;

		duck_amount = data.duck_amount;
		lby = data.lby;

		angles = data.angles;
		abs_angles = data.abs_angles;
		velocity = data.velocity;
		origin = data.origin;
		mins = data.mins;
		maxs = data.maxs;
	}

	adjust_data& operator=(const adjust_data& data) //-V730
	{
		player = data.player;
		i = data.i;

		side = data.side;

		memcpy(layers, data.layers, 15 * sizeof(AnimationLayer));
		memcpy(matrix, data.matrix, MAXSTUDIOBONES * sizeof(matrix3x4_t));

		dormant = data.dormant;

		flags = data.flags;
		bone_count = data.bone_count;

		simulation_time = data.simulation_time;
		old_simulation_time = data.old_simulation_time;

		duck_amount = data.duck_amount;
		lby = data.lby;

		angles = data.angles;
		abs_angles = data.abs_angles;
		velocity = data.velocity;
		origin = data.origin;
		mins = data.mins;
		maxs = data.maxs;

		return *this;
	}

	void reset()
	{
		player = nullptr;
		i = -1;

		side = NONE;
		dormant = false;

		flags = 0;
		bone_count = 0;

		simulation_time = 0.0f;
		old_simulation_time = 0.0f;

		duck_amount = 0.0f;
		lby = 0.0f;

		angles.Zero();
		abs_angles.Zero();
		velocity.Zero();
		origin.Zero();
		mins.Zero();
		maxs.Zero();
	}

	adjust_data(player_t* e)
	{
		store_data(e);
	}

	void store_data(player_t* e)
	{
		if (!e->is_alive(false))
			return;

		player = e;

		i = player->EntIndex();
		side = (resolver_side)g_ctx.globals.side[i];

		memcpy(layers, player->get_animlayers(), player->animlayer_count() * sizeof(AnimationLayer));
		memcpy(matrix, player->m_CachedBoneData().Base(), player->get_bone_count() * sizeof(matrix3x4_t));

		dormant = player->IsDormant();

		flags = player->m_fFlags();
		bone_count = player->get_bone_count();

		simulation_time = player->m_flSimulationTime();
		old_simulation_time = player->m_flOldSimulationTime();

		duck_amount = player->m_flDuckAmount();
		lby = player->m_flLowerBodyYawTarget();

		angles = player->m_angEyeAngles();
		abs_angles = player->GetAbsAngles();
		velocity = player->m_vecVelocity();
		origin = player->GetAbsOrigin();
		mins = player->GetCollideable()->OBBMins();
		maxs = player->GetCollideable()->OBBMaxs();
	}

	void adjust_player()
	{
		if (!valid())
			return;

		memcpy(player->get_animlayers(), layers, player->animlayer_count() * sizeof(AnimationLayer));
		memcpy(player->m_CachedBoneData().Base(), matrix, bone_count * sizeof(matrix3x4_t));

		player->m_fFlags() = flags;
		player->get_bone_count() = bone_count;

		player->m_flSimulationTime() = simulation_time;
		player->m_flOldSimulationTime() = old_simulation_time;

		player->m_flDuckAmount() = duck_amount;
		player->m_flLowerBodyYawTarget() = lby;

		player->m_angEyeAngles() = angles;
		player->set_abs_angles(abs_angles);
		player->m_vecVelocity() = velocity;
		player->set_abs_origin(origin);
		player->GetCollideable()->OBBMins() = mins;
		player->GetCollideable()->OBBMaxs() = maxs;
	}

	bool valid()
	{
		if (!player)
			return false;

		if (!player->is_alive(false))
			return false;

		return true;
	}
};

class lagcompensation : public singleton <lagcompensation>
{
public:
	void fsn(ClientFrameStage_t stage, int force_update);

	bool valid(int i, player_t* e, ClientFrameStage_t stage);

	void apply_interpolation_flags(player_t* e, bool flag);

	void min_delta(player_t* e, player_record_t& player_record);
	void max_delta(player_t* e, player_record_t& player_record);
	void update_player_animations(player_t* e, player_record_t& player_record);

	float get_interpolation();

	player_record_t players[65];
	adjust_data player_data[65];

	bool is_dormant[65];
	float previous_yaw[65];
	int first_ticks[65];
	int second_ticks[65];
};
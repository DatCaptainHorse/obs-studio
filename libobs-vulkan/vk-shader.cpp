/******************************************************************************
    Copyright (C) 2022 by Kristian Ollikainen <kristiankulta2000@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "vk-subsystem.hpp"

void gs_shader_destroy(gs_shader_t *shader)
{
	UNUSED_PARAMETER(shader);
}

int gs_shader_get_num_params(const gs_shader_t *shader)
{
	UNUSED_PARAMETER(shader);
	return 0;
}

gs_sparam_t *gs_shader_get_param_by_idx(gs_shader_t *shader, uint32_t param)
{
	UNUSED_PARAMETER(shader);
	UNUSED_PARAMETER(param);
	return nullptr;
}

gs_sparam_t *gs_shader_get_param_by_name(gs_shader_t *shader, const char *name)
{
	UNUSED_PARAMETER(shader);
	UNUSED_PARAMETER(name);
	return nullptr;
}

gs_sparam_t *gs_shader_get_viewproj_matrix(const gs_shader_t *shader)
{
	UNUSED_PARAMETER(shader);
	return nullptr;
}

gs_sparam_t *gs_shader_get_world_matrix(const gs_shader_t *shader)
{
	UNUSED_PARAMETER(shader);
	return nullptr;
}

void gs_shader_get_param_info(const gs_sparam_t *param,
			      struct gs_shader_param_info *info)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(info);
}

void gs_shader_set_bool(gs_sparam_t *param, bool val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_float(gs_sparam_t *param, float val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_int(gs_sparam_t *param, int val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_matrix3(gs_sparam_t *param, const struct matrix3 *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_matrix4(gs_sparam_t *param, const struct matrix4 *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_vec2(gs_sparam_t *param, const struct vec2 *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_vec3(gs_sparam_t *param, const struct vec3 *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_vec4(gs_sparam_t *param, const struct vec4 *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_texture(gs_sparam_t *param, gs_texture_t *val)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_val(gs_sparam_t *param, const void *val, size_t size)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(val);
}

void gs_shader_set_default(gs_sparam_t *param)
{
	UNUSED_PARAMETER(param);
}

void gs_shader_set_next_sampler(gs_sparam_t *param, gs_samplerstate_t *sampler)
{
	UNUSED_PARAMETER(param);
	UNUSED_PARAMETER(sampler);
}

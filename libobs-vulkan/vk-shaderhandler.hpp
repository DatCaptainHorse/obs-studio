/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

/*
 *  Modified for Vulkan use
 */

#pragma once

#include <string>
#include <vector>
#include <memory>

#include "vk-includes.hpp"
#include <graphics/shader-parser.h>

struct ShaderParser : shader_parser {
	inline ShaderParser() { shader_parser_init(this); }
	inline ~ShaderParser() { shader_parser_free(this); }
};

struct ShaderProcessor {
	gs_device_t *device;
	ShaderParser parser;

	void BuildInputLayout(ShaderInputs &inputs);
	void BuildParams(std::vector<gs_shader_param> &params);
	void
	BuildSamplers(std::vector<std::unique_ptr<gs_samplerstate_t>> &samplers);
	void BuildString(std::string &outputString);
	void Process(const char *shader_string, const char *file);
	std::string Vulkanify(gs_shader_t *shader, const std::string &shaderCode, bool resetBindIndex);

	inline ShaderProcessor(gs_device_t *_device) : device(_device) {}
};

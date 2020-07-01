#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>

#include "density_mech_helper.hpp"

arb::mechanism_desc read_dynamics_params_point(std::string fname);

// Return map from mechsnim `group` (group that has the same overwritble variable) to the contents of the group
std::unordered_map<std::string, mech_groups> read_dynamics_params_density_base(std::string fname);

// Return map from mechsnim `group` (group that has the same overwritble variable) to the overrides of the variables of that group
std::unordered_map<std::string, variable_map> read_dynamics_params_density_override(std::string fname);
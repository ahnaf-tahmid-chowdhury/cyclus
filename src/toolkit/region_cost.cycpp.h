/// This includes the required header to add regional costs to archetypes.
/// One should only need to:
/// - '#include "toolkit/region_cost.cycpp.h"' in the header of the
///    archetype class (as private)
/// - Add `InitEconParameters()` to `EnterNotify()` in the cc file of the
///   archetype class.

/// How to add parameters to this file:
/// 1. Add the pragma. A default value MUST be added to ensure backwards
///    compatibility.
/// 2. Edit the unordered_map called "econ_params"
///          i. add the desired parameter to the array {"name", value}
///         ii. the value of the pair should be the variable name exactly
/// 3. Add "std::vector<int> cycpp_shape_<param_name> = {0};" to the end of the
///    file with the other ones, reaplcing <param_name> with the name you put
///    in the econ_params array (again, must match exactly).

// clang-format off
#pragma cyclus var { \
    "default": 0.0, \
    "uilabel": "Corporate Income Tax Rate as decimal", \
    "range": [0.0, 1.0], \
    "doc": "Income Tax Rate for all facilities belonging to this region as decimal (1% --> 0.01)", \
    "units": "Dimensionless" \
    }
double corporate_income_tax_rate;
// clang-format on

// Must be done in a function so that we can access the user-defined values
std::unordered_map<std::string, double> GenerateParamList() const {
  std::unordered_map<std::string, double> econ_params{
      {"corporate_income_tax_rate", corporate_income_tax_rate}};

  return econ_params;
}

// Required for compilation but not added by the cycpp preprocessor. Do not
// remove. Must be one for each variable.
std::vector<int> cycpp_shape_corporate_income_tax_rate = {0};
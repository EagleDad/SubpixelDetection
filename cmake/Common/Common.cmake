# Standard configuration for build directory:
include("${CMAKE_CURRENT_LIST_DIR}/BuildLocation.cmake")


include("${CMAKE_CURRENT_LIST_DIR}/CopyRuntimeDependencies.cmake")

# Add more common tools here

# The libray name that is set for the ALIAS with "::" as "MyDll::MyDll" can nor be 
# used for the add library command. So we have to replace "::" with "_"
macro(get_raw_target_name raw_target_name var_name)
    string(REPLACE "::" "_" ${var_name} ${raw_target_name})
    string(TOLOWER ${${var_name}} ${var_name})
endmacro()

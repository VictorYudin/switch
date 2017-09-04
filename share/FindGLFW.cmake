
set(GLFW_LOCATION
	"${GLFW_LOCATION}"
	CACHE
	PATH
	"Directory to search for GLFW")


find_library(GLFW_LIBRARY
	NAMES
	glfw3
	PATHS
	${GLFW_LOCATION}/lib
	${GLFW_LOCATION}/bin)


find_path(GLFW_INCLUDE_DIR
	NAMES
	GLFW/glfw3.h
	PATHS
	${GLFW_LOCATION}
	PATH_SUFFIXES
	include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
	"GLFW"
	DEFAULT_MSG
	GLFW_LIBRARY
	GLFW_INCLUDE_DIR)


if(GLFW_FOUND)
	set(GLFW_LIBRARY "${GLFW_LIBRARY}")
	set(GLFW_INCLUDE_DIR "${GLFW_INCLUDE_DIR}")
	mark_as_advanced(GLFW_LOCATION)
endif()

mark_as_advanced(GLFW_INCLUDE_DIR GLFW_LIBRARY)

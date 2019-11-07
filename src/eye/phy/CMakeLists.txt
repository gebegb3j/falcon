
file(GLOB SOURCES "*.cc" "*.c")

add_library(eye_phy STATIC ${SOURCES})
target_link_libraries(eye_phy falcon_phy falcon_prof)

target_compile_options(eye_phy PUBLIC "-std=c++11")

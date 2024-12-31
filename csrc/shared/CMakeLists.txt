# Add the shared library
add_library(chinese_checkers_shared
    chinese_checkers.cpp
    chinese_checkers.h
    constants.h
    board.h)

# Set include directories for the shared library
target_include_directories(chinese_checkers_shared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

# Make sure symbols are exported (important for shared libraries)
set_target_properties(chinese_checkers_shared PROPERTIES PUBLIC_HEADER chinese_checkers.h)
set_target_properties(chinese_checkers_shared PROPERTIES DEFINE_SYMBOL "chinese_checkers_shared_EXPORTS")

# Install headers (optional)
install(FILES chinese_checkers.h constants.h board.h DESTINATION include)
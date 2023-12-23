# Add sources from the current directory to the project target.
# We pass the target that needs the sources as the first argument,
# then a visibility parameter for the sources (which should normally be PRIVATE).
# Finally, we supply a list of source files that will be built into the target.

file(GLOB 
    sources 
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h* 
        ${CMAKE_CURRENT_SOURCE_DIR}/*.c*
        )

target_sources("${PROJECT_NAME}"
    PRIVATE
        ${sources}
        )

# Add subdirectories with source files, if present
# add_subdirectory(<name>)
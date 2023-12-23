### JUCE CMake Audio Plugin Template ###
cmake_minimum_required(VERSION 3.22)
project(NDI-Audio-IO VERSION 0.0.0)

message(STATUS ${CMAKE_SYSTEM_NAME})

# set(CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 12)
# set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "ABC12345DE")
# set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development" CACHE STRING "")

set(PRODUCT_NAME "NDI Audio IO")
set(COMPANY_NAME "ak5k") 

set(PLUGIN_CODE "Ndi1")
set(COMPANY_CODE "ak5K")

include(CTest)
enable_testing()

# dependencies
set(LIB_JUCE_TAG "7.0.7")

include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${PROJECT_SOURCE_DIR}/external CACHE PATH "External dependencies path." FORCE)

FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${LIB_JUCE_TAG}
    GIT_SHALLOW    TRUE
    GIT_CONFIG     advice.detachedHead=false    # Disable detached HEAD warning for fetching a specific tag
    SOURCE_DIR     "${FETCHCONTENT_BASE_DIR}/JUCE"
    )

message(STATUS "Downloading JUCE")
FetchContent_Populate(juce)

if(WIN32 AND NOT NO_ASIO)
    # ASIO-related path/file variables
    set(asio_download_root "https://download.steinberg.net/sdk_downloads")
    set(asio_file_name "asiosdk_2.3.3_2019-06-14.zip")
    set(asio_dir_name "asiosdk_2.3.3_2019-06-14")
    set(asio_working_dir "${CMAKE_CURRENT_SOURCE_DIR}/external")
    set(asio_output_path "${asio_working_dir}/${asio_file_name}")

    message(STATUS "Downloading ASIO SDK")
    file(DOWNLOAD "${asio_download_root}/${asio_file_name}" ${asio_output_path})
    file(SHA1 ${asio_output_path} asio_zip_hash)
    message("   ASIO SDK SHA1: ${asio_zip_hash}")

    message("   Extracting ASIO SDK")
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar "xf" ${asio_output_path} --format=zip
      WORKING_DIRECTORY ${asio_working_dir}
      INPUT_FILE ${asio_output_path}
    )

    # Set the ASIO SDK path for the caller
endif()

file(READ
    ${PROJECT_SOURCE_DIR}/license.txt
    LICENSE)

### Plugin Project Setup ###

# If you've installed JUCE somehow (via a package manager, or directly using the CMake install
# target), you'll need to tell this project that it depends on the installed copy of JUCE. If you've
# included JUCE directly in your source tree (perhaps as a submodule), you'll need to tell CMake to
# include that subdirectory as part of the build.

# find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
# or
# add_subdirectory(JUCE)                    # If you've put JUCE in a subdirectory called JUCE


# JUCE JUCE JUCE
add_subdirectory(${FETCHCONTENT_BASE_DIR}/JUCE EXCLUDE_FROM_ALL)

# If you are building a VST2 or AAX plugin, CMake needs to be told where to find these SDKs on your
# system. This setup should be done before calling `juce_add_plugin`.

# juce_set_vst2_sdk_path(...)
# juce_set_aax_sdk_path(${PROJECT_SOURCE_DIR}/external/aax-sdk-2-5-1/)

# `juce_add_plugin` adds a static library target with the name passed as the first argument
# (AudioPluginExample here). This target is a normal CMake target, but has a lot of extra properties set
# up by default. As well as this shared code static library, this function adds targets for each of
# the formats specified by the FORMATS arguments. This function accepts many optional arguments.
# Check the readme at `docs/CMake API.md` in the JUCE repo for the full list.

juce_add_plugin(${PROJECT_NAME}
    FORMATS Standalone VST3 AU                  # The formats to build. Valid formats: Standalone Unity VST3 AU AUv3 AAX VST LV2.
                                                # AU and AUv3 plugins will only be enabled when building on macOS.
    PRODUCT_NAME "${PRODUCT_NAME}"              # The name of the final executable, which can differ from the target name.
    # PLUGIN_NAME                               # Name of the plugin that will be displayed in the DAW. Can differ from PRODUCT_NAME.

    # ICON_BIG                                  # ICON_* arguments specify a path to an image file to use as an icon for the Standalone.
    # ICON_SMALL
    COMPANY_NAME "${COMPANY_NAME}"                           # The name of this target's author. The value is inherited from JUCE_COMPANY_NAME.
    # COMPANY_WEBSITE                           # The address of a website related to this target in some way.
                                                # The value is inherited from JUCE_COMPANY_WEBSITE.
    # COMPANY_EMAIL                             # An email address for this target's author. The value is inherited from JUCE_COMPANY_EMAIL.
    PLUGIN_MANUFACTURER_CODE "${COMPANY_CODE}"               # A four-character manufacturer id with at least one upper-case character.
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case.
    PLUGIN_CODE "${PLUGING_CODE}"                            # A unique four-character plugin id with exactly one upper-case character.
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case.

    IS_SYNTH FALSE                              # Is this a synth or an effect?
    NEEDS_MIDI_INPUT FALSE                      # Does the plugin need midi input?
    NEEDS_MIDI_OUTPUT FALSE                     # Does the plugin need midi output?
    IS_MIDI_EFFECT FALSE                        # Is this plugin a MIDI effect?
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE           # Does the editor need keyboard focus?

    # VST3_CATEGORIES                           # Should be one or more, separated by spaces, of the following:
                                                # Fx, Instrument, Analyzer, Delay, Distortion, Drum, Dynamics, EQ, External,
                                                # Filter, Generator, Mastering, Modulation, Mono, Network, NoOfflineProcess,
                                                # OnlyOfflineProcess, OnlyRT, Pitch Shift, Restoration, Reverb, Sampler, Spatial,
                                                # Stereo, Surround, Synth, Tools, Up-Downmix
    # AU_MAIN_TYPE                              # Should be one or more, separated by spaces, of the following:
                                                # kAudioUnitType_Effect, kAudioUnitType_FormatConverter, kAudioUnitType_Generator,
                                                # kAudioUnitType_MIDIProcessor, kAudioUnitType_Mixer, kAudioUnitType_MusicDevice,
                                                # kAudioUnitType_MusicEffect, kAudioUnitType_OfflineEffect, kAudioUnitType_Output,
                                                # kAudioUnitType_Panner


    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "${PRODUCT_NAME}: audio input access needed"
    
    VST3_AUTO_MANIFEST FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE

    )                                           # Should the plugin be installed to a default location after building?
                                                # NOTE: Writing to the default install locations might require administrator privileges on Windows.

# include global (system) headers
FILE(GLOB external_includes 
    ${PROJECT_SOURCE_DIR}/external/*/include
    ${PROJECT_SOURCE_DIR}/external/asiosdk*/common
    )

target_include_directories(${PROJECT_NAME}
    SYSTEM PRIVATE
        ${external_includes}
        )

# ASIO audio
if (MSVC)
target_compile_definitions(
    ${PROJECT_NAME} 
    PUBLIC 
    JUCE_ASIO=1)
endif()

# build ios ndi lib from pieces
if(IOS AND NOT EXISTS ${PROJECT_SOURCE_DIR}/external/ndi5-sdk/lib/iOS/libndi_ios.a)
    FILE(GLOB files 
        ${PROJECT_SOURCE_DIR}/external/ndi5-sdk/lib/iOS/x*
        )
    foreach(file ${files})
        file(READ ${file} buffer HEX)
        file(APPEND ${PROJECT_SOURCE_DIR}/temp "${buffer}")
        message(STATUS "${file}")
    endforeach()
    execute_process(COMMAND xxd -r -p 
        ${PROJECT_SOURCE_DIR}/temp 
        ${PROJECT_SOURCE_DIR}/external/ndi5-sdk/lib/iOS/libndi_ios.a
        )

    file(REMOVE ${PROJECT_SOURCE_DIR}/temp)
    
endif()

# "auto install" plugin if debug
if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Release")
    message(STATUS "not release")
    juce_enable_copy_plugin_step(${PROJECT_NAME})
endif()

# `juce_generate_juce_header` will create a JuceHeader.h for a given target, which will be generated
# into your build tree. This should be included with `#include <JuceHeader.h>`. The include path for
# this header will be automatically added to the target. The main function of the JuceHeader is to
# include all your JUCE module headers; if you're happy to include module headers directly, you
# probably don't need to call this.

juce_generate_juce_header(${PROJECT_NAME})

# add sources
add_subdirectory(Source)

# `target_compile_definitions` adds some preprocessor definitions to our target.
# JUCE modules also make use of compile definitions to switch certain features on/off,
# so if there's a particular feature you need that's not on by default, check the module header
# for the correct flag to set here.
# These definitions will be visible both to your code, and also the JUCE module code.

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
        )

# If your target needs extra binary assets, you can add them here.
# NOTE: Conversion to binary-data happens when the target is built.

# juce_add_binary_data(AudioPluginData  # Name of a static library target that will include all the binary resources
    # HEADER_NAME ...                   # Name of the generated header. "BinaryData.h" is used by default
    # NAMESPACE ...                     # Namespace of the generated binary data class. "BinaryData" is used by default
#   SOURCES ...)                        # List of asset files of any kind that should be built into the static library


if (IOS)
    add_library(ndi5::sdk STATIC IMPORTED)
    if(IOS)
        target_link_libraries(ndi5::sdk INTERFACE 
            $<LINK_LIBRARY:FRAMEWORK,CoreMedia>
            $<LINK_LIBRARY:FRAMEWORK,CoreVideo>
            $<LINK_LIBRARY:FRAMEWORK,VideoToolbox>
            )
    endif()
else()
    add_library(ndi5::sdk SHARED IMPORTED)
endif()


# handle NDI
if(MSVC)
set_property(TARGET ndi5::sdk 
    PROPERTY IMPORTED_LOCATION 
        ${PROJECT_SOURCE_DIR}/external/ndi5-sdk/bin/x64/Processing.NDI.Lib.x64.dll
    )
set_property(TARGET ndi5::sdk
    PROPERTY IMPORTED_IMPLIB
        ${PROJECT_SOURCE_DIR}/external/ndi5-sdk/lib/x64/Processing.NDI.Lib.x64.lib
    )
elseif(IOS)
set_property(TARGET ndi5::sdk 
    PROPERTY IMPORTED_LOCATION
        ${PROJECT_SOURCE_DIR}/external/ndi5-sdk/lib/iOS/libndi_ios.a
    )
elseif(APPLE)
set_property(TARGET ndi5::sdk 
    PROPERTY IMPORTED_LOCATION
        ${PROJECT_SOURCE_DIR}/external/ndi5-sdk/lib/libndi.dylib
    )
elseif(UNIX AND NOT APPLE)
set_property(TARGET ndi5::sdk 
    PROPERTY IMPORTED_LOCATION 
        ${PROJECT_SOURCE_DIR}/external/ndi5-sdk/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu/libndi.so.5
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ndi5::sdk)

# `target_link_libraries` links libraries and JUCE modules to other libraries or executables. Here,
# we're linking our executable target to the `juce::juce_audio_utils` module. Inter-module
# dependencies are resolved automatically.

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# installers
include(GNUInstallDirs)
set(CPACK_THREADS 0)
set(CPACK_PACKAGE_NAME "${PRODUCT_NAME}")
set(CPACK_PACKAGE_VENDOR "${COMPANY_NAME}")
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/cpack)
string(REGEX REPLACE " " "_" file_name "${PRODUCT_NAME}")
string(TOLOWER "${file_name}" file_name)
set(CPACK_PACKAGE_FILE_NAME ${file_name}_installer)

if(NOT APPLE)
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PRODUCT_NAME}")
endif()

# Library component is NDI library
if(MSVC)
    set(CPACK_GENERATOR "NSIS") # note: nsis variables also
    set(VST3_DEST ".") # ignored by nsis
    set(AAX_DEST ".") # ignored by nsis

    set(CPACK_NSIS_MANIFEST_DPI_AWARE 1) # hires

    # for startmenu
    set(CPACK_PACKAGE_EXECUTABLES "${PRODUCT_NAME}" "${PRODUCT_NAME}")

    # appdata is nsis var
    set(CPACK_NSIS_Library_INSTALL_DIRECTORY "\$APPDATA\\${PRODUCT_NAME}") # NDI dll install

    # sane path to VST3 dest
    get_property(VST3_NSIS GLOBAL PROPERTY JUCE_VST3_COPY_DIR)
    file(TO_CMAKE_PATH "${VST3_NSIS}" VST3_NSIS)
    file(TO_NATIVE_PATH "${VST3_NSIS}" VST3_NSIS)
    set(CPACK_NSIS_VST3_INSTALL_DIRECTORY "${VST3_NSIS}") # vst3 to common files

    get_property(AAX_NSIS GLOBAL PROPERTY JUCE_AAX_COPY_DIR)
    file(TO_CMAKE_PATH "${AAX_NSIS}" AAX_NSIS)
    file(TO_NATIVE_PATH "${AAX_NSIS}" AAX_NSIS)
    set(CPACK_NSIS_AAX_INSTALL_DIRECTORY "${AAX_NSIS}") # vst3 to common files

    set(NDILIB_DEST "$ENV{PROGRAMDATA}\\${PRODUCT_NAME}" )
    
    # declare to be installed with "dummy" paths
    install(TARGETS ${PROJECT_NAME}_Standalone RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Standalone)
    # runtime dest !!!
    install(IMPORTED_RUNTIME_ARTIFACTS ndi5::sdk RUNTIME DESTINATION . COMPONENT Library)

elseif(UNIX AND NOT APPLE)
    # install to user paths
    set(CPACK_GENERATOR "STGZ")
    set(CMAKE_INSTALL_PREFIX "/" CACHE PATH "Target path" FORCE)
    set(APP_DEST "./opt/${PRODUCT_NAME}/" )
    set(NDILIB_DEST "./.config/${PRODUCT_NAME}/" )
    set(VST3_DEST "./.vst3/${PRODUCT_NAME}.vst3/Contents/x86_64-linux/" )
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "")
    set(CPACK_POST_BUILD_SCRIPTS ${PROJECT_SOURCE_DIR}/cmake/linux-installer-post.cmake)

    install(IMPORTED_RUNTIME_ARTIFACTS ndi5::sdk LIBRARY DESTINATION "${NDILIB_DEST}" COMPONENT Library)
    install(TARGETS ${PROJECT_NAME}_Standalone RUNTIME DESTINATION "${APP_DEST}" COMPONENT Standalone)

elseif(APPLE AND NOT IOS)
    # install using RELATIVE PATHS from Applications!!!
    set(CPACK_GENERATOR "productbuild")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/Applications")
    set(APP_DEST "${PRODUCT_NAME}/" )
    # set(NDILIB_DEST "../Library/Application Support/${PRODUCT_NAME}/" )
    set(NDILIB_DEST "../usr/local/lib" )
    set(VST3_DEST "../Library/Audio/Plug-Ins/VST3" )
    set(AU_DEST "../Library/Audio/Plug-Ins/Components" )
    set(AAX_DEST "../Library/Application Support/Avid/Audio/Plug-Ins")

    # set_target_properties(${PROJECT_NAME}_Standalone PROPERTIES INSTALL_RPATH ${PROJECT_SOURCE_DIR}/external/ndi5-sdk/lib)
    set(CMAKE_MACOSX_RPATH 1)
    set(CMAKE_INSTALL_NAME_DIR "/usr/local/lib")
    set_target_properties(${PROJECT_NAME}_Standalone PROPERTIES BUNDLE TRUE)

    install(TARGETS ${PROJECT_NAME}_Standalone RUNTIME DESTINATION . BUNDLE DESTINATION . COMPONENT Standalone)
    install(IMPORTED_RUNTIME_ARTIFACTS ndi5::sdk LIBRARY DESTINATION "${NDILIB_DEST}" COMPONENT Library)
    install(TARGETS ${PROJECT_NAME}_AU LIBRARY DESTINATION "${AU_DEST}" COMPONENT AU)

elseif(IOS)
    # install using RELATIVE PATHS from Applications!!!
    set(CPACK_GENERATOR "productbuild")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/Applications")
    set(APP_DEST "${PRODUCT_NAME}/" )
    set(NDILIB_DEST "../Library/Application Support/${PRODUCT_NAME}" )
    set(VST3_DEST "../Library/Audio/Plug-Ins/VST3" )
    set(AU_DEST "../Library/Audio/Plug-Ins/Components" )

    set_target_properties(${PROJECT_NAME}_Standalone PROPERTIES INSTALL_RPATH ${PROJECT_SOURCE_DIR}/external/ndi5-sdk/lib)
    set_target_properties(${PROJECT_NAME}_Standalone PROPERTIES BUNDLE TRUE)

    install(TARGETS ${PROJECT_NAME}_Standalone RUNTIME DESTINATION . BUNDLE DESTINATION . COMPONENT Standalone)
endif()

if(NOT IOS AND NOT ANDROID)
    install(TARGETS ${PROJECT_NAME}_VST3 LIBRARY DESTINATION "${VST3_DEST}" COMPONENT VST3)
  #  install(TARGETS ${PROJECT_NAME}_AAX LIBRARY DESTINATION "${AAX_DEST}" COMPONENT AAX)
endif()

set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/license.txt)
set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/readme.txt)
install(FILES ${PROJECT_SOURCE_DIR}/license.txt DESTINATION . COMPONENT Standalone)


# remove all other targets from installer
get_cmake_property(CPACK_COMPONENTS_ALL COMPONENTS)
list(REMOVE_ITEM CPACK_COMPONENTS_ALL "Unspecified")

include(CPack)
cpack_add_component(Standalone REQUIRED TRUE)
cpack_add_component(Library REQUIRED TRUE)
if(NOT IOS)
    cpack_add_component(VST3)
    #cpack_add_component(AAX)
    cpack_add_component(AU)
endif()



# for clang-tidy(this enable to find system header files).
if(CMAKE_EXPORT_COMPILE_COMMANDS)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
endif()


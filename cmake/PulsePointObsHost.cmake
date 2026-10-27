# pp_obs_host(<slug> NAME "<nome do produto>" VST3 "<arquivo .vst3>")
#
# Cria o alvo obs_<slug>: o módulo pulsepoint-<slug> (.plugin no Mac, .dll no Windows).
#   NAME  o nome que aparece no OBS, igual ao do plugin na DAW
#   VST3  o nome do bundle que o instalador põe na pasta padrão de VST3
#
# O libobs não é linkado: as funções são resolvidas no processo do OBS (src/ObsApi.cpp).

set(PP_OBS_ROOT ${CMAKE_CURRENT_LIST_DIR}/..)

function(pp_obs_host slug)
    cmake_parse_arguments(A "" "NAME;VST3" "" ${ARGN})
    if(NOT A_NAME OR NOT A_VST3)
        message(FATAL_ERROR "pp_obs_host(${slug}): faltou NAME ou VST3")
    endif()

    set(target obs_${slug})
    add_library(${target} MODULE
        ${PP_OBS_ROOT}/src/Module.cpp
        ${PP_OBS_ROOT}/src/ObsApi.cpp
        ${PP_OBS_ROOT}/src/PluginHost.cpp)

    target_include_directories(${target} PRIVATE
        ${PP_OBS_ROOT}/src
        ${PP_OBS_ROOT}/third_party/obs-studio/libobs)

    target_compile_definitions(${target} PRIVATE
        PP_OBS_SLUG="${slug}"
        PP_OBS_NAME="${A_NAME}"
        PP_OBS_VST3="${A_VST3}"
        PP_OBS_HOST_VERSION="${PP_OBS_HOST_VERSION}"
        JUCE_STANDALONE_APPLICATION=0
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_MODAL_LOOPS_PERMITTED=0)

    target_link_libraries(${target} PRIVATE
        juce::juce_audio_processors
        juce::juce_gui_basics
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

    # Cada módulo leva o seu JUCE: nada dele pode vazar para o processo do OBS, onde outros
    # módulos (e o próprio VST3) carregam outras cópias.
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME pulsepoint-${slug}
        PREFIX ""
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON)

    if(APPLE)
        # O OBS no Mac carrega ~/Library/Application Support/obs-studio/plugins/<módulo>.plugin
        set(PP_OBS_BUNDLE_ID com.pulsepoint.obs.${slug})
        set(PP_OBS_EXECUTABLE pulsepoint-${slug})
        configure_file(${PP_OBS_ROOT}/cmake/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/${target}-Info.plist @ONLY)
        set_target_properties(${target} PROPERTIES
            BUNDLE TRUE
            BUNDLE_EXTENSION plugin
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/${target}-Info.plist)
    endif()
endfunction()

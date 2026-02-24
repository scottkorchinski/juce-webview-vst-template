cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
project(ExciteLifelineModule VERSION 0.1.0)

juce_add_plugin(ExciteLifelineModule
  COMPANY_NAME "Excite Audio"
  BUNDLE_ID "com.exciteaudio.lifeline-module"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  PLUGIN_MANUFACTURER_CODE Exct
  PLUGIN_CODE ExLM
  FORMATS AU VST3 Standalone
  PRODUCT_NAME "Lifeline Module"
  NEEDS_WEB_BROWSER TRUE
  NEEDS_WEBVIEW2 TRUE
)

option(USE_LOCALHOST_UI "Load plugin UI from http://127.0.0.1:5173 (run 'cd web && npm run dev' first)" OFF)

get_target_property(PRODUCT_NAME ${PROJECT_NAME} JUCE_PRODUCT_NAME)
get_target_property(COMPANY_NAME ${PROJECT_NAME} JUCE_COMPANY_NAME)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  JUCE_PRODUCT_NAME="${PRODUCT_NAME}"
  JUCE_COMPANY_NAME="${COMPANY_NAME}"
  JUCE_PRODUCT_VERSION="${PROJECT_VERSION}"
)
if(USE_LOCALHOST_UI)
  target_compile_definitions(${PROJECT_NAME} PRIVATE USE_LOCALHOST_UI=1)
endif()

set(SOURCES
  Source/PluginEditor.cpp
  Source/PluginProcessor.cpp
)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_sources(${PROJECT_NAME}
  PRIVATE
  ${SOURCES}
  ${INCLUDE_DIR}/PluginEditor.h
  ${INCLUDE_DIR}/PluginProcessor.h
  ${INCLUDE_DIR}/ParameterIDs.h
)

target_include_directories(${PROJECT_NAME}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Source
  SYSTEM PUBLIC ${JUCE_MODULES_DIR}
)

# Copy JUCE frontend library into WebView assets (for embedding)
set(WEBVIEW_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/WebViewAssets)
file(COPY ${JUCE_SOURCE_DIR}/modules/juce_gui_extra/native/javascript
  DESTINATION ${WEBVIEW_ASSETS_DIR}/js/juce
)

# Zip WebView assets for BinaryData (archive must contain WebViewAssets/ prefix for C++ lookup)
set(WEBVIEW_ZIP_NAME "webview_assets.zip")
set(WEBVIEW_ZIP_PATH ${CMAKE_BINARY_DIR}/${WEBVIEW_ZIP_NAME})
get_filename_component(WEBVIEW_ASSETS_ABS ${WEBVIEW_ASSETS_DIR} ABSOLUTE)
get_filename_component(WEBVIEW_PARENT ${WEBVIEW_ASSETS_ABS} DIRECTORY)
cmake_path(GET WEBVIEW_ASSETS_ABS FILENAME ZIPPED_PREFIX)
set(ZIPPED_FILES_PREFIX "${ZIPPED_PREFIX}/")
target_compile_definitions(${PROJECT_NAME} PRIVATE ZIPPED_FILES_PREFIX="${ZIPPED_FILES_PREFIX}")

add_custom_target(ZipWebViewAssets
  COMMAND ${CMAKE_COMMAND} -E tar cvf "${WEBVIEW_ZIP_PATH}" --format=zip "${ZIPPED_PREFIX}"
  WORKING_DIRECTORY ${WEBVIEW_PARENT}
  BYPRODUCTS ${WEBVIEW_ZIP_PATH}
  COMMENT "Zipping WebView assets..."
  VERBATIM
)

juce_add_binary_data(WebViewAssets
  HEADER_NAME WebViewAssets.h
  NAMESPACE webview_assets
  SOURCES ${WEBVIEW_ZIP_PATH}
)
add_dependencies(WebViewAssets ZipWebViewAssets)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  juce::juce_audio_utils
  juce::juce_dsp
  WebViewAssets
  PUBLIC
  juce::juce_recommended_config_flags
  juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
  JUCE_WEB_BROWSER=1
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
)

set_source_files_properties(${SOURCES} PROPERTIES COMPILE_OPTIONS "${CXX_PROJECT_WARNINGS}")

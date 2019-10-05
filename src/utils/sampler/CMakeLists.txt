set(sampler_SOURCES sampler.cpp)

#set_directory_properties(PROPERTIES CORRADE_USE_PEDANTIC_FLAGS ON)
add_executable(sampler ${sampler_SOURCES})

target_link_libraries(sampler
  PRIVATE
    assets
    gfx
    nav
    scene
)

# copy JS/HTML/CSS scaffolding for WebGL build
if(CORRADE_TARGET_EMSCRIPTEN)
  install(FILES
          sampler.html
      ${MAGNUM_EMSCRIPTENAPPLICATION_JS}
      ${MAGNUM_WEBAPPLICATION_CSS}
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

#pragma once
// UNNAMED NEON GRAPPLER GAME
// Graphics like beatsaber, first person grappler

#define USE_DEBUG_MESSAGE_CALLBACK 1
// enables gamma correction
#define CORRECT_GAMMA 0

#define QUICK_LOADING 1

#define STDOUT_LOGGING 0
#define FILE_LOGGING 1

// Only enable logging if there's somewhere to log to
#define ENABLE_LOGGING (STDOUT_LOGGING || FILE_LOGGING)
// No point doing it in 'debug mode' if we're not even logging anything
#define ENABLE_DEBUG_MESSAGE_CALLBACK                                          \
  (USE_DEBUG_MESSAGE_CALLBACK && ENABLE_LOGGING)

/* TODO:
 * Make submeshes use a texture atlas
 * Use a renderer (at least to not redundantly set textures the whole time)
 * Make textures in models a unique pointer
 * When #embed is here: use that instead of header files
 */
/*
 * DONE (for showing in HPG)
 * MultiMesh
 * Frustum Culling
 * Window Class
 */

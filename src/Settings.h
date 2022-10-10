#pragma once
// UNNAMED NEON GRAPPLER GAME
// Graphics like beatsaber, first person grappler

#define USE_DEBUG_MESSAGE_CALLBACK 1

#define ENABLE_HDR 1 // only works when ENABLE_POSTPROCESSING is 1
#define ENABLE_BLOOM 1 // ENABLE_HDR is adviced when turning this option on, only works when ENABLE_POSTPROCESSING is 1

// enables gamma correction
#define CORRECT_GAMMA 0

// makes the thing parallel, can't be used with a renderer
#define QUICK_LOADING 1

#define ENABLE_DEFERRED_SHADING 0

#define USE_RENDERER 0

#define DIST 0

/* TODO:
* Make submeshes use a texture atlas
* Use a renderer (at least to not redundantly set textures the whole time)
* Make textures in models a unique pointer
*/
/*
* DONE (for showing in HPG)
* MultiMesh
* Frustum Culling
* Window Class
*/
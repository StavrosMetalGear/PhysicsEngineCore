#pragma once

// ╔══════════════════════════════════════════════════════════════════════════════╗
//  PhysicsEngine_export.h — DLL / static-library export macros
//
//  When building as a shared library (DLL), define PHYSICS_ENGINE_EXPORTS.
//  Consumers of the library leave it undefined.
//  For static builds (the default), everything is a no-op.
// ╚══════════════════════════════════════════════════════════════════════════════╝

#ifdef PHYSICS_ENGINE_SHARED
    #ifdef _MSC_VER
        #ifdef PHYSICS_ENGINE_EXPORTS
            #define PHYSENG_API __declspec(dllexport)
        #else
            #define PHYSENG_API __declspec(dllimport)
        #endif
    #else
        #define PHYSENG_API __attribute__((visibility("default")))
    #endif
#else
    // Static library — no decoration needed
    #define PHYSENG_API
#endif

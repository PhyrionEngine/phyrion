#pragma once

#include "Common.hpp"
#include "Fyrion/Core/StringView.hpp"
#include "Fyrion/Core/Math.hpp"
#include "Fyrion/Core/Event.hpp"

namespace Fyrion
{
    using OnInit = EventType<"Fyrion::OnInit"_h, void()>;
    using OnUpdate = EventType<"Fyrion::OnUpdate"_h, void(f64 deltaTime)>;
    using OnShutdown = EventType<"Fyrion::OnShutdown"_h, void()>;
    using OnShutdownRequest = EventType<"Fyrion::OnShutdownRequest"_h, void(bool* canClose)>;

    struct EngineContextCreation
    {
        StringView  title{};
        Extent      resolution{};
        bool        maximize{false};
        bool        fullscreen{false};
        bool        headless = false;
    };


    struct FY_API Engine
    {
        static void Init();
        static void Init(i32 argc, char** argv);
        static void CreateContext(const EngineContextCreation& contextCreation);
        static void Run();
        static void Shutdown();
        static void Destroy();
    };
}

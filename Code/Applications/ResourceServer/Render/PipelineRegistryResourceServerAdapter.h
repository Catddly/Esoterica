#pragma once

#include "Base/Render/RenderPipelineRegistry.h"

namespace EE::Render
{
    class PipelineRegistryResourceServerAdapter final : public PipelineRegistry
    {
    public:

        bool Initialize();
        void Shutdown();

    private:

        virtual void UpdateLoadPipelineShaders() override;

    private:
    };
}
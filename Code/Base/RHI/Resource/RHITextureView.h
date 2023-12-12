#pragma once

#include "RHIResourceCreationCommons.h"

namespace EE::RHI
{
    struct RHITextureView
    {
        inline bool IsValid() const { return m_pViewHandle != nullptr; }

        RHI::RHITextureViewCreateDesc       m_desc = {};
        void*                               m_pViewHandle = nullptr;
    };
}
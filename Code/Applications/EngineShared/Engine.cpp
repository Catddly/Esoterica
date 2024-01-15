#include "Engine.h"
#include "Base/Network/NetworkSystem.h"
#include "Base/Profiling.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/Time/Timers.h"
#include "Base/IniFile.h"
#include "Base/FileSystem/FileSystemUtils.h"
#include "Base/Logging/LoggingSystem.h"

#include "_AutoGenerated/EngineTypeRegistration.h"

//-------------------------------------------------------------------------

namespace EE
{
    Engine::Engine( TFunction<bool( EE::String const& error )>&& errorHandler )
        : m_fatalErrorHandler( errorHandler )
    {}

    //-------------------------------------------------------------------------

    void Engine::RegisterTypes()
    {
        AutoGenerated::Engine::RegisterTypes( *m_pTypeRegistry );
    }

    void Engine::UnregisterTypes()
    {
        AutoGenerated::Engine::UnregisterTypes( *m_pTypeRegistry );
    }

    //-------------------------------------------------------------------------

    bool Engine::Initialize( Int2 const& windowDimensions )
    {
        EE_LOG_MESSAGE( "System", nullptr, "Engine Application Startup" );

        //-------------------------------------------------------------------------
        // Read INI file
        //-------------------------------------------------------------------------

        FileSystem::Path const iniFilePath = FileSystem::GetCurrentProcessPath().Append( "Esoterica.ini" );
        IniFile iniFile( iniFilePath );
        if ( !iniFile.IsValid() )
        {
            InlineString const errorMessage( InlineString::CtorSprintf(), "Failed to load settings from INI file: %s", iniFilePath.c_str() );
            return m_fatalErrorHandler( errorMessage.c_str() );
        }

        //-------------------------------------------------------------------------
        // Initialize Core
        //-------------------------------------------------------------------------

        if ( !m_engineModule.InitializeCoreSystems( iniFile ) )
        {
            return m_fatalErrorHandler( "Failed to initialize engine core systems!" );
        }

        m_pTaskSystem = m_engineModule.GetTaskSystem();
        m_pTypeRegistry = m_engineModule.GetTypeRegistry();
        m_pSystemRegistry = m_engineModule.GetSystemRegistry();
        m_pInputSystem = m_engineModule.GetInputSystem();
        m_pResourceSystem = m_engineModule.GetResourceSystem();
        m_pRenderDevice = m_engineModule.GetRenderDevice();
        m_pEntityWorldManager = m_engineModule.GetEntityWorldManager();

        #if EE_DEVELOPMENT_TOOLS
        m_pImguiSystem = m_engineModule.GetImguiSystem();
        #endif

        m_updateContext.m_pSystemRegistry = m_pSystemRegistry;

        //-------------------------------------------------------------------------
        // Register Types
        //-------------------------------------------------------------------------

        RegisterTypes();

        //-------------------------------------------------------------------------
        // Modules
        //-------------------------------------------------------------------------

        m_moduleInitStageReached = true;

        if ( !m_engineModule.InitializeModule() )
        {
            return m_fatalErrorHandler( "Failed to initialize engine module!" );
        }

        ModuleContext moduleContext;
        moduleContext.m_pTaskSystem = m_pTaskSystem;
        moduleContext.m_pTypeRegistry = m_pTypeRegistry;
        moduleContext.m_pResourceSystem = m_pResourceSystem;
        moduleContext.m_pSystemRegistry = m_pSystemRegistry;
        moduleContext.m_pRenderDevice = m_pRenderDevice;
        moduleContext.m_pEntityWorldManager = m_pEntityWorldManager;

        if ( !m_gameModule.InitializeModule( moduleContext, iniFile ) )
        {
            return m_fatalErrorHandler( "Failed to initialize game module" );
        }

        #if EE_DEVELOPMENT_TOOLS
        if ( !InitializeToolsModulesAndSystems( moduleContext, iniFile ) )
        {
            return false;
        }
        #endif

        //-------------------------------------------------------------------------
        // Load Required Module Resources
        //-------------------------------------------------------------------------

        m_moduleResourcesInitStageReached = true;

        m_engineModule.LoadModuleResources( *m_pResourceSystem );
        m_gameModule.LoadModuleResources( *m_pResourceSystem );

        // Wait for all load requests to complete
        while ( m_pResourceSystem->IsBusy() )
        {
            Network::NetworkSystem::Update();
            m_pResourceSystem->Update();
        }

        // Verify that all module resources have been correctly loaded
        if ( !m_engineModule.VerifyModuleResourceLoadingComplete() || !m_gameModule.VerifyModuleResourceLoadingComplete() )
        {
            return m_fatalErrorHandler( "Failed to load required engine resources - See EngineApplication log for details" );
        }

        //-------------------------------------------------------------------------
        // Final initialization
        //-------------------------------------------------------------------------

        m_finalInitStageReached = true;

        // Initialize entity world manager and load startup map
        m_pEntityWorldManager->Initialize( *m_pSystemRegistry );
        if ( m_startupMap.IsValid() )
        {
            auto const mapResourceID = EE::ResourceID( m_startupMap );
            m_pEntityWorldManager->GetWorlds()[0]->LoadMap( mapResourceID );
        }

        // Initialize rendering system
        m_renderPipelineRegistry.Initialize( m_pResourceSystem );
        m_renderingSystem.Initialize( m_pRenderDevice, &m_renderPipelineRegistry, Float2( windowDimensions ), m_engineModule.GetRendererRegistry(), m_pEntityWorldManager );
        m_pSystemRegistry->RegisterSystem( &m_renderingSystem );

        // Create tools UI
        #if EE_DEVELOPMENT_TOOLS
        CreateToolsUI();
        EE_ASSERT( m_pToolsUI != nullptr );
        m_pToolsUI->Initialize( m_updateContext, m_pImguiSystem->GetImageCache() );
        #endif

        m_initialized = true;
        return true;
    }

    bool Engine::Shutdown()
    {
        EE_LOG_MESSAGE( "System", nullptr, "Engine Application Shutdown Started" );

        if ( m_pTaskSystem != nullptr )
        {
            m_pTaskSystem->WaitForAll();
        }

        //-------------------------------------------------------------------------
        // Shutdown World and runtime state
        //-------------------------------------------------------------------------

        if ( m_finalInitStageReached )
        {
            // Destroy development tools
            #if EE_DEVELOPMENT_TOOLS
            EE_ASSERT( m_pToolsUI != nullptr );
            m_pToolsUI->Shutdown( m_updateContext );
            DestroyToolsUI();
            EE_ASSERT( m_pToolsUI == nullptr );
            #endif

            m_pRenderDevice->GetRHIDevice()->WaitUntilIdle();

            m_renderPipelineRegistry.DestroyAllPipelineStates( m_pRenderDevice->GetRHIDevice() );
            m_renderPipelineRegistry.Shutdown();

            // Shutdown rendering system
            m_pSystemRegistry->UnregisterSystem( &m_renderingSystem );
            m_renderingSystem.Shutdown();

            // Wait for resource/object systems to complete all resource unloading
            m_pEntityWorldManager->Shutdown();

            while ( m_pEntityWorldManager->IsBusyLoading() || m_pResourceSystem->IsBusy() )
            {
                m_pResourceSystem->Update();
            }

            m_finalInitStageReached = false;
        }

        //-------------------------------------------------------------------------
        // Unload engine resources
        //-------------------------------------------------------------------------

        if( m_moduleResourcesInitStageReached )
        {
            if ( m_pResourceSystem != nullptr )
            {
                m_gameModule.UnloadModuleResources( *m_pResourceSystem );
                m_engineModule.UnloadModuleResources( *m_pResourceSystem );

                while ( m_pResourceSystem->IsBusy() )
                {
                    m_pResourceSystem->Update();
                }
            }

            m_moduleResourcesInitStageReached = false;
        }

        //-------------------------------------------------------------------------
        // Shutdown modules
        //-------------------------------------------------------------------------

        if ( m_moduleInitStageReached )
        {
            ModuleContext moduleContext;
            moduleContext.m_pTaskSystem = m_pTaskSystem;
            moduleContext.m_pTypeRegistry = m_pTypeRegistry;
            moduleContext.m_pResourceSystem = m_pResourceSystem;
            moduleContext.m_pSystemRegistry = m_pSystemRegistry;
            moduleContext.m_pRenderDevice = m_pRenderDevice;
            moduleContext.m_pEntityWorldManager = m_pEntityWorldManager;

            #if EE_DEVELOPMENT_TOOLS
            ShutdownToolsModulesAndSystems( moduleContext );
            #endif

            m_gameModule.ShutdownModule( moduleContext );
            m_engineModule.ShutdownModule();
        }

        //-------------------------------------------------------------------------
        // Unregister types
        //-------------------------------------------------------------------------

        if ( m_moduleInitStageReached )
        {
            UnregisterTypes();
            m_moduleInitStageReached = false;
        }

        //-------------------------------------------------------------------------
        // Shutdown Core
        //-------------------------------------------------------------------------

        m_updateContext.m_pSystemRegistry = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        m_pImguiSystem = nullptr;
        #endif

        m_pTaskSystem = nullptr;
        m_pTypeRegistry = nullptr;
        m_pSystemRegistry = nullptr;
        m_pInputSystem = nullptr;
        m_pResourceSystem = nullptr;
        m_pRenderDevice = nullptr;
        m_pEntityWorldManager = nullptr;

        m_engineModule.ShutdownCoreSystems();

        //-------------------------------------------------------------------------

        m_initialized = false;
        return true;
    }

    //-------------------------------------------------------------------------

    bool Engine::Update()
    {
        EE_ASSERT( m_initialized );

        // Check for fatal errors
        //-------------------------------------------------------------------------

        if ( Log::System::HasFatalErrorOccurred() )
        {
            return m_fatalErrorHandler( Log::System::GetFatalError().m_message );
        }

        // Perform Frame Update
        //-------------------------------------------------------------------------

        Profiling::StartFrame();

        Milliseconds deltaTime = 0;
        {
            ScopedTimer<PlatformClock> frameTimer( deltaTime );

            // Frame Start
            //-------------------------------------------------------------------------
            {
                EE_PROFILE_SCOPE_ENTITY( "Frame Start" );
                m_updateContext.m_stage = UpdateStage::FrameStart;

                //-------------------------------------------------------------------------

                {
                    EE_PROFILE_SCOPE_NETWORK( "Networking" );
                    Network::NetworkSystem::Update();
                }

                //-------------------------------------------------------------------------

                // Note: To ensure UI system will NOT fetch the stale textures,
                //       we must resize render target before UI drawing (i.e. before m_pToolsUI->StartFrame() )
                m_renderingSystem.ResizeWorldRenderTargets();

                m_pEntityWorldManager->StartFrame();

                //-------------------------------------------------------------------------

                #if EE_DEVELOPMENT_TOOLS
                m_pImguiSystem->StartFrame( m_updateContext.GetDeltaTime() );
                m_pToolsUI->StartFrame( m_updateContext );
                #endif

                //-------------------------------------------------------------------------
                
                m_renderPipelineRegistry.Update();

                //-------------------------------------------------------------------------

                {
                    EE_PROFILE_SCOPE_RESOURCE( "Resource System" );

                    m_pResourceSystem->Update();

                    // Handle hot-reloading of entities
                    #if EE_DEVELOPMENT_TOOLS
                    if ( m_pResourceSystem->RequiresHotReloading() )
                    {
                        m_pToolsUI->BeginHotReload( m_pResourceSystem->GetUsersToBeReloaded(), m_pResourceSystem->GetResourcesToBeReloaded() );
                        m_pEntityWorldManager->BeginHotReload( m_pResourceSystem->GetUsersToBeReloaded() );
                        m_pResourceSystem->ClearHotReloadRequests();

                        // Ensure that all resource requests (both load/unload are completed before continuing with the hot-reload)
                        while ( m_pResourceSystem->IsBusy() )
                        {
                            Network::NetworkSystem::Update();
                            m_pResourceSystem->Update( true );
                        }

                        m_pEntityWorldManager->EndHotReload();
                        m_pToolsUI->EndHotReload();
                    }
                    #endif
                }

                //-------------------------------------------------------------------------

                {
                    EE_PROFILE_SCOPE_ENTITY( "World Loading" );
                    m_pEntityWorldManager->UpdateLoading();
                }

                //-------------------------------------------------------------------------

                {
                    EE_PROFILE_SCOPE_DEVTOOLS( "Input System" );
                    m_pInputSystem->Update( m_updateContext.GetDeltaTime() );
                }

                #if EE_DEVELOPMENT_TOOLS
                m_pToolsUI->Update( m_updateContext );
                #endif

                m_pEntityWorldManager->UpdateWorlds( m_updateContext );
            }

            // Pre-Physics
            //-------------------------------------------------------------------------
            {
                EE_PROFILE_SCOPE_ENTITY( "Pre-Physics Update" );
                m_updateContext.m_stage = UpdateStage::PrePhysics;

                #if EE_DEVELOPMENT_TOOLS
                m_pToolsUI->Update( m_updateContext );
                #endif

                m_pEntityWorldManager->UpdateWorlds( m_updateContext );
            }

            // Physics
            //-------------------------------------------------------------------------
            {
                EE_PROFILE_SCOPE_ENTITY( "Physics Update" );
                m_updateContext.m_stage = UpdateStage::Physics;

                #if EE_DEVELOPMENT_TOOLS
                m_pToolsUI->Update( m_updateContext );
                #endif

                m_pEntityWorldManager->UpdateWorlds( m_updateContext );
            }

            // Post-Physics
            //-------------------------------------------------------------------------
            {
                EE_PROFILE_SCOPE_ENTITY( "Post-Physics Update" );
                m_updateContext.m_stage = UpdateStage::PostPhysics;

                #if EE_DEVELOPMENT_TOOLS
                m_pToolsUI->Update( m_updateContext );
                #endif

                m_pEntityWorldManager->UpdateWorlds( m_updateContext );
            }

            // Pause Updates
            //-------------------------------------------------------------------------
            // This is an optional update that's only run when a world is "paused"
            {
                EE_PROFILE_SCOPE_ENTITY( "Paused Update" );
                m_updateContext.m_stage = UpdateStage::Paused;

                #if EE_DEVELOPMENT_TOOLS
                m_pToolsUI->Update( m_updateContext );
                #endif

                m_pEntityWorldManager->UpdateWorlds( m_updateContext );
            }

            // Frame End
            //-------------------------------------------------------------------------
            {
                EE_PROFILE_SCOPE_ENTITY( "Frame End" );
                m_updateContext.m_stage = UpdateStage::FrameEnd;

                #if EE_DEVELOPMENT_TOOLS
                m_pToolsUI->Update( m_updateContext );
                #endif

                m_pEntityWorldManager->UpdateWorlds( m_updateContext );

                //-------------------------------------------------------------------------

                #if EE_DEVELOPMENT_TOOLS
                m_pToolsUI->EndFrame( m_updateContext );
                m_pImguiSystem->EndFrame();
                #endif

                m_pEntityWorldManager->EndFrame();

                if ( !m_exitRequested )
                {
                    m_renderingSystem.Update( m_updateContext );
                }

                m_pInputSystem->ClearFrameState();
            }
        }

        // Update Time
        //-------------------------------------------------------------------------

        // Ensure we dont get crazy time delta's when we hit breakpoints
        #if EE_DEVELOPMENT_TOOLS
        if ( deltaTime.ToSeconds() > 1.0f )
        {
            deltaTime = m_updateContext.GetDeltaTime(); // Keep last frame delta
        }
        #endif

        // Frame rate limiter
        if ( m_updateContext.HasFrameRateLimit() )
        {
            float const minimumFrameTime = m_updateContext.GetLimitedFrameTime();
            if ( deltaTime < minimumFrameTime )
            {
                Threading::Sleep( minimumFrameTime - deltaTime );
                deltaTime = minimumFrameTime;
            }
        }

        m_updateContext.UpdateDeltaTime( deltaTime );
        EngineClock::Update( deltaTime );
        Profiling::EndFrame();

        // Should we exit?
        //-------------------------------------------------------------------------

        return true;
    }
}
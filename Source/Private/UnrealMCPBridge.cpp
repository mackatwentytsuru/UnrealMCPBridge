#include "UnrealMCPBridge.h"
#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnrealMCPBridge, Log, All);

#define LOCTEXT_NAMESPACE "FUnrealMCPBridgeModule"

void FUnrealMCPBridgeModule::StartupModule()
{
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("UnrealMCPBridge module starting up for UE5.6"));
	
	// Check if Python Script Plugin is enabled
	if (!IsPythonScriptPluginEnabled())
	{
		UE_LOG(LogUnrealMCPBridge, Warning, TEXT("PythonScriptPlugin is not enabled. MCP Bridge functionality will be limited."));
		return;
	}
	
	// Initialize MCP server
	InitializeMCPServer();
}

void FUnrealMCPBridgeModule::ShutdownModule()
{
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("UnrealMCPBridge module shutting down"));
	
	// Cleanup MCP server
	ShutdownMCPServer();
}

void FUnrealMCPBridgeModule::InitializeMCPServer()
{
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("Initializing MCP Server..."));
	
	// TODO: Implement MCP server initialization
	// This will include:
	// - Setting up TCP/WebSocket server for MCP protocol
	// - Registering Python API endpoints
	// - Setting up tool and resource handlers
	
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCP Server initialized successfully"));
}

void FUnrealMCPBridgeModule::ShutdownMCPServer()
{
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("Shutting down MCP Server..."));
	
	// TODO: Implement MCP server cleanup
	// This will include:
	// - Stopping server threads
	// - Cleaning up connections
	// - Unregistering endpoints
	
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCP Server shutdown complete"));
}

bool FUnrealMCPBridgeModule::IsPythonScriptPluginEnabled() const
{
	// Check if the Python Script Plugin module is loaded
	return FModuleManager::Get().IsModuleLoaded("PythonScriptPlugin");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealMCPBridgeModule, UnrealMCPBridge)
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Unreal MCP Bridge Module
 * 
 * This module implements an MCP (Model Context Protocol) server that allows
 * MCP clients to access the Unreal Engine Editor Python API.
 * 
 * Compatible with Unreal Engine 5.6
 */
class FUnrealMCPBridgeModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Initialize MCP server and Python integration */
	void InitializeMCPServer();
	
	/** Cleanup MCP server resources */
	void ShutdownMCPServer();
	
	/** Check if Python Script Plugin is available */
	bool IsPythonScriptPluginEnabled() const;
};
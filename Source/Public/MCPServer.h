#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Containers/Queue.h"
#include "Dom/JsonObject.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMCPServer, Log, All);

/**
 * MCP Protocol Message Types
 */
UENUM(BlueprintType)
enum class EMCPMessageType : uint8
{
    Request,
    Response,
    Notification
};

/**
 * MCP Server Implementation
 * 
 * Implements the Model Context Protocol server functionality
 * Compatible with UE5.6 networking improvements
 */
class UNREALMCPBRIDGE_API FMCPServer : public FRunnable
{
public:
    FMCPServer();
    virtual ~FMCPServer();

    /** Start the MCP server */
    bool StartServer(int32 Port = 3001);
    
    /** Stop the MCP server */
    void StopServer();
    
    /** Check if server is running */
    bool IsRunning() const { return bIsRunning; }
    
    /** Get server port */
    int32 GetPort() const { return ServerPort; }

    // FRunnable interface
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

protected:
    /** Handle incoming MCP message */
    void HandleMCPMessage(const FString& Message, int32 ClientId);
    
    /** Send response to client */
    void SendResponse(int32 ClientId, const TSharedPtr<FJsonObject>& Response);
    
    /** Process MCP tool call */
    TSharedPtr<FJsonObject> ProcessToolCall(const TSharedPtr<FJsonObject>& Request);
    
    /** Process MCP resource request */
    TSharedPtr<FJsonObject> ProcessResourceRequest(const TSharedPtr<FJsonObject>& Request);
    
    /** Initialize tool registry */
    void InitializeTools();
    
    /** Register Python API tools */
    void RegisterPythonTools();

private:
    /** Server thread */
    FRunnableThread* ServerThread;
    
    /** Server running state */
    std::atomic<bool> bIsRunning;
    std::atomic<bool> bStopRequested;
    
    /** Server port */
    int32 ServerPort;
    
    /** Message queue for thread-safe communication */
    TQueue<FString> MessageQueue;
    
    /** Connected clients */
    TMap<int32, FString> ConnectedClients;
    
    /** Tool registry */
    TMap<FString, TFunction<TSharedPtr<FJsonObject>(const TSharedPtr<FJsonObject>&)>> ToolRegistry;
    
    /** Resource registry */
    TMap<FString, TFunction<TSharedPtr<FJsonObject>(const TSharedPtr<FJsonObject>&)>> ResourceRegistry;
};
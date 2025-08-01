#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "Sockets.h"
#include "Dom/JsonObject.h"
#include "Tasks/Task.h"
#include "Containers/Queue.h"
#include "EditorSubsystem.h"
#include "MCPServerUE56.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMCPServerUE56, Log, All);

/**
 * MCP プロトコルメッセージタイプ
 */
UENUM(BlueprintType)
enum class EMCPMessageType : uint8
{
	Request,
	Response,
	Notification
};

/**
 * UE5.6対応 MCP サーバー実装
 * 
 * TasksシステムとInsightsを活用した最適化されたMCPサーバー
 * 詳細改造計画書に基づく本格実装
 */
UCLASS(BlueprintType, Blueprintable)
class UNREALMCPBRIDGE_API UMCPServerUE56 : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UMCPServerUE56();
	virtual ~UMCPServerUE56();

	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** MCPサーバーを開始 */
	UFUNCTION(BlueprintCallable, Category = "MCP Server")
	bool StartServer(int32 Port = 3001);
	
	/** MCPサーバーを停止 */
	UFUNCTION(BlueprintCallable, Category = "MCP Server")
	void StopServer();
	
	/** サーバーが実行中かチェック */
	UFUNCTION(BlueprintCallable, Category = "MCP Server")
	bool IsServerRunning() const { return bIsServerRunning.load(); }
	
	/** 現在のポート番号を取得 */
	UFUNCTION(BlueprintCallable, Category = "MCP Server")
	int32 GetServerPort() const { return ServerPort; }

	/** MCPコマンドを直接実行（テスト用） */
	UFUNCTION(BlueprintCallable, Category = "MCP Server")
	FString ExecuteCommand(const FString& Command);

	/** サーバー統計情報を取得 */
	UFUNCTION(BlueprintCallable, Category = "MCP Server")
	FString GetServerStats() const;

protected:
	/** 受信MCPメッセージを処理 */
	void HandleMCPMessage(const FString& MessageData, int32 ClientId);
	
	/** クライアントにレスポンスを送信 */
	void SendResponse(int32 ClientId, const TSharedPtr<FJsonObject>& Response);
	
	/** MCPツール呼び出しを処理 */
	TSharedPtr<FJsonObject> ProcessToolCall(const TSharedPtr<FJsonObject>& Request);
	
	/** MCPリソースリクエストを処理 */
	TSharedPtr<FJsonObject> ProcessResourceRequest(const TSharedPtr<FJsonObject>& Request);
	
	/** Python APIコマンドを実行（UE5.6最適化版） */
	FString ExecutePythonCommand(const FString& Command);
	
	/** ツールレジストリを初期化 */
	void InitializeToolRegistry();
	
	/** Pythonツールを登録 */
	void RegisterPythonTools();

private:
	/** UE5.6 Tasks システムを使用したサーバーループ */
	void ServerLoop();
	
	/** クライアント接続を処理 */
	void ProcessClient(TSharedPtr<FSocket> ClientSocket);
	
	/** Insightsプロファイリングイベントを記録 */
	void LogMCPEvent(const FString& EventName, const FString& Data) const;

	/** サーバーソケット */
	TSharedPtr<FSocket> ListenSocket;
	
	/** サーバータスク（UE5.6 Tasks システム） */
	UE::Tasks::FTask ServerTask;
	
	/** サーバー実行状態 */
	std::atomic<bool> bIsServerRunning;
	std::atomic<bool> bStopRequested;
	
	/** サーバーポート */
	int32 ServerPort;
	
	/** 接続中のクライアント */
	TMap<int32, TSharedPtr<FSocket>> ConnectedClients;
	mutable FCriticalSection ClientsLock;
	
	/** ツールレジストリ */
	TMap<FString, TFunction<TSharedPtr<FJsonObject>(const TSharedPtr<FJsonObject>&)>> ToolRegistry;
	
	/** リソースレジストリ */
	TMap<FString, TFunction<TSharedPtr<FJsonObject>(const TSharedPtr<FJsonObject>&)>> ResourceRegistry;

	/** パフォーマンス統計（UE5.6最適化） */
	std::atomic<int64> ProcessedMessageCount;
	std::atomic<int64> ActiveClientCount;
	double ServerStartTime;
};
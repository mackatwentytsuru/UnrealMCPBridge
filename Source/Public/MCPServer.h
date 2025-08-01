#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EditorSubsystem.h"
#include "Tasks/Task.h"
#include "Sockets.h"
#include "Common/TcpSocketBuilder.h"
#include "Json.h"
#include "MCPServer.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMCPServer, Log, All);

// UE5.6のTraceシステム対応
#if ENABLE_MCP_INSIGHTS
DECLARE_TRACE_CHANNEL_EXTERN(MCPChannel);
#endif

/**
 * MCP Message構造体
 * メッセージプールで再利用される
 */
USTRUCT()
struct UNREALMCPBRIDGE_API FMCPMessage
{
	GENERATED_BODY()

	UPROPERTY()
	FString Command;

	UPROPERTY()
	FString Data;

	UPROPERTY()
	double Timestamp;

	FMCPMessage()
		: Timestamp(0.0)
	{
	}

	void Reset()
	{
		Command.Empty();
		Data.Empty();
		Timestamp = 0.0;
	}
};

/**
 * UE5.6対応のMCPサーバー実装
 * 
 * 新しいTasksシステムを使用した高性能な非同期通信を提供します。
 * Live Coding、Insights統合、メモリ最適化などUE5.6の新機能を活用。
 */
UCLASS()
class UNREALMCPBRIDGE_API UMCPServer : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UMCPServer();

	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** MCPサーバーを開始 */
	UFUNCTION(BlueprintCallable, Category = "MCP Bridge")
	bool StartServer(int32 Port = 9000);

	/** MCPサーバーを停止 */
	UFUNCTION(BlueprintCallable, Category = "MCP Bridge")
	void StopServer();

	/** サーバーが実行中かチェック */
	UFUNCTION(BlueprintPure, Category = "MCP Bridge")
	bool IsRunning() const { return bIsRunning.load(); }

	/** MCPコマンドを実行（内部使用） */
	FString ExecuteCommand(const FString& JsonCommand);

protected:
	/** サーバーループメイン処理 */
	void ServerLoop();

	/** クライアント接続処理 */
	void ProcessClient(FSocket* ClientSocket);

	/** Python統合でコマンド実行 */
	FString ExecutePythonCommand(const FString& Command);

	/** Live Coding対応チェック */
	bool IsLiveCodingActive() const;

private:
	// UE5.6 Tasks システム
	TSharedPtr<UE::Tasks::FTask> ServerTask;
	
	// ソケット通信
	TSharedPtr<FSocket> ListenSocket;
	
	// 実行状態管理
	std::atomic<bool> bIsRunning;
	std::atomic<bool> bStopRequested;
	
	// サーバー設定
	int32 ServerPort;
	
	// メッセージプール（メモリ最適化）
	TArray<TSharedPtr<FMCPMessage>> MessagePool;
	FCriticalSection PoolCriticalSection;
	
	// パフォーマンス統計
	int32 ProcessedMessageCount;
	double LastPerformanceReport;

	/** メモリ最適化されたメッセージ取得 */
	TSharedPtr<FMCPMessage> AcquireMessage();

	/** メッセージをプールに返却 */
	void ReleaseMessage(TSharedPtr<FMCPMessage> Message);

	/** パフォーマンス統計をレポート */
	void ReportPerformanceStats();
};
#include "UnrealMCPBridge.h"
#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnrealMCPBridge, Log, All);

#define LOCTEXT_NAMESPACE "FUnrealMCPBridgeModule"

void FUnrealMCPBridgeModule::StartupModule()
{
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("UnrealMCPBridge モジュールを UE5.6 で起動中"));
	
	// Python Script Plugin が有効かチェック
	if (!IsPythonScriptPluginEnabled())
	{
		UE_LOG(LogUnrealMCPBridge, Warning, TEXT("PythonScriptPlugin が有効ではありません。MCP Bridge 機能が制限されます。"));
		return;
	}
	
	// MCPサーバーを初期化
	InitializeMCPServer();
}

void FUnrealMCPBridgeModule::ShutdownModule()
{
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("UnrealMCPBridge モジュールをシャットダウン中"));
	
	// MCPサーバーをクリーンアップ
	ShutdownMCPServer();
}

void FUnrealMCPBridgeModule::InitializeMCPServer()
{
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCPサーバーを初期化中..."));
	
	// TODO: MCPサーバー初期化を実装
	// これには以下が含まれます:
	// - MCPプロトコル用のTCP/WebSocketサーバーの設定
	// - Python APIエンドポイントの登録
	// - ツールとリソースハンドラーの設定
	
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCPサーバーの初期化が完了しました"));
}

void FUnrealMCPBridgeModule::ShutdownMCPServer()
{
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCPサーバーをシャットダウン中..."));
	
	// TODO: MCPサーバークリーンアップを実装
	// これには以下が含まれます:
	// - サーバースレッドの停止
	// - 接続のクリーンアップ
	// - エンドポイントの登録解除
	
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCPサーバーのシャットダウンが完了しました"));
}

bool FUnrealMCPBridgeModule::IsPythonScriptPluginEnabled() const
{
	// Python Script Plugin モジュールがロードされているかチェック
	return FModuleManager::Get().IsModuleLoaded("PythonScriptPlugin");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealMCPBridgeModule, UnrealMCPBridge)
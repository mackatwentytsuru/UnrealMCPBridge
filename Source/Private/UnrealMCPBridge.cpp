#include "UnrealMCPBridge.h"
#include "MCPServer.h"
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
	
	// UE5.6のEditor Subsystemを使用してMCPサーバーを取得・開始
	if (GEditor)
	{
		UMCPServer* MCPServer = GEditor->GetEditorSubsystem<UMCPServer>();
		if (MCPServer)
		{
			// サーバーを自動開始（デフォルトポート9000）
			if (MCPServer->StartServer())
			{
				UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCPサーバーが正常に開始されました"));
				
				// エディター通知を表示
				ShowEditorNotification(
					LOCTEXT("MCPServerStarted", "MCP Bridgeサーバーが開始されました"),
					SNotificationItem::CS_Success
				);
			}
			else
			{
				UE_LOG(LogUnrealMCPBridge, Warning, TEXT("MCPサーバーの開始に失敗しました"));
				
				ShowEditorNotification(
					LOCTEXT("MCPServerStartFailed", "MCP Bridgeサーバーの開始に失敗しました"),
					SNotificationItem::CS_Fail
				);
			}
		}
		else
		{
			UE_LOG(LogUnrealMCPBridge, Error, TEXT("MCPServerサブシステムが見つかりません"));
		}
	}
	else
	{
		UE_LOG(LogUnrealMCPBridge, Warning, TEXT("エディター環境ではありません。MCPサーバーはスキップされます。"));
	}
	
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCPサーバーの初期化が完了しました"));
}

void FUnrealMCPBridgeModule::ShutdownMCPServer()
{
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCPサーバーをシャットダウン中..."));
	
	// UE5.6のEditor Subsystemを使用してMCPサーバーを停止
	if (GEditor)
	{
		UMCPServer* MCPServer = GEditor->GetEditorSubsystem<UMCPServer>();
		if (MCPServer && MCPServer->IsRunning())
		{
			MCPServer->StopServer();
			UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCPサーバーが正常に停止されました"));
		}
	}
	
	UE_LOG(LogUnrealMCPBridge, Log, TEXT("MCPサーバーのシャットダウンが完了しました"));
}

bool FUnrealMCPBridgeModule::IsPythonScriptPluginEnabled() const
{
	// Python Script Plugin モジュールがロードされているかチェック
	return FModuleManager::Get().IsModuleLoaded("PythonScriptPlugin");
}

void FUnrealMCPBridgeModule::ShowEditorNotification(const FText& Message, SNotificationItem::ECompletionState CompletionState)
{
	if (!GEditor)
	{
		return;
	}

	// UE5.6対応の通知システム
	FNotificationInfo NotificationInfo(Message);
	NotificationInfo.bFireAndForget = true;
	NotificationInfo.FadeOutDuration = 3.0f;
	NotificationInfo.ExpireDuration = 5.0f;
	NotificationInfo.bUseThrobber = false;
	NotificationInfo.bUseSuccessFailIcons = true;

	TSharedPtr<SNotificationItem> NotificationItem = FSlateNotificationManager::Get().AddNotification(NotificationInfo);
	if (NotificationItem.IsValid())
	{
		NotificationItem->SetCompletionState(CompletionState);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealMCPBridgeModule, UnrealMCPBridge)
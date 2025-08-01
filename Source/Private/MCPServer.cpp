#include "MCPServer.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFilemanager.h"
#include "IPythonScriptPlugin.h"
#include "HAL/Event.h"
#include "Async/TaskGraphInterfaces.h"
#include "JsonObjectConverter.h"

DEFINE_LOG_CATEGORY(LogMCPServer);

// UE5.6のTraceシステム
#if ENABLE_MCP_INSIGHTS
DEFINE_TRACE_CHANNEL(MCPChannel);
#endif

UMCPServer::UMCPServer()
	: bIsRunning(false)
	, bStopRequested(false)
	, ServerPort(9000)
	, ProcessedMessageCount(0)
	, LastPerformanceReport(0.0)
{
}

void UMCPServer::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogMCPServer, Log, TEXT("MCPサーバーサブシステムを初期化中..."));
	
	// メッセージプールの初期化（100個のメッセージをプール）
	{
		FScopeLock Lock(&PoolCriticalSection);
		MessagePool.Reserve(100);
		for (int32 i = 0; i < 100; i++)
		{
			MessagePool.Add(MakeShared<FMCPMessage>());
		}
	}
	
	// パフォーマンス統計の初期化
	LastPerformanceReport = FPlatformTime::Seconds();
	
	UE_LOG(LogMCPServer, Log, TEXT("MCPサーバーサブシステムの初期化が完了"));
}

void UMCPServer::Deinitialize()
{
	UE_LOG(LogMCPServer, Log, TEXT("MCPサーバーサブシステムを終了中..."));
	
	StopServer();
	
	// メッセージプールのクリーンアップ
	{
		FScopeLock Lock(&PoolCriticalSection);
		MessagePool.Empty();
	}
	
	Super::Deinitialize();
	
	UE_LOG(LogMCPServer, Log, TEXT("MCPサーバーサブシステムの終了が完了"));
}

bool UMCPServer::StartServer(int32 Port)
{
#if ENABLE_MCP_INSIGHTS
	TRACE_CPUPROFILER_EVENT_SCOPE(MCP_StartServer);
#endif

	if (bIsRunning.load())
	{
		UE_LOG(LogMCPServer, Warning, TEXT("MCPサーバーは既にポート %d で実行中です"), ServerPort);
		return false;
	}

	ServerPort = Port;
	bStopRequested.store(false);

	// ソケットの作成
	FIPv4Endpoint Endpoint(FIPv4Address(127, 0, 0, 1), ServerPort);
	ListenSocket = MakeShared<FSocket>(
		FTcpSocketBuilder(TEXT("MCPServer"))
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.Listening(8)
	);

	if (!ListenSocket.IsValid())
	{
		UE_LOG(LogMCPServer, Error, TEXT("ポート %d でソケットの作成に失敗しました"), ServerPort);
		return false;
	}

	// UE5.6のTasksシステムでサーバーループを開始
	ServerTask = MakeShared<UE::Tasks::FTask>(
		UE::Tasks::Launch(TEXT("MCPServerLoop"), 
		[this]() { ServerLoop(); },
		UE::Tasks::ETaskPriority::BackgroundNormal)
	);

	UE_LOG(LogMCPServer, Log, TEXT("MCPサーバーがポート %d で開始されました"), ServerPort);
	return true;
}

void UMCPServer::StopServer()
{
#if ENABLE_MCP_INSIGHTS
	TRACE_CPUPROFILER_EVENT_SCOPE(MCP_StopServer);
#endif

	if (!bIsRunning.load())
	{
		return;
	}

	UE_LOG(LogMCPServer, Log, TEXT("MCPサーバーを停止中..."));
	
	bStopRequested.store(true);
	
	// ソケットを閉じる
	if (ListenSocket.IsValid())
	{
		ListenSocket->Close();
		ListenSocket.Reset();
	}
	
	// タスクの完了を待つ
	if (ServerTask.IsValid())
	{
		ServerTask->Wait();
		ServerTask.Reset();
	}
	
	bIsRunning.store(false);
	
	// 最終パフォーマンス統計をレポート
	ReportPerformanceStats();
	
	UE_LOG(LogMCPServer, Log, TEXT("MCPサーバーが停止されました"));
}

void UMCPServer::ServerLoop()
{
#if ENABLE_MCP_INSIGHTS
	TRACE_CPUPROFILER_EVENT_SCOPE(MCP_ServerLoop);
#endif

	UE_LOG(LogMCPServer, Log, TEXT("MCPサーバーループを開始"));
	bIsRunning.store(true);

	while (!bStopRequested.load() && ListenSocket.IsValid())
	{
		// クライアント接続を待機
		FSocket* ClientSocket = ListenSocket->Accept(TEXT("MCPClient"));
		if (ClientSocket)
		{
			UE_LOG(LogMCPServer, Log, TEXT("新しいクライアント接続を受信"));
			
			// クライアント処理を別タスクで実行
			UE::Tasks::Launch(TEXT("MCPClientHandler"),
				[this, ClientSocket]() { ProcessClient(ClientSocket); },
				UE::Tasks::ETaskPriority::Normal
			);
		}
		
		// CPUを他のタスクに譲る
		FPlatformProcess::Sleep(0.01f);
		
		// 定期的にパフォーマンス統計をレポート
		double CurrentTime = FPlatformTime::Seconds();
		if (CurrentTime - LastPerformanceReport > 60.0) // 1分間隔
		{
			ReportPerformanceStats();
			LastPerformanceReport = CurrentTime;
		}
	}

	bIsRunning.store(false);
	UE_LOG(LogMCPServer, Log, TEXT("MCPサーバーループを終了"));
}

void UMCPServer::ProcessClient(FSocket* ClientSocket)
{
#if ENABLE_MCP_INSIGHTS
	TRACE_CPUPROFILER_EVENT_SCOPE(MCP_ProcessClient);
#endif

	if (!ClientSocket)
	{
		return;
	}

	// スマートポインタでソケットを管理
	TUniquePtr<FSocket> ClientSocketPtr(ClientSocket);
	
	uint8 Buffer[4096];
	FString ReceivedData;

	while (!bStopRequested.load() && ClientSocketPtr->GetConnectionState() == SCS_Connected)
	{
		int32 BytesRead = 0;
		if (ClientSocketPtr->Recv(Buffer, sizeof(Buffer) - 1, BytesRead))
		{
			if (BytesRead > 0)
			{
				Buffer[BytesRead] = 0;
				ReceivedData += UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buffer));

				// メッセージの完了をチェック（改行で区切られていると仮定）
				int32 NewlineIndex;
				while (ReceivedData.FindChar(TEXT('\n'), NewlineIndex))
				{
					FString Message = ReceivedData.Left(NewlineIndex);
					ReceivedData = ReceivedData.Mid(NewlineIndex + 1);

					// メッセージを処理
					FString Response = ExecuteCommand(Message);
					
					// レスポンスを送信
					FTCHARToUTF8 UTF8Response(*Response);
					ClientSocketPtr->Send(reinterpret_cast<const uint8*>(UTF8Response.Get()), UTF8Response.Length());
					ClientSocketPtr->Send(reinterpret_cast<const uint8*>("\n"), 1);
					
					ProcessedMessageCount++;
				}
			}
		}
		else
		{
			// エラーまたは切断
			break;
		}
		
		// CPUを他のタスクに譲る
		FPlatformProcess::Sleep(0.001f);
	}

	UE_LOG(LogMCPServer, Log, TEXT("クライアント接続が終了しました"));
}

FString UMCPServer::ExecuteCommand(const FString& JsonCommand)
{
#if ENABLE_MCP_INSIGHTS
	TRACE_CPUPROFILER_EVENT_SCOPE(MCP_ExecuteCommand);
#endif

	// メッセージプールからメッセージを取得
	TSharedPtr<FMCPMessage> Message = AcquireMessage();
	
	// JSONをパース
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonCommand);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		ReleaseMessage(Message);
		return TEXT("{\"error\": \"Invalid JSON format\"}");
	}

	// コマンドを取得
	FString Command;
	if (!JsonObject->TryGetStringField(TEXT("command"), Command))
	{
		ReleaseMessage(Message);
		return TEXT("{\"error\": \"Missing command field\"}");
	}

	// Live Coding チェック
	if (IsLiveCodingActive())
	{
		ReleaseMessage(Message);
		return TEXT("{\"status\": \"deferred\", \"message\": \"Live Coding実行中のため処理を延期\"}");
	}

	FString Result;
	
	// コマンドに応じて処理を分岐
	if (Command == TEXT("execute_python"))
	{
		FString PythonCode;
		if (JsonObject->TryGetStringField(TEXT("code"), PythonCode))
		{
			Result = ExecutePythonCommand(PythonCode);
		}
		else
		{
			Result = TEXT("{\"error\": \"Missing Python code\"}");
		}
	}
	else if (Command == TEXT("get_project_info"))
	{
		// プロジェクト情報を取得
		TSharedPtr<FJsonObject> ProjectInfo = MakeShareable(new FJsonObject);
		ProjectInfo->SetStringField(TEXT("project_name"), FApp::GetProjectName());
		ProjectInfo->SetStringField(TEXT("engine_version"), FEngineVersion::Current().ToString());
		ProjectInfo->SetStringField(TEXT("project_dir"), FPaths::ProjectDir());
		
		FString ProjectInfoJson;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ProjectInfoJson);
		FJsonSerializer::Serialize(ProjectInfo.ToSharedRef(), Writer);
		Result = ProjectInfoJson;
	}
	else
	{
		Result = FString::Printf(TEXT("{\"error\": \"Unknown command: %s\"}"), *Command);
	}

	// メッセージをプールに返却
	ReleaseMessage(Message);
	
	return Result;
}

FString UMCPServer::ExecutePythonCommand(const FString& Command)
{
#if ENABLE_MCP_INSIGHTS
	TRACE_CPUPROFILER_EVENT_SCOPE(MCP_ExecutePythonCommand);
#endif

	// Python Script Pluginの存在確認
	if (!FModuleManager::Get().IsModuleLoaded("PythonScriptPlugin"))
	{
		return TEXT("{\"error\": \"PythonScriptPlugin が読み込まれていません\"}");
	}

	// Python実行
	FPythonCommandEx PythonCommand;
	PythonCommand.Command = Command;
	PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteStatement;
	PythonCommand.FileExecutionScope = EPythonFileExecutionScope::Private;

	const FString PythonResult = IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);

	// 結果をJSONでラップ
	TSharedPtr<FJsonObject> ResultJson = MakeShareable(new FJsonObject);
	ResultJson->SetStringField(TEXT("status"), TEXT("success"));
	ResultJson->SetStringField(TEXT("result"), PythonResult);

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(ResultJson.ToSharedRef(), Writer);

	return JsonString;
}

bool UMCPServer::IsLiveCodingActive() const
{
#if WITH_LIVE_CODING
	return GEngine && GEngine->IsLiveCodingEnabled();
#else
	return false;
#endif
}

TSharedPtr<FMCPMessage> UMCPServer::AcquireMessage()
{
	FScopeLock Lock(&PoolCriticalSection);
	
	if (MessagePool.Num() > 0)
	{
		return MessagePool.Pop();
	}
	
	// プールが空の場合は新しいメッセージを作成
	return MakeShared<FMCPMessage>();
}

void UMCPServer::ReleaseMessage(TSharedPtr<FMCPMessage> Message)
{
	if (Message.IsValid())
	{
		Message->Reset();
		
		FScopeLock Lock(&PoolCriticalSection);
		MessagePool.Add(Message);
	}
}

void UMCPServer::ReportPerformanceStats()
{
	double CurrentTime = FPlatformTime::Seconds();
	double ElapsedTime = CurrentTime - LastPerformanceReport;
	
	if (ElapsedTime > 0.0)
	{
		double MessagesPerSecond = ProcessedMessageCount / ElapsedTime;
		
		UE_LOG(LogMCPServer, Display, 
			TEXT("MCPサーバー統計: %d件のメッセージを%.2f秒で処理 (%.2f メッセージ/秒)"),
			ProcessedMessageCount, ElapsedTime, MessagesPerSecond);
		
		// プール状況のレポート
		{
			FScopeLock Lock(&PoolCriticalSection);
			UE_LOG(LogMCPServer, VeryVerbose, 
				TEXT("メッセージプール: %d個利用可能"), MessagePool.Num());
		}
		
		ProcessedMessageCount = 0;
	}
}
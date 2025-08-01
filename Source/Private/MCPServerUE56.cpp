#include "MCPServerUE56.h"
#include "Modules/ModuleManager.h"
#include "Engine/Engine.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/TcpSocketBuilder.h"
#include "Common/TcpListener.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Tasks/Task.h"
#include "Misc/DateTime.h"
#include "IPythonScriptPlugin.h"

DEFINE_LOG_CATEGORY(LogMCPServerUE56);

UMCPServerUE56::UMCPServerUE56()
{
	bIsServerRunning.store(false);
	bStopRequested.store(false);
	ServerPort = 3001;
	ProcessedMessageCount.store(0);
	ActiveClientCount.store(0);
	ServerStartTime = 0.0;
}

UMCPServerUE56::~UMCPServerUE56()
{
	StopServer();
}

void UMCPServerUE56::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(LogMCPServerUE56, Log, TEXT("MCPServerUE56 サブシステムを初期化中..."));
	
	// ツールレジストリを初期化
	InitializeToolRegistry();
	
	UE_LOG(LogMCPServerUE56, Log, TEXT("MCPServerUE56 サブシステムの初期化が完了しました"));
}

void UMCPServerUE56::Deinitialize()
{
	UE_LOG(LogMCPServerUE56, Log, TEXT("MCPServerUE56 サブシステムを終了中..."));
	
	StopServer();
	
	Super::Deinitialize();
	
	UE_LOG(LogMCPServerUE56, Log, TEXT("MCPServerUE56 サブシステムの終了が完了しました"));
}

bool UMCPServerUE56::StartServer(int32 Port)
{
	if (bIsServerRunning.load())
	{
		UE_LOG(LogMCPServerUE56, Warning, TEXT("MCPサーバーは既にポート%dで実行中です"), ServerPort);
		return false;
	}

	ServerPort = Port;
	bStopRequested.store(false);

	// UE5.6のInsightsプロファイリング開始
	LogMCPEvent(TEXT("ServerStart"), FString::Printf(TEXT("Port: %d"), Port));

	// ソケットサブシステムを取得
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogMCPServerUE56, Error, TEXT("ソケットサブシステムの取得に失敗しました"));
		return false;
	}

	// リスニングソケットを作成
	ListenSocket = MakeShared<FSocket>(SocketSubsystem->CreateSocket(NAME_Stream, TEXT("MCPServerSocket"), false));
	if (!ListenSocket.IsValid())
	{
		UE_LOG(LogMCPServerUE56, Error, TEXT("ソケットの作成に失敗しました"));
		return false;
	}

	// ソケットをアドレスにバインド
	TSharedRef<FInternetAddr> ListenAddr = SocketSubsystem->CreateInternetAddr();
	ListenAddr->SetIp(TEXT("127.0.0.1"));
	ListenAddr->SetPort(Port);

	if (!ListenSocket->Bind(*ListenAddr))
	{
		UE_LOG(LogMCPServerUE56, Error, TEXT("ポート%dへのバインドに失敗しました"), Port);
		ListenSocket.Reset();
		return false;
	}

	// リスニング開始
	if (!ListenSocket->Listen(8))
	{
		UE_LOG(LogMCPServerUE56, Error, TEXT("ソケットリスニングの開始に失敗しました"));
		ListenSocket.Reset();
		return false;
	}

	// UE5.6のTasksシステムを使用してサーバーループを開始
	ServerTask = UE::Tasks::Launch(TEXT("MCPServerLoop"), 
		[this]() { ServerLoop(); },
		UE::Tasks::ETaskPriority::BackgroundNormal
	);

	ServerStartTime = FPlatformTime::Seconds();
	
	UE_LOG(LogMCPServerUE56, Log, TEXT("MCPサーバーがポート%dで正常に開始されました"), Port);
	return true;
}

void UMCPServerUE56::StopServer()
{
	if (!bIsServerRunning.load())
	{
		return;
	}

	UE_LOG(LogMCPServerUE56, Log, TEXT("MCPサーバーを停止中..."));
	
	LogMCPEvent(TEXT("ServerStop"), FString::Printf(TEXT("Port: %d, Messages: %lld"), 
		ServerPort, ProcessedMessageCount.load()));

	bStopRequested.store(true);

	// 全クライアント接続を閉じる
	{
		FScopeLock Lock(&ClientsLock);
		for (auto& ClientPair : ConnectedClients)
		{
			if (ClientPair.Value.IsValid())
			{
				ClientPair.Value->Close();
			}
		}
		ConnectedClients.Empty();
	}

	// リスニングソケットを閉じる
	if (ListenSocket.IsValid())
	{
		ListenSocket->Close();
		ListenSocket.Reset();
	}

	// サーバータスクの完了を待機
	if (ServerTask.IsValid())
	{
		ServerTask.Wait();
	}

	bIsServerRunning.store(false);
	ActiveClientCount.store(0);
	
	UE_LOG(LogMCPServerUE56, Log, TEXT("MCPサーバーが正常に停止されました"));
}

FString UMCPServerUE56::ExecuteCommand(const FString& Command)
{
	LogMCPEvent(TEXT("DirectCommand"), Command);
	
	// JSON解析
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Command);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return TEXT("{\"error\": \"無効なJSONフォーマット\"}");
	}

	// ツール呼び出しとして処理
	TSharedPtr<FJsonObject> Response = ProcessToolCall(JsonObject);
	
	// レスポンスを文字列に変換
	FString ResponseString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	
	ProcessedMessageCount.fetch_add(1);
	
	return ResponseString;
}

FString UMCPServerUE56::GetServerStats() const
{
	double UpTime = ServerStartTime > 0.0 ? FPlatformTime::Seconds() - ServerStartTime : 0.0;
	
	return FString::Printf(TEXT(
		"{"
		"\"running\": %s,"
		"\"port\": %d,"
		"\"uptime\": %.2f,"
		"\"processed_messages\": %lld,"
		"\"active_clients\": %lld,"
		"\"tools_registered\": %d"
		"}"
	),
		bIsServerRunning.load() ? TEXT("true") : TEXT("false"),
		ServerPort,
		UpTime,
		ProcessedMessageCount.load(),
		ActiveClientCount.load(),
		ToolRegistry.Num()
	);
}

void UMCPServerUE56::ServerLoop()
{
	UE_LOG(LogMCPServerUE56, Log, TEXT("MCPサーバーループを開始中"));
	bIsServerRunning.store(true);

	while (!bStopRequested.load() && ListenSocket.IsValid())
	{
		// クライアント接続を待機
		bool bHasPendingConnection = false;
		if (ListenSocket->HasPendingConnection(bHasPendingConnection) && bHasPendingConnection)
		{
			TSharedPtr<FSocket> ClientSocket = MakeShared<FSocket>(ListenSocket->Accept(TEXT("MCPClient")));
			if (ClientSocket.IsValid())
			{
				// クライアント処理を別のタスクで実行（UE5.6 Tasks最適化）
				UE::Tasks::Launch(TEXT("MCPClientHandler"),
					[this, ClientSocket]() { ProcessClient(ClientSocket); },
					UE::Tasks::ETaskPriority::Normal
				);
			}
		}

		// CPU使用率を抑制するため少し待機
		FPlatformProcess::Sleep(0.01f);
	}

	bIsServerRunning.store(false);
	UE_LOG(LogMCPServerUE56, Log, TEXT("MCPサーバーループが終了しました"));
}

void UMCPServerUE56::ProcessClient(TSharedPtr<FSocket> ClientSocket)
{
	if (!ClientSocket.IsValid())
	{
		return;
	}

	int32 ClientId = FPlatformProcess::GetCurrentProcessId() + FPlatformTime::Cycles();
	
	// クライアントをリストに追加
	{
		FScopeLock Lock(&ClientsLock);
		ConnectedClients.Add(ClientId, ClientSocket);
		ActiveClientCount.fetch_add(1);
	}

	UE_LOG(LogMCPServerUE56, Log, TEXT("新しいクライアント接続: ID=%d"), ClientId);
	LogMCPEvent(TEXT("ClientConnect"), FString::Printf(TEXT("ClientID: %d"), ClientId));

	// クライアントからのメッセージを処理
	TArray<uint8> Buffer;
	Buffer.SetNum(4096);

	while (!bStopRequested.load() && ClientSocket->GetConnectionState() == SCS_Connected)
	{
		int32 BytesRead = 0;
		if (ClientSocket->Recv(Buffer.GetData(), Buffer.Num() - 1, BytesRead))
		{
			if (BytesRead > 0)
			{
				Buffer[BytesRead] = 0;
				FString MessageData = FString(UTF8_TO_TCHAR(Buffer.GetData()));
				
				if (!MessageData.IsEmpty())
				{
					HandleMCPMessage(MessageData, ClientId);
				}
			}
		}
		else
		{
			// 接続エラーまたは切断
			break;
		}

		FPlatformProcess::Sleep(0.001f);
	}

	// クライアントをリストから削除
	{
		FScopeLock Lock(&ClientsLock);
		ConnectedClients.Remove(ClientId);
		ActiveClientCount.fetch_sub(1);
	}

	ClientSocket->Close();
	
	UE_LOG(LogMCPServerUE56, Log, TEXT("クライアント切断: ID=%d"), ClientId);
	LogMCPEvent(TEXT("ClientDisconnect"), FString::Printf(TEXT("ClientID: %d"), ClientId));
}

void UMCPServerUE56::HandleMCPMessage(const FString& MessageData, int32 ClientId)
{
	LogMCPEvent(TEXT("MessageReceived"), MessageData.Left(100)); // 最初の100文字のみログ
	
	// JSON解析
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MessageData);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogMCPServerUE56, Warning, TEXT("無効なJSONメッセージを受信: %s"), *MessageData.Left(100));
		return;
	}

	// メッセージタイプに応じて処理
	FString Method;
	if (JsonObject->TryGetStringField(TEXT("method"), Method))
	{
		TSharedPtr<FJsonObject> Response;
		
		if (Method == TEXT("tools/call"))
		{
			Response = ProcessToolCall(JsonObject);
		}
		else if (Method == TEXT("resources/read"))
		{
			Response = ProcessResourceRequest(JsonObject);
		}
		else
		{
			// 未知のメソッド
			Response = MakeShared<FJsonObject>();
			Response->SetStringField(TEXT("error"), TEXT("未知のメソッド: ") + Method);
		}

		if (Response.IsValid())
		{
			SendResponse(ClientId, Response);
		}
	}

	ProcessedMessageCount.fetch_add(1);
}

void UMCPServerUE56::SendResponse(int32 ClientId, const TSharedPtr<FJsonObject>& Response)
{
	FScopeLock Lock(&ClientsLock);
	
	TSharedPtr<FSocket>* ClientSocket = ConnectedClients.Find(ClientId);
	if (!ClientSocket || !ClientSocket->IsValid())
	{
		return;
	}

	// レスポンスをJSON文字列に変換
	FString ResponseString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

	// UTF-8に変換して送信
	FTCHARToUTF8 UTF8String(*ResponseString);
	int32 BytesSent = 0;
	(*ClientSocket)->Send((uint8*)UTF8String.Get(), UTF8String.Length(), BytesSent);
	
	LogMCPEvent(TEXT("ResponseSent"), ResponseString.Left(100));
}

TSharedPtr<FJsonObject> UMCPServerUE56::ProcessToolCall(const TSharedPtr<FJsonObject>& Request)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	
	// パラメータを取得
	const TSharedPtr<FJsonObject>* ParamsPtr;
	if (!Request->TryGetObjectField(TEXT("params"), ParamsPtr) || !ParamsPtr->IsValid())
	{
		Response->SetStringField(TEXT("error"), TEXT("パラメータが見つかりません"));
		return Response;
	}

	TSharedPtr<FJsonObject> Params = *ParamsPtr;
	FString ToolName;
	if (!Params->TryGetStringField(TEXT("name"), ToolName))
	{
		Response->SetStringField(TEXT("error"), TEXT("ツール名が指定されていません"));
		return Response;
	}

	// ツールレジストリから関数を検索
	if (ToolRegistry.Contains(ToolName))
	{
		TFunction<TSharedPtr<FJsonObject>(const TSharedPtr<FJsonObject>&)>& ToolFunction = ToolRegistry[ToolName];
		return ToolFunction(Params);
	}
	else
	{
		Response->SetStringField(TEXT("error"), TEXT("未知のツール: ") + ToolName);
		return Response;
	}
}

TSharedPtr<FJsonObject> UMCPServerUE56::ProcessResourceRequest(const TSharedPtr<FJsonObject>& Request)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetStringField(TEXT("content"), TEXT("リソース機能は実装予定です"));
	return Response;
}

FString UMCPServerUE56::ExecutePythonCommand(const FString& Command)
{
	// UE5.6のLive Coding対応チェック
	if (GEngine && GEngine->IsLiveCodingEnabled())
	{
		return TEXT("{\"status\": \"deferred\", \"message\": \"Live Coding実行中のため延期されました\"}");
	}

	// Python Script Plugin を使用してコマンドを実行
	if (IPythonScriptPlugin::Get())
	{
		FPythonCommandEx PythonCommand;
		PythonCommand.Command = Command;
		PythonCommand.ExecutionMode = EPythonCommandExecutionMode::ExecuteStatement;
		PythonCommand.FileExecutionScope = EPythonFileExecutionScope::Private;
		
		return IPythonScriptPlugin::Get()->ExecPythonCommandEx(PythonCommand);
	}

	return TEXT("{\"error\": \"Python Script Plugin が利用できません\"}");
}

void UMCPServerUE56::InitializeToolRegistry()
{
	UE_LOG(LogMCPServerUE56, Log, TEXT("ツールレジストリを初期化中..."));

	// 基本的なPythonツールを登録
	RegisterPythonTools();

	UE_LOG(LogMCPServerUE56, Log, TEXT("ツールレジストリの初期化完了: %d個のツールを登録"), ToolRegistry.Num());
}

void UMCPServerUE56::RegisterPythonTools()
{
	// get_project_dir ツール
	ToolRegistry.Add(TEXT("get_project_dir"), [this](const TSharedPtr<FJsonObject>& Params) -> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
		FString ProjectDir = FPaths::ProjectDir();
		Response->SetStringField(TEXT("result"), ProjectDir);
		Response->SetStringField(TEXT("status"), TEXT("success"));
		return Response;
	});

	// execute_python ツール  
	ToolRegistry.Add(TEXT("execute_python"), [this](const TSharedPtr<FJsonObject>& Params) -> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
		
		FString PythonCode;
		if (Params->TryGetStringField(TEXT("code"), PythonCode))
		{
			FString Result = ExecutePythonCommand(PythonCode);
			Response->SetStringField(TEXT("result"), Result);
			Response->SetStringField(TEXT("status"), TEXT("success"));
		}
		else
		{
			Response->SetStringField(TEXT("error"), TEXT("Python コードが指定されていません"));
		}
		
		return Response;
	});

	// get_server_stats ツール
	ToolRegistry.Add(TEXT("get_server_stats"), [this](const TSharedPtr<FJsonObject>& Params) -> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
		Response->SetStringField(TEXT("result"), GetServerStats());
		Response->SetStringField(TEXT("status"), TEXT("success"));
		return Response;
	});
}

void UMCPServerUE56::LogMCPEvent(const FString& EventName, const FString& Data) const
{
	// UE5.6のInsightsプロファイリング
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*EventName);
	
	#if WITH_EDITOR
	// エディタビルドでのみブックマークを記録
	TRACE_BOOKMARK(TEXT("MCP"), *FString::Printf(TEXT("%s: %s"), *EventName, *Data.Left(50)));
	#endif
	
	// 通常のログも出力
	UE_LOG(LogMCPServerUE56, VeryVerbose, TEXT("MCP Event - %s: %s"), *EventName, *Data.Left(100));
}
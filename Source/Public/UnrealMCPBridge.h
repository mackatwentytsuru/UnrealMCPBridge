#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Unreal MCP Bridge モジュール
 * 
 * このモジュールは、MCPクライアントがUnreal Engine Editor Python APIに
 * アクセスできるようにするMCP（Model Context Protocol）サーバーを実装します。
 * 
 * Unreal Engine 5.6 対応
 */
class FUnrealMCPBridgeModule : public IModuleInterface
{
public:
	/** IModuleInterface実装 */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** MCPサーバーとPython統合を初期化 */
	void InitializeMCPServer();
	
	/** MCPサーバーリソースをクリーンアップ */
	void ShutdownMCPServer();
	
	/** Python Script Pluginが利用可能かチェック */
	bool IsPythonScriptPluginEnabled() const;
};
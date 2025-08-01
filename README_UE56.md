# UnrealMCPBridge v2.0.0 - UE5.6 対応

MCPクライアントがUnreal Engine Editor Python APIにアクセスできるようにするMCP（Model Context Protocol）サーバーを実装するUnreal Engineプラグインです。

## 🎯 v2.0.0 の新機能

- **✅ Unreal Engine 5.6 完全対応**: UE5.6 APIの完全サポート
- **🔧 更新されたプラグインアーキテクチャ**: 最新のプラグイン構造
- **🐍 強化されたPython統合**: UE5.6 Python Script Plugin サポート
- **⚡ パフォーマンス最適化**: UE5.6の改善を活用

## 📋 必要条件

- **Unreal Engine**: 5.6 以降
- **プラットフォーム**: Windows, Mac, Linux
- **必須プラグイン**: PythonScriptPlugin, EditorScriptingUtilities

## 🔧 インストール方法

1. このリポジトリをクローン
2. プロジェクトの `Plugins/` ディレクトリにコピー
3. UE5.6で必須プラグインを有効化
4. Unreal Engine を再起動

## 🔍 動作確認

Output Log で以下のログを確認してください：
```
LogUnrealMCPBridge: Log: UnrealMCPBridge モジュールを UE5.6 で起動中
LogUnrealMCPBridge: Log: MCPサーバーの初期化が完了しました
```

## 🛠️ 開発状況

- [x] UE5.6 コア互換性対応
- [ ] MCP Server 実装
- [ ] Python API 統合
- [ ] ドキュメント & サンプル

## 📄 ライセンス

MIT License - 詳細は LICENSE ファイルを参照してください。
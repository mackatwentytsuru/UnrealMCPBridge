# Unreal Engine MCP Python Bridge プラグイン

これは、Model Context Protocol（MCP）のサーバー実装を作成するUnreal Engine（UE）用プラグインです。これにより、AnthropicのClaude等のMCPクライアントが、UE Python APIの全機能にアクセスできるようになります。

## 🎯 概要

UnrealMCPBridgeは、Unreal Engine 5.6に対応したプラグインで、MCPプロトコルを通じてUnreal Engine Editor Python APIへのブリッジを提供します。

## ✨ 主な機能

- **🌐 MCP サーバー実装**: Model Context Protocol の完全なサーバー実装
- **🐍 Python API ブリッジ**: Unreal Engine Python API への直接アクセス
- **🤖 Claude 統合**: Anthropic Claude との seamless な連携
- **⚡ UE5.6 対応**: 最新の Unreal Engine 5.6 API を活用
- **🖥️ クロスプラットフォーム**: Windows, Mac, Linux をサポート

## 📋 必要環境

- **Unreal Engine**: 5.6 以降
- **Python Script Plugin**: 必須
- **Editor Scripting Utilities Plugin**: 推奨

## 🚀 インストール

1. このリポジトリをクローン
2. プロジェクトの `Plugins/` フォルダに配置
3. Unreal Engine を起動し、プラグインを有効化
4. 必要な依存プラグインも有効化
5. エディタを再起動

## 🛠️ 開発状況

- [x] UE5.6 基盤対応 + 日本語化
- [ ] MCP サーバー実装
- [ ] Python API 統合
- [ ] Claude 連携テスト

## 📄 ライセンス

MIT License

詳細は [README_UE56.md](README_UE56.md) をご覧ください。
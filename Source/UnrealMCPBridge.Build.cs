using UnrealBuildTool;

public class UnrealMCPBridge : ModuleRules
{
	public UnrealMCPBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// UE 5.6必須設定
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppStandard = CppStandardVersion.Latest; // C++20対応

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject", 
				"Engine",
				"UnrealEd",
				"EditorSubsystem",
				"EditorScriptingUtilities",
				"PythonScriptPlugin",
				"Sockets",
				"Networking",
				"Json",
				"JsonUtilities"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"ToolMenus",
				"LevelEditor",
				"EditorStyle",
				"EditorWidgets",
				"Slate",
				"SlateCore",
				"PropertyEditor",
				"ContentBrowser",
				"WorkspaceMenuStructure"
			}
		);

		// UE 5.6の新機能を活用
		if (Target.Version.MajorVersion >= 5 && Target.Version.MinorVersion >= 6)
		{
			PublicDependencyModuleNames.AddRange(new string[] {
				"RHI", // Bindless resourcesサポート用
				"Tasks" // 新しいTasksシステム用
			});
		}
	}
}
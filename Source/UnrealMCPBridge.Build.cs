using UnrealBuildTool;

public class UnrealMCPBridge : ModuleRules
{
	public UnrealMCPBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject", 
				"Engine",
				"UnrealEd",
				"EditorScriptingUtilities",
				"PythonScriptPlugin"
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
	}
}
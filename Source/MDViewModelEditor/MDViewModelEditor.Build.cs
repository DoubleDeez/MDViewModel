using UnrealBuildTool;

public class MDViewModelEditor : ModuleRules
{
    public MDViewModelEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "GameplayTags",
                "MDViewModel",
                "StructUtilsEditor"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
	            "BlueprintGraph",
                "CoreUObject",
                "Engine",
                "GraphEditor",
                "InputCore",
                "Kismet",
                "MDViewModelGraph",
                "PropertyEditor",
                "Slate",
                "SlateCore",
                "StructUtils",
                "ToolWidgets",
                "UMG",
                "UMGEditor",
                "UnrealEd"
            }
        );
    }
}
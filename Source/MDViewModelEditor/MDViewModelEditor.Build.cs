using UnrealBuildTool;

public class MDViewModelEditor : ModuleRules
{
    public MDViewModelEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "GameplayTags",
                "MDViewModel"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
	            "BlueprintGraph",
                "CoreUObject",
                "Engine",
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
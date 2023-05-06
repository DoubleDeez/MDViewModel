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
                "MDViewModel"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "InputCore",
                "Kismet",
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
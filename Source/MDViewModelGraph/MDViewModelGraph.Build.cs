using UnrealBuildTool;

public class MDViewModelGraph : ModuleRules
{
    public MDViewModelGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "BlueprintGraph",
                "Kismet"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "MDViewModel",
                "Slate",
                "SlateCore",
                "StructUtils",
                "UMG",
                "UMGEditor",
                "UnrealEd"
            }
        );
    }
}
using UnrealBuildTool;

public class MDViewModelGraph : ModuleRules
{
    public MDViewModelGraph(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "BlueprintGraph"
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
                "UMGEditor",
                "UnrealEd"
            }
        );

        if (Target.Type == TargetType.Editor)
        {
	        PrivateDependencyModuleNames.Add("MDViewModelEditor");
        }
    }
}
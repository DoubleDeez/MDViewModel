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
                "BlueprintGraph",
                "Kismet"
            }
        );

#if UE_5_3_OR_LATER
	    PrivateDependencyModuleNames.Add("FieldNotification");
#endif

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "GameplayTags",
                "KismetCompiler",
                "MDViewModel",
                "Slate",
                "SlateCore",
                "StructUtils",
                "ToolMenus",
                "UMG",
                "UMGEditor",
                "UnrealEd"
            }
        );
    }
}
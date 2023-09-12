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
		
#if UE_5_3_OR_LATER
        PublicDependencyModuleNames.Add("FieldNotification");
#endif

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
	            "ApplicationCore",
	            "BlueprintGraph",
                "CoreUObject",
                "DeveloperSettings",
                "Engine",
                "GraphEditor",
                "InputCore",
                "Kismet",
                "MDViewModelGraph",
                "PropertyEditor",
                "SharedSettingsWidgets",
                "Slate",
                "SlateCore",
                "SourceControl",
                "StructUtils",
                "ToolWidgets",
                "UMG",
                "UMGEditor",
                "UnrealEd"
            }
        );
    }
}
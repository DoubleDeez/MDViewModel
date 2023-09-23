// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MDViewModel : ModuleRules
{
	public MDViewModel(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"DeveloperSettings",
				"GameplayTags",
				"StructUtils",
				"UMG"
			}
		);

#if UE_5_3_OR_LATER
		PublicDependencyModuleNames.Add("FieldNotification");
#endif

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore"
			}
		);

		if (Target.Type == TargetType.Editor)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"UMGEditor",
					"UnrealEd"
				}
			);
		}
	}
}

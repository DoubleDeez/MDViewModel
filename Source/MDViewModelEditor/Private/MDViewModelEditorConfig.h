#pragma once

#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "MDViewModelEditorConfig.generated.h"

enum class EClassViewerNameTypeToDisplay: uint8;

UENUM()
enum class EMDVMClassViewerNameTypeToDisplay : uint8
{
	/** Display both the display name and class name if they're available and different. */
	Dynamic,
	/** Always use the display name */
	DisplayName,
	/** Always use the class name */
	ClassName,
};

/**
 * Settings 
 */
UCLASS(DefaultConfig, Config = MDViewModelEditor, meta = (DisplayName = "View Model Editor"))
class MDVIEWMODELEDITOR_API UMDViewModelEditorConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMDViewModelEditorConfig();
	
	virtual FName GetContainerName() const override { return TEXT("Editor"); }

	virtual void PostInitProperties() override;
	
	// How the view model classes are displayed in the class selection list
	UPROPERTY(EditDefaultsOnly, Config, Category = "View Model Dialog")
	EMDVMClassViewerNameTypeToDisplay NameTypeToDisplay = EMDVMClassViewerNameTypeToDisplay::DisplayName;

	// If true, a gameplay tag selector will be used instead of a text field to enter view model instance names
	UPROPERTY(EditDefaultsOnly, Config, Category = "View Model Dialog")
	bool bUseGameplayTagsForViewModelNaming = false;

	// If set, view model name tag selection is limited to children of this tag.
	UPROPERTY(EditDefaultsOnly, Config, Category = "View Model Dialog", meta = (EditCondition = "bUseGameplayTagsForViewModelNaming", EditConditionHides))
	FGameplayTag ViewModelNameRootTag;

	// WIP - Not Yet Functional: If true, the view model editor will be enabled in actor blueprints
	UPROPERTY(VisibleAnywhere, Config, Category = "Actor View Models")
	bool bEnableViewModelsInActorBlueprints = false;

	EClassViewerNameTypeToDisplay GetNameTypeToDisplay() const;
};

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

	// If true, the view model editor will be enabled in actor blueprints.
	// Any existing view model assignments will continue to work even if the editor is disabled.
	UPROPERTY(EditDefaultsOnly, Config, Category = "Actor View Models")
	bool bEnableViewModelsInActorBlueprints = true;

	// If true, the view model editor will call functions that appear in the Properties list and display the values of the result in the view model editor
	// This only happens when an object with a valid view model is selected in the blueprint editor
	UPROPERTY(EditDefaultsOnly, Config, Category = "View Model Debugger")
	bool bEnableReturnValuePreviewing = true;

	EClassViewerNameTypeToDisplay GetNameTypeToDisplay() const;
};

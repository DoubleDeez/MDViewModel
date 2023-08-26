#pragma once

#include "Engine/DeveloperSettings.h"
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
UCLASS(Config = MDViewModelEditor, meta = (DisplayName = "View Model Editor"))
class MDVIEWMODELEDITOR_API UMDViewModelEditorConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMDViewModelEditorConfig();
	
	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	
	// How the view model classes are displayed in the class selection list
	UPROPERTY(EditDefaultsOnly, Config, Category = "View Model Dialog")
	EMDVMClassViewerNameTypeToDisplay NameTypeToDisplay = EMDVMClassViewerNameTypeToDisplay::DisplayName;

	EClassViewerNameTypeToDisplay GetNameTypeToDisplay() const;
};

#pragma once

#include "CoreMinimal.h"
#include "MDViewModelUtils.h"
#include "UObject/Object.h"
#include "MDViewModelAssignmentReference.generated.h"

class UMDViewModelBase;

DECLARE_DELEGATE_RetVal(UClass*, FMDViewModelReferenceGetWidgetClass);

/**
 * Help struct to store a reference to viewmodel class assigned to a widget.
 * Has an editor customization to make selecting the viewmodel user-friendly
 */
USTRUCT()
struct MDVIEWMODEL_API FMDViewModelAssignmentReference
{
	GENERATED_BODY()

public:
	// The viewmodel class to reference
	UPROPERTY(EditAnywhere, Category = "View Model")
	TSoftClassPtr<UMDViewModelBase> ViewModelClass;

	// The name of the viewmodel viewmodel instance to reference
	UPROPERTY(EditAnywhere, Category = "View Model")
	FName ViewModelName = MDViewModelUtils::DefaultViewModelName;

#if WITH_EDITOR
	// Only used for editor customization
	FMDViewModelReferenceGetWidgetClass OnGetWidgetClass;
#endif
};

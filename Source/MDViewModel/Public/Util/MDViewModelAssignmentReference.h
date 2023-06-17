#pragma once

#include "MDViewModelUtils.h"
#include "UObject/Object.h"
#include "MDViewModelAssignmentReference.generated.h"

class UMDViewModelBase;
class UUserWidget;

#if WITH_EDITOR
DECLARE_DELEGATE_RetVal(UClass*, FMDViewModelReferenceGetWidgetClass);
#endif

/**
 * Helper struct to store a reference to a widget's view model assignment.
 * Has an editor customization to make selecting the view model user-friendly
 */
USTRUCT()
struct MDVIEWMODEL_API FMDViewModelAssignmentReference
{
	GENERATED_BODY()

public:
	// The view model class to reference
	UPROPERTY(EditAnywhere, Category = "View Model")
	TSoftClassPtr<UMDViewModelBase> ViewModelClass;

	// The name of the view model instance to reference
	UPROPERTY(EditAnywhere, Category = "View Model")
	FName ViewModelName = MDViewModelUtils::DefaultViewModelName;

	UMDViewModelBase* ResolveViewModelAssignment(const UUserWidget* Widget) const;

	bool IsAssignmentValid() const;

#if WITH_EDITOR
	// Only used for editor customization
	FMDViewModelReferenceGetWidgetClass OnGetWidgetClass;
#endif

	FMDViewModelAssignmentReference& operator=(const FMDViewModelAssignmentReference& Other);
};

template<>
struct TStructOpsTypeTraits<FMDViewModelAssignmentReference> : public TStructOpsTypeTraitsBase2<FMDViewModelAssignmentReference>
{
	enum
	{
		WithCopy = true,
	};
};

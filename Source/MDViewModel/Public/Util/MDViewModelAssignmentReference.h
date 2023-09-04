#pragma once

#include "MDViewModelUtils.h"
#include "UObject/Object.h"
#include "MDViewModelAssignmentReference.generated.h"

class UMDViewModelBase;


/**
 * Helper struct to store a reference to a widget's view model assignment.
 * Has an editor customization to make selecting the view model user-friendly
 */
USTRUCT(BlueprintType)
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

	UMDViewModelBase* ResolveViewModelAssignment(const UObject* Object) const;

	bool IsAssignmentValid() const;

#if WITH_EDITOR
	// Only used for editor customization
	DECLARE_DELEGATE_RetVal(UClass*, FMDViewModelReferenceGetObjectClass);
	UE_DEPRECATED(All, "OnGetWidgetClass is deprecated, bind it OnGetBoundObjectClass instead.")
	FMDViewModelReferenceGetObjectClass OnGetWidgetClass;
	FMDViewModelReferenceGetObjectClass OnGetBoundObjectClass;

	UClass* GetBoundObjectClass() const
	{
		if (OnGetBoundObjectClass.IsBound())
		{
			return OnGetBoundObjectClass.Execute();
		}

		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		if (OnGetWidgetClass.IsBound())
		{
			return OnGetWidgetClass.Execute();
		}
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

		return nullptr;
	}
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

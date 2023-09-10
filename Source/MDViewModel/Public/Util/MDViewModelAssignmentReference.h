#pragma once

#include "MDViewModelUtils.h"
#include "UObject/Object.h"
#include "MDViewModelAssignmentReference.generated.h"

struct FMDViewModelAssignment;
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
	FMDViewModelAssignmentReference() = default;
	explicit FMDViewModelAssignmentReference(const FMDViewModelAssignment& Assignment);
	FMDViewModelAssignmentReference(TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	
	// The view model class to reference
	UPROPERTY(EditAnywhere, Category = "View Model")
	TSoftClassPtr<UMDViewModelBase> ViewModelClass;

	// The name of the view model instance to reference
	UPROPERTY(EditAnywhere, Category = "View Model")
	FName ViewModelName = MDViewModelUtils::DefaultViewModelName;

	bool IsAssignmentValid() const;

#if WITH_EDITOR
	// Only used for editor customization
	DECLARE_DELEGATE_RetVal(UClass*, FMDViewModelReferenceGetObjectClass);
	UE_DEPRECATED(All, "OnGetWidgetClass is deprecated, bind to OnGetBoundObjectClass instead.")
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

	bool operator==(const FMDViewModelAssignmentReference& Other) const;

	friend uint32 GetTypeHash(const FMDViewModelAssignmentReference& AssignmentReference)
	{
		return HashCombine(GetTypeHash(AssignmentReference.ViewModelName), GetTypeHash(AssignmentReference.ViewModelClass));
	}
};

template<>
struct TStructOpsTypeTraits<FMDViewModelAssignmentReference> : public TStructOpsTypeTraitsBase2<FMDViewModelAssignmentReference>
{
	enum
	{
		WithCopy = true,
	};
};

MDVIEWMODEL_API FCbWriter& operator<<(FCbWriter& Writer, const FMDViewModelAssignmentReference& Assignment);

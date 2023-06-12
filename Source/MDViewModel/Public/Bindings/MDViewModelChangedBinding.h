#pragma once

#include "Engine/DynamicBlueprintBinding.h"
#include "Templates/SubclassOf.h"
#include "MDViewModelChangedBinding.generated.h"

class UUserWidget;
class UMDViewModelBase;

/** Entry for a delegate to assign after a blueprint has been instanced */
USTRUCT()
struct MDVIEWMODEL_API FMDViewModelChangedBindingEntry
{
	GENERATED_BODY()

public:
	// Class of the view model we're binding to
	UPROPERTY()
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	// Name of the view model we're binding to
	UPROPERTY()
	FName ViewModelName = NAME_None;

	// Name of the function that we're binding
	UPROPERTY()
	FName FunctionNameToBind = NAME_None;
};

/**
 * Class to handle binding to a view model changing at runtime
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelChangedBinding : public UDynamicBlueprintBinding
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FMDViewModelChangedBindingEntry> ViewModelChangedBindings;

	virtual void BindDynamicDelegates(UObject* InInstance) const override;
	virtual void UnbindDynamicDelegates(UObject* InInstance) const override;

private:
	void OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UUserWidget> BoundWidget) const;
};

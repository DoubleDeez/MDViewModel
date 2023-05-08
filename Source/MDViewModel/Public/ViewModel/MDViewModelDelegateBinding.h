#pragma once

#include "CoreMinimal.h"
#include "Engine/DynamicBlueprintBinding.h"
#include "Templates/SubclassOf.h"
#include "MDViewModelDelegateBinding.generated.h"

class UUserWidget;
class UMDViewModelBase;

/** Entry for a delegate to assign after a blueprint has been instanced */
USTRUCT()
struct MDVIEWMODEL_API FMDViewModelDelegateBindingEntry
{
	GENERATED_BODY()

public:
	// Class of the view model we're binding to
	UPROPERTY()
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	// Name of the view model we're binding to
	UPROPERTY()
	FName ViewModelName = NAME_None;

	// Name of the delegate on the viewmodel we're going to bind to
	UPROPERTY()
	FName DelegatePropertyName = NAME_None;

	// Name of the function that we're binding to the delegate
	UPROPERTY()
	FName FunctionNameToBind = NAME_None;
};

/**
 * Class to handle binding to viewmodel events at runtime
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelDelegateBinding : public UDynamicBlueprintBinding
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FMDViewModelDelegateBindingEntry> ViewModelDelegateBindings;

	virtual void BindDynamicDelegates(UObject* InInstance) const override;
	virtual void UnbindDynamicDelegates(UObject* InInstance) const override;

private:
	void OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UUserWidget> BoundWidget) const;
};

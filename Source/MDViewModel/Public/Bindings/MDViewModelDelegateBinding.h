#pragma once

#include "MDVMBlueprintBindingBase.h"
#include "MDViewModelDelegateBinding.generated.h"

class UMDViewModelBase;

/** Entry for a delegate to assign after a blueprint has been instanced */
USTRUCT()
struct MDVIEWMODEL_API FMDViewModelDelegateBindingEntry : public FMDViewModeBindingEntryBase
{
	GENERATED_BODY()

public:
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
class MDVIEWMODEL_API UMDViewModelDelegateBinding : public UMDVMBlueprintBindingBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FMDViewModelDelegateBindingEntry> ViewModelDelegateBindings;

protected:
	virtual void OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UObject> BoundObject) const override;

	virtual const FMDViewModeBindingEntryBase* GetEntry(int32 Index) const override { return ViewModelDelegateBindings.IsValidIndex(Index) ? &ViewModelDelegateBindings[Index] : nullptr; }
	virtual int32 GetNumEntries() const override { return ViewModelDelegateBindings.Num(); }
};

#pragma once

#include "MDVMBlueprintBindingBase.h"
#include "MDViewModelChangedBinding.generated.h"

class UMDViewModelBase;

/** Entry for a delegate to assign after a blueprint has been instanced */
USTRUCT()
struct MDVIEWMODEL_API FMDViewModelChangedBindingEntry : public FMDViewModeBindingEntryBase
{
	GENERATED_BODY()

public:
	// Name of the function that we're binding
	UPROPERTY()
	FName FunctionNameToBind = NAME_None;
};

/**
 * Class to handle binding to a view model changing at runtime
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelChangedBinding : public UMDVMBlueprintBindingBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FMDViewModelChangedBindingEntry> ViewModelChangedBindings;

private:
	virtual void OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UObject> BoundObject) const override;
	
	virtual const FMDViewModeBindingEntryBase* GetEntry(int32 Index) const override { return ViewModelChangedBindings.IsValidIndex(Index) ? &ViewModelChangedBindings[Index] : nullptr; }
	virtual int32 GetNumEntries() const override { return ViewModelChangedBindings.Num(); }
};

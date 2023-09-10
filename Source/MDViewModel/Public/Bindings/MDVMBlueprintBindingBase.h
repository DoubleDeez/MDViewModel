#pragma once

#include "Engine/DynamicBlueprintBinding.h"
#include "Templates/SubclassOf.h"
#include "MDVMBlueprintBindingBase.generated.h"

class UMDViewModelBase;
class IMDViewModelRuntimeInterface;

USTRUCT()
struct MDVIEWMODEL_API FMDViewModeBindingEntryBase
{
	GENERATED_BODY()

public:
	// Class of the view model we're binding to
	UPROPERTY()
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	// Name of the view model we're binding to
	UPROPERTY()
	FName ViewModelName = NAME_None;
};

UCLASS()
class MDVIEWMODEL_API UMDVMBlueprintBindingBase : public UDynamicBlueprintBinding
{
	GENERATED_BODY()

	friend class UMDViewModelAssignmentComponent;

protected:
	virtual void BindViewModelDelegates(IMDViewModelRuntimeInterface& Object) const;
	virtual void UnbindViewModelDelegates(IMDViewModelRuntimeInterface& Object) const;
	
	virtual void OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UObject> BoundObject) const {}

	virtual const FMDViewModeBindingEntryBase* GetEntry(int32 Index) const { return nullptr; }
	virtual int32 GetNumEntries() const { return 0; }

private:
	virtual void BindDynamicDelegates(UObject* InInstance) const override final;
	virtual void UnbindDynamicDelegates(UObject* InInstance) const override final;
};

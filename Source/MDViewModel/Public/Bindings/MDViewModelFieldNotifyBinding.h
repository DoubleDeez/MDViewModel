#pragma once

#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
#include "FieldNotificationId.h"
#else
#include "FieldNotification/FieldId.h"
#endif
#include "MDVMBlueprintBindingBase.h"
#include "MDViewModelFieldNotifyBinding.generated.h"

class UMDViewModelBase;

/** Entry for a field notify to assign after a blueprint has been instanced */
USTRUCT()
struct MDVIEWMODEL_API FMDViewModelFieldNotifyBindingEntry : public FMDViewModeBindingEntryBase
{
	GENERATED_BODY()

public:
	// Name of the field notify property/function on the viewmodel we're going to bind to
	UPROPERTY()
	FName FieldNotifyName = NAME_None;

	// Name of the function that we're binding to the delegate
	UPROPERTY()
	FName FunctionNameToBind = NAME_None;
};

/**
 * Class to handle binding to view model field notify properties/functions at runtime
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelFieldNotifyBinding : public UMDVMBlueprintBindingBase
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FMDViewModelFieldNotifyBindingEntry> ViewModelFieldNotifyBindings;

protected:
	virtual void OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UObject> BoundObject) const override;
	
	virtual const FMDViewModeBindingEntryBase* GetEntry(int32 Index) const override { return ViewModelFieldNotifyBindings.IsValidIndex(Index) ? &ViewModelFieldNotifyBindings[Index] : nullptr; }
	virtual int32 GetNumEntries() const override { return ViewModelFieldNotifyBindings.Num(); }

private:
	void OnFieldValueChanged(UObject* ViewModel, UE::FieldNotification::FFieldId Field, int32 EntryIndex, TWeakObjectPtr<UObject> BoundObject) const;

	mutable TMap<TTuple<int32, TWeakObjectPtr<UObject>>, FDelegateHandle> BoundDelegates;
};

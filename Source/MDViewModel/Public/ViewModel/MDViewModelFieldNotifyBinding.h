#pragma once

#include "CoreMinimal.h"
#include "Engine/DynamicBlueprintBinding.h"
#include "FieldNotification/FieldId.h"
#include "Templates/SubclassOf.h"
#include "MDViewModelFieldNotifyBinding.generated.h"

class UMDViewModelBase;
class UUserWidget;

/** Entry for a field notify to assign after a blueprint has been instanced */
USTRUCT()
struct MDVIEWMODEL_API FMDViewModelFieldNotifyBindingEntry
{
	GENERATED_BODY()

public:
	// Class of the view model we're binding to
	UPROPERTY()
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	// Name of the view model we're binding to
	UPROPERTY()
	FName ViewModelName = NAME_None;

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
class MDVIEWMODEL_API UMDViewModelFieldNotifyBinding : public UDynamicBlueprintBinding
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FMDViewModelFieldNotifyBindingEntry> ViewModelFieldNotifyBindings;

	virtual void BindDynamicDelegates(UObject* InInstance) const override;
	virtual void UnbindDynamicDelegates(UObject* InInstance) const override;

private:
	void OnViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, int32 EntryIndex, TWeakObjectPtr<UUserWidget> BoundWidget) const;

	void OnFieldValueChanged(UObject* ViewModel, UE::FieldNotification::FFieldId Field, int32 EntryIndex, TWeakObjectPtr<UUserWidget> BoundWidget) const;

	mutable TMap<TWeakObjectPtr<UUserWidget>, FDelegateHandle> BoundDelegates;
};

#pragma once

#include "Debugging/SKismetDebugTreeView.h"
#include "Templates/SubclassOf.h"
#include "Util/MDViewModelAssignmentReference.h"

class FMDVMInspectorDragAndDropActionBase;
class UMDViewModelBase;
class UWidgetBlueprint;

class FMDViewModelDebugLineItemBase : public FDebugLineItem
{
public:
	FMDViewModelDebugLineItemBase(const FText& DisplayName, const FText& Description, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, bool bIsFieldNotify = false, UWidgetBlueprint* WidgetBP = nullptr, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None);

	// TODO - Add DebugViewModel
	void UpdateViewModel(const FName& InViewModelName, TSubclassOf<UMDViewModelBase> InViewModelClass);

	virtual uint32 GetHash() override
	{
		return HashCombine(GetTypeHash(ViewModelName), GetTypeHash(ViewModelClass));
	}

	virtual TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateDragAndDropAction() const;

	FMDViewModelAssignmentReference GetViewModelAssignmentReference() const;

protected:
	virtual void UpdateCachedChildren() const {};

	virtual bool HasChildren() const override;

	virtual void GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch) override;

	virtual TSharedRef<SWidget> GenerateNameWidget(TSharedPtr<FString> InSearchString) override;

	virtual FText GetDisplayName() const override;

	virtual FText GetDescription() const override;

	mutable TOptional<TArray<FDebugTreeItemPtr>> CachedChildren;

	mutable TMap<FName, FDebugTreeItemPtr> CachedPropertyItems;

	FText DisplayName;
	FText Description;
	bool bIsFieldNotify = false;
	TWeakObjectPtr<UWidgetBlueprint> WidgetBP;
	TSubclassOf<UMDViewModelBase> ViewModelClass;
	TWeakObjectPtr<UMDViewModelBase> DebugViewModel;
	FName ViewModelName = NAME_None;
};

#pragma once

#include "Debugging/SKismetDebugTreeView.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Templates/SubclassOf.h"
#include "Util/MDViewModelAssignmentReference.h"

class FBlueprintEditor;
class FMDVMInspectorDragAndDropActionBase;
class UMDViewModelBase;
class UBlueprint;

class FMDViewModelDebugLineItemBase : public FDebugLineItem
{
public:
	FMDViewModelDebugLineItemBase(const FText& DisplayName, const FText& Description, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, bool bIsFieldNotify = false, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None);

	// TODO - Add DebugViewModel
	void UpdateViewModel(const FName& InViewModelName, TSubclassOf<UMDViewModelBase> InViewModelClass);

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
	virtual uint32 GetHash() const override
#else
	virtual uint32 GetHash() override
#endif
	{
		return HashCombine(GetTypeHash(ViewModelName), GetTypeHash(ViewModelClass));
	}

	virtual TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateDragAndDropAction() const;

	FMDViewModelAssignmentReference GetViewModelAssignmentReference() const;

protected:
	virtual void UpdateCachedChildren() const {};

	virtual bool HasChildren() const override;
	
	virtual void ExtendContextMenu(FMenuBuilder& MenuBuilder, bool bInDebuggerTab) override;

	virtual void GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch) override;

	virtual TSharedRef<SWidget> GenerateNameWidget(TSharedPtr<FString> InSearchString) override;

	virtual FText GetDisplayName() const override;

	virtual FText GetDescription() const override;

	virtual bool CanCreateNodes() const;

	virtual FString GenerateSearchString() const { return {}; }
	
	void OnFindReferencesClicked() const;

	mutable TOptional<TArray<FDebugTreeItemPtr>> CachedChildren;

	mutable TMap<FName, FDebugTreeItemPtr> CachedPropertyItems;

	FText DisplayName;
	FText Description;
	bool bIsFieldNotify = false;
	TWeakPtr<FBlueprintEditor> BlueprintEditorPtr;
	TWeakObjectPtr<UBlueprint> BlueprintPtr;
	TSubclassOf<UMDViewModelBase> ViewModelClass;
	TWeakObjectPtr<UMDViewModelBase> DebugViewModel;
	FName ViewModelName = NAME_None;
};

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
	FMDViewModelDebugLineItemBase(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, const FMDViewModelAssignmentReference& Assignment);

	void UpdateViewModel(const FMDViewModelAssignmentReference& InAssignment);
	void UpdateDebugging(bool InIsDebugging, TWeakObjectPtr<UMDViewModelBase> InDebugViewModel);

	void SetDisplayText(const FText& Name, const FText& Desc);

	virtual bool Compare(const FDebugLineItem* BaseOther) const override
	{
		const FMDViewModelDebugLineItemBase* Other = static_cast<const FMDViewModelDebugLineItemBase*>(BaseOther);
		return GetTypeName() == Other->GetTypeName() && Assignment == Other->Assignment;
	}

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
	virtual uint32 GetHash() const override
#else
	virtual uint32 GetHash() override
#endif
	{
		return HashCombine(GetTypeHash(Assignment), GetTypeHash(GetTypeName()));
	}

	virtual TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateDragAndDropAction() const;

	virtual bool CanDrag() const;

	const FMDViewModelAssignmentReference& GetViewModelAssignmentReference() const { return Assignment; }

protected:
	virtual void UpdateCachedChildren() const {};

	virtual bool HasChildren() const override;

	virtual FName GetTypeName() const { return TEXT("Base"); }

	virtual void ExtendContextMenu(FMenuBuilder& MenuBuilder, bool bInDebuggerTab) override;

	virtual void GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch) override;

	virtual TSharedRef<SWidget> GenerateNameWidget(TSharedPtr<FString> InSearchString) override;

	virtual FText GetDisplayName() const override;

	virtual FText GetDescription() const override;

	virtual void OnDebuggingChanged() {}

	virtual bool CanCreateNodes() const;

	virtual FString GenerateSearchString() const { return {}; }

	virtual FFieldVariant GetFieldForDefinitionNavigation() const { return {}; }

	void OnFindReferencesClicked() const;

	void NavigateToDefinitionField() const;

	FText GeneratePropertyDisplayValue(const FProperty* Property, void* ValuePtr) const;

	mutable TOptional<TArray<FDebugTreeItemPtr>> CachedChildren;

	mutable TMap<FName, FDebugTreeItemPtr> CachedPropertyItems;

	TWeakPtr<FBlueprintEditor> BlueprintEditorPtr;
	FMDViewModelAssignmentReference Assignment;

	FText DisplayName;
	FText Description;

	TWeakObjectPtr<UBlueprint> BlueprintPtr;

	bool bIsDebugging = false;
	TWeakObjectPtr<UMDViewModelBase> DebugViewModel;
};

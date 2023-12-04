#pragma once
#include "MDViewModelDebugLineItemBase.h"

class FMDVMInspectorDragAndDropCommand;

using FMDVMDragAndDropCreatorFunc = TDelegate<TSharedRef<FMDVMInspectorDragAndDropActionBase>(TWeakObjectPtr<const UFunction>, const FMDViewModelAssignmentReference&)>;

class FMDViewModelFunctionDebugLineItem : public FMDViewModelDebugLineItemBase
{
public:
	FMDViewModelFunctionDebugLineItem(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, const FMDViewModelAssignmentReference& Assignment, const UFunction* Function, const FMDVMDragAndDropCreatorFunc& DragAndDropCreator = {}, bool bIsFieldNotify = false);
	virtual ~FMDViewModelFunctionDebugLineItem() override;

	virtual bool Compare(const FDebugLineItem* BaseOther) const override
	{
		const FMDViewModelFunctionDebugLineItem* Other = static_cast<const FMDViewModelFunctionDebugLineItem*>(BaseOther);
		return FMDViewModelDebugLineItemBase::Compare(BaseOther) && FunctionPtr == Other->FunctionPtr;
	}

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
	virtual uint32 GetHash() const override
#else
	virtual uint32 GetHash() override
#endif
	{
		return HashCombine(GetTypeHash(FunctionPtr), FMDViewModelDebugLineItemBase::GetHash());
	}

	virtual bool CanHaveChildren() override { return true; }

	virtual TSharedRef<SWidget> GetNameIcon() override;

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

	virtual TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateDragAndDropAction() const override;

	TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateBindingDragAndDropAction() const;

	const UFunction* GetFunction() const { return FunctionPtr.Get(); }

protected:
	virtual void UpdateCachedChildren() const override;

	virtual FDebugLineItem* Duplicate() const override;

	virtual bool CanCreateNodes() const override;

	virtual FString GenerateSearchString() const override;

	virtual FFieldVariant GetFieldForDefinitionNavigation() const override;

	virtual FName GetTypeName() const override { return TEXT("Function"); }

	virtual FText GetDisplayValue() const;

	virtual bool CanDisplayReturnValue() const;

	virtual void OnDebuggingChanged() override;

	virtual bool CanDrag() const override;

private:
	int32 GetShouldDisplayFieldNotifyIndex() const;
	FReply OnAddOrViewBoundFieldNotifyFunctionClicked() const;
	bool CanAddBoundFieldNotifyFunction() const;
	int32 GetAddOrViewBoundFieldNotifyFunctionIndex() const;

	void TryUpdateGetterReturnValue() const;
	void CleanUpGetterReturnValue() const;

	TWeakObjectPtr<const UFunction> FunctionPtr;
	FMDVMDragAndDropCreatorFunc DragAndDropCreator;

	bool bIsFieldNotify = false;
	mutable void* GetterReturnValuePtr = nullptr;
};


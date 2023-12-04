#pragma once

#include "MDViewModelDebugLineItemBase.h"
#include "UObject/WeakFieldPtr.h"

class FMDViewModelFieldDebugLineItem : public FMDViewModelDebugLineItemBase
{
public:
	FMDViewModelFieldDebugLineItem(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, const FMDViewModelAssignmentReference& Assignment, const FProperty* Property, void* InValuePtr, bool bIsFieldNotify = false);

	virtual bool Compare(const FDebugLineItem* BaseOther) const override
	{
		const FMDViewModelFieldDebugLineItem* Other = static_cast<const FMDViewModelFieldDebugLineItem*>(BaseOther);
		return FMDViewModelDebugLineItemBase::Compare(BaseOther) && PropertyPtr.Get() == Other->PropertyPtr.Get() && ValuePtr == Other->ValuePtr;
	}

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
	virtual uint32 GetHash() const override
#else
	virtual uint32 GetHash() override
#endif
	{
		return HashCombine(FMDViewModelDebugLineItemBase::GetHash(), HashCombine(GetTypeHash(PropertyPtr), GetTypeHash(ValuePtr)));
	}

	virtual bool CanHaveChildren() override { return true; }

	virtual TSharedRef<SWidget> GetNameIcon() override;

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

	virtual TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateDragAndDropAction() const override;

	TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateBindingDragAndDropAction() const;

	FText GetDisplayValue() const;

	const FProperty* GetProperty() const { return PropertyPtr.Get(); }
	void* GetValuePtr() const { return ValuePtr; }

	void UpdateValuePtr(void* InValuePtr);

	virtual bool CanDrag() const override;

protected:
	virtual void UpdateCachedChildren() const override;

	virtual FDebugLineItem* Duplicate() const override;

	virtual bool CanCreateNodes() const override;

	virtual FString GenerateSearchString() const override;

	virtual FFieldVariant GetFieldForDefinitionNavigation() const override;

	virtual FName GetTypeName() const override { return TEXT("Field"); }

private:
	int32 GetShouldDisplayFieldNotifyIndex() const;
	FReply OnAddOrViewBoundFunctionClicked() const;
	bool CanAddBoundFunction() const;
	int32 GetAddOrViewBoundFunctionIndex() const;

	TWeakFieldPtr<const FProperty> PropertyPtr;
	void* ValuePtr = nullptr;

	bool bIsFieldNotify = false;
};

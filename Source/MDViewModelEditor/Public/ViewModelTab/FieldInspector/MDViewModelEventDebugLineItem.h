#pragma once

#include "MDViewModelFunctionDebugLineItem.h"
#include "UObject/WeakFieldPtr.h"

class FMDViewModelEventDebugLineItem : public FMDViewModelFunctionDebugLineItem
{
public:
	FMDViewModelEventDebugLineItem(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, const FMDViewModelAssignmentReference& Assignment, const FMulticastDelegateProperty* Prop);

	virtual bool Compare(const FDebugLineItem* BaseOther) const override
	{
		const FMDViewModelEventDebugLineItem* Other = static_cast<const FMDViewModelEventDebugLineItem*>(BaseOther);
		return FMDViewModelFunctionDebugLineItem::Compare(BaseOther) && WeakDelegateProp.Get() == Other->WeakDelegateProp.Get();
	}

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
	virtual uint32 GetHash() const override
#else
	virtual uint32 GetHash() override
#endif
	{
		return HashCombine(GetTypeHash(WeakDelegateProp), FMDViewModelFunctionDebugLineItem::GetHash());
	}

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

protected:
	virtual FDebugLineItem* Duplicate() const override;

	virtual bool CanCreateNodes() const override;

	virtual FString GenerateSearchString() const override;

	virtual FFieldVariant GetFieldForDefinitionNavigation() const override;

	virtual FName GetTypeName() const override { return TEXT("Event"); }

	virtual bool CanDisplayReturnValue() const override { return false; }

	virtual bool CanDrag() const override;

	virtual TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateDragAndDropAction() const override;

private:
	FReply OnAddOrViewBoundFunctionClicked() const;
	int32 GetAddOrViewBoundFunctionIndex() const;

	TWeakFieldPtr<const FMulticastDelegateProperty> WeakDelegateProp;
};

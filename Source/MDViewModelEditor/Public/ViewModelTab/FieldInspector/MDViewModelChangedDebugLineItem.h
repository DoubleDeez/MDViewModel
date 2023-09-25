#pragma once

#include "MDViewModelDebugLineItemBase.h"

class FMDViewModelChangedDebugLineItem : public FMDViewModelDebugLineItemBase
{
public:
	FMDViewModelChangedDebugLineItem(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, const FMDViewModelAssignmentReference& Assignment);

	virtual bool Compare(const FDebugLineItem* BaseOther) const override;

	virtual bool CanHaveChildren() override { return false; }

	virtual bool HasChildren() const override { return false; }

	virtual TSharedRef<SWidget> GetNameIcon() override;

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

protected:
	virtual FDebugLineItem* Duplicate() const override;

	virtual bool CanCreateNodes() const override;

	virtual FName GetTypeName() const override { return TEXT("Changed"); }

private:
	FReply OnAddOrViewBoundVMChangedFunctionClicked() const;
	int32 GetAddOrViewBoundVMChangedFunctionIndex() const;
};

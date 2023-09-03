#pragma once

#include "MDViewModelDebugLineItemBase.h"

class FMDViewModelChangedDebugLineItem : public FMDViewModelDebugLineItemBase
{
public:
	FMDViewModelChangedDebugLineItem(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None)
		: FMDViewModelDebugLineItemBase(INVTEXT("On View Model Changed"), INVTEXT("Bind to when this view model is changed"), nullptr, BlueprintEditorPtr, false, ViewModelClass, ViewModelName)
	{
	}

	virtual bool Compare(const FDebugLineItem* BaseOther) const override;

	virtual bool CanHaveChildren() override { return false; }

	virtual bool HasChildren() const override { return false; }

	virtual TSharedRef<SWidget> GetNameIcon() override;

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

protected:
	virtual FDebugLineItem* Duplicate() const override;

	virtual bool CanCreateNodes() const override;

private:
	FReply OnAddOrViewBoundVMChangedFunctionClicked() const;
	int32 GetAddOrViewBoundVMChangedFunctionIndex() const;
};

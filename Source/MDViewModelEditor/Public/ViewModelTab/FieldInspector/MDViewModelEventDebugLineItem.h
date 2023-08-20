#pragma once

#include "MDViewModelFunctionDebugLineItem.h"
#include "UObject/WeakFieldPtr.h"

class FMDViewModelEventDebugLineItem : public FMDViewModelFunctionDebugLineItem
{
public:
	FMDViewModelEventDebugLineItem(const FMulticastDelegateProperty* Prop, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, bool bIsFieldNotify = false, UWidgetBlueprint* WidgetBP = nullptr, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None)
		: FMDViewModelFunctionDebugLineItem(Prop->SignatureFunction, Prop->GetDisplayNameText(), Prop->GetToolTipText(), DebugViewModel, false, false, bIsFieldNotify, WidgetBP, ViewModelClass, ViewModelName)
		, WeakDelegateProp(Prop)
	{
	}

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

protected:
	virtual FDebugLineItem* Duplicate() const override;

private:
	FReply OnAddOrViewBoundFunctionClicked() const;
	int32 GetAddOrViewBoundFunctionIndex() const;

	TWeakFieldPtr<const FMulticastDelegateProperty> WeakDelegateProp;
};

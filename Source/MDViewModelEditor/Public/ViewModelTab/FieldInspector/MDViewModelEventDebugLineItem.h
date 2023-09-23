#pragma once

#include "MDViewModelFunctionDebugLineItem.h"
#include "UObject/WeakFieldPtr.h"

class FMDViewModelEventDebugLineItem : public FMDViewModelFunctionDebugLineItem
{
public:
	FMDViewModelEventDebugLineItem(const FMulticastDelegateProperty* Prop, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, bool bIsFieldNotify = false, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None)
		: FMDViewModelFunctionDebugLineItem(Prop->SignatureFunction, Prop->GetDisplayNameText(), Prop->GetToolTipText(), DebugViewModel, BlueprintEditorPtr, nullptr, bIsFieldNotify, ViewModelClass, ViewModelName)
		, WeakDelegateProp(Prop)
	{
	}

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

protected:
	virtual FDebugLineItem* Duplicate() const override;

	virtual bool CanCreateNodes() const override;

	virtual FString GenerateSearchString() const override;

	virtual FFieldVariant GetFieldForDefinitionNavigation() const override;

private:
	FReply OnAddOrViewBoundFunctionClicked() const;
	int32 GetAddOrViewBoundFunctionIndex() const;

	TWeakFieldPtr<const FMulticastDelegateProperty> WeakDelegateProp;
};

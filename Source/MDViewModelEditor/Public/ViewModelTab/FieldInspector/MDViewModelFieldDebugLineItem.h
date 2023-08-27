#pragma once

#include "MDViewModelDebugLineItemBase.h"
#include "UObject/WeakFieldPtr.h"

class FMDViewModelFieldDebugLineItem : public FMDViewModelDebugLineItemBase
{
public:
	FMDViewModelFieldDebugLineItem(const FProperty* Property, void* InValuePtr, const FText& DisplayName, const FText& Description, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, bool bIsFieldNotify = false, UWidgetBlueprint* WidgetBP = nullptr, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None)
		: FMDViewModelDebugLineItemBase(DisplayName, Description, DebugViewModel, bIsFieldNotify, WidgetBP, ViewModelClass, ViewModelName)
		, PropertyPtr(Property)
		, ValuePtr(InValuePtr)
	{
	}

	virtual bool Compare(const FDebugLineItem* BaseOther) const override;

	virtual uint32 GetHash() override
	{
		return HashCombine(GetTypeHash(PropertyPtr), GetTypeHash(ValuePtr));
	}

	virtual bool CanHaveChildren() override { return true; }

	virtual TSharedRef<SWidget> GetNameIcon() override;

	virtual TSharedRef<SWidget> GenerateNameWidget(TSharedPtr<FString> InSearchString) override;
	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

	virtual TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateDragAndDropAction() const override;

	FText GetDisplayValue() const;

	void* GetValuePtr() const { return ValuePtr; }

	void UpdateValuePtr(void* InValuePtr);

protected:
	virtual void UpdateCachedChildren() const override;

	virtual FDebugLineItem* Duplicate() const override;

	virtual bool CanCreateNodes() const override;

private:
	int32 GetShouldDisplayFieldNotifyIndex() const;
	FReply OnAddOrViewBoundFunctionClicked() const;
	int32 GetAddOrViewBoundFunctionIndex() const;

	TWeakFieldPtr<const FProperty> PropertyPtr;
	void* ValuePtr = nullptr;
};

#pragma once
#include "MDViewModelDebugLineItemBase.h"

class FMDViewModelFunctionDebugLineItem : public FMDViewModelDebugLineItemBase
{
public:
	FMDViewModelFunctionDebugLineItem(const UFunction* Function, const FText& DisplayName, const FText& Description, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, bool bIsCommand, bool bIsGetter, bool bIsFieldNotify = false, UWidgetBlueprint* WidgetBP = nullptr, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None)
		: FMDViewModelDebugLineItemBase(DisplayName, Description, DebugViewModel, bIsFieldNotify, WidgetBP, ViewModelClass, ViewModelName)
		, FunctionPtr(Function)
		, bIsCommand(bIsCommand)
		, bIsGetter(bIsGetter)
	{
	}

	virtual bool Compare(const FDebugLineItem* BaseOther) const override
	{
		const FMDViewModelFunctionDebugLineItem* Other = static_cast<const FMDViewModelFunctionDebugLineItem*>(BaseOther);
		return FunctionPtr == Other->FunctionPtr;
	}

	virtual uint32 GetHash() override
	{
		return GetTypeHash(FunctionPtr);
	}

	virtual bool CanHaveChildren() override { return true; }

	virtual TSharedRef<SWidget> GetNameIcon() override;

	virtual TSharedRef<SWidget> GenerateNameWidget(TSharedPtr<FString> InSearchString) override;

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

	virtual TSharedRef<FMDVMInspectorDragAndDropActionBase> CreateDragAndDropAction() const override;

	void UpdateIsDebugging(bool InIsDebugging);
	
	const UFunction* GetFunction() const { return FunctionPtr.Get(); }

protected:
	virtual void UpdateCachedChildren() const override;

	virtual FDebugLineItem* Duplicate() const override;

	virtual bool CanCreateNodes() const override;

private:
	int32 GetShouldDisplayFieldNotifyIndex() const;
	FReply OnAddOrViewBoundFieldNotifyFunctionClicked() const;
	int32 GetAddOrViewBoundFieldNotifyFunctionIndex() const;

	TWeakObjectPtr<const UFunction> FunctionPtr;
	bool bIsDebugging = false;
	bool bIsCommand = false;
	bool bIsGetter = false;
};


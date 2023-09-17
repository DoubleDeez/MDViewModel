#pragma once
#include "MDViewModelDebugLineItemBase.h"

class FMDViewModelFunctionDebugLineItem : public FMDViewModelDebugLineItemBase
{
public:
	FMDViewModelFunctionDebugLineItem(const UFunction* Function, const FText& DisplayName, const FText& Description, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, bool bIsCommand, bool bIsGetter, bool bIsFieldNotify = false, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None)
		: FMDViewModelDebugLineItemBase(DisplayName, Description, DebugViewModel, BlueprintEditorPtr, bIsFieldNotify, ViewModelClass, ViewModelName)
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

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
	virtual uint32 GetHash() const override
#else
	virtual uint32 GetHash() override
#endif
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

	virtual FString GenerateSearchString() const override;

	virtual FFieldVariant GetFieldForDefinitionNavigation() const override;

private:
	int32 GetShouldDisplayFieldNotifyIndex() const;
	FReply OnAddOrViewBoundFieldNotifyFunctionClicked() const;
	int32 GetAddOrViewBoundFieldNotifyFunctionIndex() const;

	TWeakObjectPtr<const UFunction> FunctionPtr;
	bool bIsDebugging = false;
	bool bIsCommand = false;
	bool bIsGetter = false;
};


#pragma once

#include "SPinValueInspector.h"
#include "Templates/SubclassOf.h"
#include "UObject/UnrealType.h"
#include "UObject/WeakFieldPtr.h"

class UMDVMNode_ViewModelEvent;
class UWidgetBlueprint;
class UMDViewModelBase;

class FMDViewModelDebugLineItemBase : public FDebugLineItem
{
public:
	FMDViewModelDebugLineItemBase(const FText& DisplayName, const FText& Description, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, bool bIsFieldNotify = false, UWidgetBlueprint* WidgetBP = nullptr, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None)
		: FDebugLineItem(DLT_Watch)
		, DisplayName(DisplayName)
		, Description(Description)
		, bIsFieldNotify(bIsFieldNotify)
		, WidgetBP(WidgetBP)
		, ViewModelClass(ViewModelClass)
		, DebugViewModel(DebugViewModel)
		, ViewModelName(ViewModelName)
	{
	}

	// TODO - Add DebugViewModel
	void UpdateViewModel(const FName& InViewModelName, TSubclassOf<UMDViewModelBase> InViewModelClass);

	virtual uint32 GetHash() override
	{
		return HashCombine(GetTypeHash(ViewModelName), GetTypeHash(ViewModelClass));
	}

protected:
	virtual void UpdateCachedChildren() const {};

	virtual bool HasChildren() const override;

	virtual void GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch) override;

	virtual TSharedRef<SWidget> GenerateNameWidget(TSharedPtr<FString> InSearchString) override;

	virtual FText GetDisplayName() const override;

	virtual FText GetDescription() const override;

	mutable TOptional<TArray<FDebugTreeItemPtr>> CachedChildren;

	mutable TMap<FName, FDebugTreeItemPtr> CachedPropertyItems;

	FText DisplayName;
	FText Description;
	bool bIsFieldNotify = false;
	TWeakObjectPtr<UWidgetBlueprint> WidgetBP;
	TSubclassOf<UMDViewModelBase> ViewModelClass;
	TWeakObjectPtr<UMDViewModelBase> DebugViewModel;
	FName ViewModelName = NAME_None;
};

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

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

	FText GetDisplayValue() const;

	void* GetValuePtr() const { return ValuePtr; }

	void UpdateValuePtr(void* InValuePtr);

protected:
	virtual void UpdateCachedChildren() const override;

	virtual FDebugLineItem* Duplicate() const override
	{
		return new FMDViewModelFieldDebugLineItem(PropertyPtr.Get(), ValuePtr, DisplayName, Description, DebugViewModel, bIsFieldNotify, WidgetBP.Get(), ViewModelClass, ViewModelName);
	}

private:
	int32 GetShouldDisplayFieldNotifyIndex() const;
	FReply OnAddOrViewBoundFunctionClicked() const;
	int32 GetAddOrViewBoundFunctionIndex() const;

	TWeakFieldPtr<const FProperty> PropertyPtr;
	void* ValuePtr = nullptr;
};

class FMDViewModelFunctionDebugLineItem : public FMDViewModelDebugLineItemBase
{
public:
	FMDViewModelFunctionDebugLineItem(const UFunction* Function, const FText& DisplayName, const FText& Description, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, bool bIsFieldNotify = false, UWidgetBlueprint* WidgetBP = nullptr, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None)
		: FMDViewModelDebugLineItemBase(DisplayName, Description, DebugViewModel, bIsFieldNotify, WidgetBP, ViewModelClass, ViewModelName)
		, FunctionPtr(Function)
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

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

	void UpdateIsDebugging(bool InIsDebugging);

protected:
	virtual void UpdateCachedChildren() const override;

	virtual FDebugLineItem* Duplicate() const override
	{
		return new FMDViewModelFunctionDebugLineItem(FunctionPtr.Get(), DisplayName, Description, DebugViewModel, bIsFieldNotify, WidgetBP.Get(), ViewModelClass, ViewModelName);
	}

private:
	int32 GetShouldDisplayFieldNotifyIndex() const;
	FReply OnAddOrViewBoundFieldNotifyFunctionClicked() const;
	int32 GetAddOrViewBoundFieldNotifyFunctionIndex() const;

	TWeakObjectPtr<const UFunction> FunctionPtr;
	bool bIsDebugging = false;
};

class FMDViewModelChangedDebugLineItem : public FMDViewModelDebugLineItemBase
{
public:
	FMDViewModelChangedDebugLineItem(UWidgetBlueprint* WidgetBP = nullptr, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None)
		: FMDViewModelDebugLineItemBase(INVTEXT("On View Model Changed"), INVTEXT("Bind to when this view model is changed"), nullptr, false, WidgetBP, ViewModelClass, ViewModelName)
	{
	}

	virtual bool Compare(const FDebugLineItem* BaseOther) const override;

	virtual bool CanHaveChildren() override { return false; }

	virtual bool HasChildren() const override { return false; }

	virtual TSharedRef<SWidget> GetNameIcon() override;

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

protected:
	virtual FDebugLineItem* Duplicate() const override
	{
		return new FMDViewModelChangedDebugLineItem(WidgetBP.Get(), ViewModelClass, ViewModelName);
	}

private:
	FReply OnAddOrViewBoundVMChangedFunctionClicked() const;
	int32 GetAddOrViewBoundVMChangedFunctionIndex() const;
};

class FMDViewModelEventDebugLineItem : public FMDViewModelFunctionDebugLineItem
{
public:
	FMDViewModelEventDebugLineItem(const FMulticastDelegateProperty* Prop, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, bool bIsFieldNotify = false, UWidgetBlueprint* WidgetBP = nullptr, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FName& ViewModelName = NAME_None)
		: FMDViewModelFunctionDebugLineItem(Prop->SignatureFunction, Prop->GetDisplayNameText(), Prop->GetToolTipText(), DebugViewModel, bIsFieldNotify, WidgetBP, ViewModelClass, ViewModelName)
		, WeakDelegateProp(Prop)
	{
	}

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

protected:
	virtual FDebugLineItem* Duplicate() const override
	{
		return new FMDViewModelEventDebugLineItem(WeakDelegateProp.Get(), DebugViewModel, bIsFieldNotify, WidgetBP.Get(), ViewModelClass, ViewModelName);
	}

private:
	FReply OnAddOrViewBoundFunctionClicked() const;
	int32 GetAddOrViewBoundFunctionIndex() const;

	TWeakFieldPtr<const FMulticastDelegateProperty> WeakDelegateProp;
};

/**
 * Widget that displays all the exposed properties of a viewmodel and their values when debugging
 */
class MDVIEWMODELEDITOR_API SMDViewModelFieldInspector : public SPinValueInspector
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelFieldInspector)
	{
	}

		SLATE_ARGUMENT(bool, bIncludeBlueprintVisibleProperties)
		SLATE_ARGUMENT(bool, bIncludeBlueprintAssignableProperties)
		SLATE_ARGUMENT(bool, bIncludeBlueprintCallable)
		SLATE_ARGUMENT(bool, bIncludeBlueprintPure)
		SLATE_ARGUMENT(bool, bIncludeFieldNotifyFunctions)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UWidgetBlueprint* WidgetBP);

	void SetReferences(TSubclassOf<UMDViewModelBase> InViewModelClass, UMDViewModelBase* InDebugViewModel, const FName& InViewModelName);

	void RefreshList();

protected:
	virtual void PopulateTreeView() override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	// We can't support search since that requires inheriting from FLineItemWithChildren but that's private
	virtual EVisibility GetSearchFilterVisibility() const override { return EVisibility::Collapsed; }

private:
	TMap<const FProperty*, TSharedPtr<FMDViewModelFieldDebugLineItem>> PropertyTreeItems;
	TMap<const UFunction*, TSharedPtr<FMDViewModelFunctionDebugLineItem>> FunctionTreeItems;
	TMap<const FMulticastDelegateProperty*, TSharedPtr<FMDViewModelEventDebugLineItem>> EventTreeItems;
	TSharedPtr<FMDViewModelChangedDebugLineItem> VMChangedItem;
	TSubclassOf<UMDViewModelBase> ViewModelClass;
	TWeakObjectPtr<UMDViewModelBase> DebugViewModel;
	bool bIsDebugging = false;
	TWeakObjectPtr<UWidgetBlueprint> WidgetBPPtr;
	FName ViewModelName = NAME_None;

	bool bIncludeBlueprintVisibleProperties = false;
	bool bIncludeBlueprintAssignableProperties = false;
	bool bIncludeBlueprintCallable = false;
	bool bIncludeBlueprintPure = false;
	bool bIncludeFieldNotifyFunctions = false;
};

#pragma once

#include "CoreMinimal.h"
#include "SPinValueInspector.h"
#include "Templates/SubclassOf.h"
#include "UObject/UnrealType.h"
#include "UObject/WeakFieldPtr.h"

class UMDViewModelBase;

class FMDViewModelFieldDebugLineItem : public FDebugLineItem
{
public:
	FMDViewModelFieldDebugLineItem(const FProperty* Property, void* InValuePtr, const FText& InDisplayNameOverride = FText::GetEmpty())
		: FDebugLineItem(EDebugLineType::DLT_Watch)
		, PropertyPtr(Property)
		, ValuePtr(InValuePtr)
		, DisplayNameOverride(InDisplayNameOverride)
	{
	}

	virtual bool Compare(const FDebugLineItem* BaseOther) const override
	{
		const FMDViewModelFieldDebugLineItem* Other = static_cast<const FMDViewModelFieldDebugLineItem*>(BaseOther);
		return GetPropertyInstance() == Other->GetPropertyInstance();
	}

	virtual uint32 GetHash() override
	{
		const TTuple<const FProperty*, void*> Instance = GetPropertyInstance();
		return HashCombine(GetTypeHash(Instance.Key), GetTypeHash(Instance.Value));
	}

	virtual bool CanHaveChildren() override { return true; }

	virtual bool HasChildren() const override;

	virtual void GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch) override;

	virtual TSharedRef<SWidget> GetNameIcon() override;

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;


	virtual FText GetDisplayName() const override
	{
		if (!DisplayNameOverride.IsEmptyOrWhitespace())
		{
			return DisplayNameOverride;
		}

		if (const FProperty* Property = PropertyPtr.Get())
		{
			return Property->GetDisplayNameText();
		}

		return INVTEXT("[Invalid]");
	}

	virtual FText GetDescription() const override
	{
		if (const FProperty* Property = PropertyPtr.Get())
		{
			return Property->GetToolTipText();
		}

		return INVTEXT("[Invalid]");
	}

	FText GetDisplayValue() const;

	void UpdateCachedChildren() const;

	TTuple<const FProperty*, void*> GetPropertyInstance() const;

	void* GetValuePtr() const { return ValuePtr; }

	void UpdateValuePtr(void* InValuePtr);

protected:
	virtual FDebugLineItem* Duplicate() const override
	{
		return new FMDViewModelFieldDebugLineItem(PropertyPtr.Get(), ValuePtr, DisplayNameOverride);
	}

	mutable TOptional<TArray<FDebugTreeItemPtr>> CachedChildren;

	mutable TMap<FName, FDebugTreeItemPtr> CachedPropertyItems;

private:
	TWeakFieldPtr<const FProperty> PropertyPtr;
	void* ValuePtr = nullptr;
	FText DisplayNameOverride;
};

/**
 * Widget that displays all the exposed properties of a viewmodel and their values when debugging
 */
class MDVIEWMODELEDITOR_API SMDViewModelFieldInspector : public SPinValueInspector
{
public:
	using SPinValueInspector::Construct;

	void SetReferences(TSubclassOf<UMDViewModelBase> InViewModelClass, UMDViewModelBase* InDebugViewModel);

	void RefreshList();

protected:
	virtual void PopulateTreeView() override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	// We can't support search since that requires inheriting from FLineItemWithChildren but that's private
	virtual EVisibility GetSearchFilterVisibility() const override { return EVisibility::Collapsed; }

private:
	TMap<const FProperty*, TSharedPtr<FMDViewModelFieldDebugLineItem>> TreeItems;
	TSubclassOf<UMDViewModelBase> ViewModelClass;
	TWeakObjectPtr<UMDViewModelBase> DebugViewModel;
	bool bIsDebugging = false;
};

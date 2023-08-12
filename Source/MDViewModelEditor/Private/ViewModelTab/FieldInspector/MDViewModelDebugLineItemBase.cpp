#include "ViewModelTab/FieldInspector/MDViewModelDebugLineItemBase.h"

#include "PropertyInfoViewStyle.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprint.h"

FMDViewModelDebugLineItemBase::FMDViewModelDebugLineItemBase(const FText& DisplayName, const FText& Description, TWeakObjectPtr<UMDViewModelBase> DebugViewModel, bool bIsFieldNotify, UWidgetBlueprint* WidgetBP, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
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

void FMDViewModelDebugLineItemBase::UpdateViewModel(const FName& InViewModelName, TSubclassOf<UMDViewModelBase> InViewModelClass)
{
	ViewModelName = InViewModelName;
	ViewModelClass = InViewModelClass;
}

FMDViewModelAssignmentReference FMDViewModelDebugLineItemBase::GetViewModelAssignmentReference() const
{
	FMDViewModelAssignmentReference Reference;
	Reference.ViewModelClass = ViewModelClass;
	Reference.ViewModelName = ViewModelName;
	return Reference;
}

bool FMDViewModelDebugLineItemBase::HasChildren() const
{
	if (!CachedChildren.IsSet())
	{
		UpdateCachedChildren();
	}

	return CachedChildren.GetValue().Num() > 0;
}

void FMDViewModelDebugLineItemBase::GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch)
{
	if (CanHaveChildren())
	{
		if (!CachedChildren.IsSet())
		{
			UpdateCachedChildren();
		}

		OutChildren.Append(CachedChildren.GetValue());
	}
}

TSharedRef<SWidget> FMDViewModelDebugLineItemBase::GenerateNameWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(PropertyInfoViewStyle::STextHighlightOverlay)
		.FullText(this, &FMDViewModelDebugLineItemBase::GetDisplayName)
		.HighlightText(this, &FDebugLineItem::GetHighlightText, InSearchString)
		[
			SNew(STextBlock)
				.ToolTipText(this, &FDebugLineItem::GetDescription)
				.Text(this, &FMDViewModelDebugLineItemBase::GetDisplayName)
		];
}

FText FMDViewModelDebugLineItemBase::GetDisplayName() const
{
	return DisplayName;
}

FText FMDViewModelDebugLineItemBase::GetDescription() const
{
	return Description;
}

#include "ViewModelTab/FieldInspector/MDViewModelChangedDebugLineItem.h"

#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

bool FMDViewModelChangedDebugLineItem::Compare(const FDebugLineItem* BaseOther) const
{
	const FMDViewModelChangedDebugLineItem* Other = static_cast<const FMDViewModelChangedDebugLineItem*>(BaseOther);
	return ViewModelName == Other->ViewModelName && ViewModelClass == Other->ViewModelClass;
}

TSharedRef<SWidget> FMDViewModelChangedDebugLineItem::GetNameIcon()
{
	return SNew(SImage)
		.Image(FAppStyle::Get().GetBrush(TEXT("GraphEditor.Function_16x")))
		.ColorAndOpacity(FLinearColor::White);
}

TSharedRef<SWidget> FMDViewModelChangedDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SButton)
	.ContentPadding(FMargin(3.0, 2.0))
	.OnClicked(this, &FMDViewModelChangedDebugLineItem::OnAddOrViewBoundVMChangedFunctionClicked)
	.IsEnabled(this, &FMDViewModelChangedDebugLineItem::CanCreateNodes)
	[
		SNew(SWidgetSwitcher)
		.WidgetIndex(this, &FMDViewModelChangedDebugLineItem::GetAddOrViewBoundVMChangedFunctionIndex)
		+SWidgetSwitcher::Slot()
		[
			SNew(SImage)
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Image(FAppStyle::Get().GetBrush("Icons.SelectInViewport"))
			.ToolTipText(INVTEXT("Focus the existing bound function."))
		]
		+SWidgetSwitcher::Slot()
		[
			SNew(SImage)
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
			.ToolTipText(INVTEXT("Create a BP event bound to this view model changing"))
		]
	];
}

FDebugLineItem* FMDViewModelChangedDebugLineItem::Duplicate() const
{
	return new FMDViewModelChangedDebugLineItem(BlueprintEditorPtr, ViewModelClass, ViewModelName);
}

bool FMDViewModelChangedDebugLineItem::CanCreateNodes() const
{
	return FMDViewModelDebugLineItemBase::CanCreateNodes() || GetAddOrViewBoundVMChangedFunctionIndex() == 0;
}

FReply FMDViewModelChangedDebugLineItem::OnAddOrViewBoundVMChangedFunctionClicked() const
{
	FMDViewModelGraphStatics::OnViewModelChangedRequestedForBlueprint(BlueprintPtr.Get(), { ViewModelClass, ViewModelName });
	return FReply::Handled();
}

int32 FMDViewModelChangedDebugLineItem::GetAddOrViewBoundVMChangedFunctionIndex() const
{
	return FMDViewModelGraphStatics::DoesBlueprintBindToViewModelChanged(BlueprintPtr.Get(), { ViewModelClass, ViewModelName })
		? 0 : 1;
}

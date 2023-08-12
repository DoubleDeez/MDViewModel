#include "ViewModelTab/FieldInspector/MDViewModelEventDebugLineItem.h"

#include "Util/MDViewModelGraphStatics.h"
#include "WidgetBlueprint.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

TSharedRef<SWidget> FMDViewModelEventDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SButton)
	.ContentPadding(FMargin(3.0, 2.0))
	.OnClicked(this, &FMDViewModelEventDebugLineItem::OnAddOrViewBoundFunctionClicked)
	[
		SNew(SWidgetSwitcher)
		.WidgetIndex(this, &FMDViewModelEventDebugLineItem::GetAddOrViewBoundFunctionIndex)
		+ SWidgetSwitcher::Slot()
		[
			SNew(SImage)
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Image(FAppStyle::Get().GetBrush("Icons.SelectInViewport"))
			.ToolTipText(INVTEXT("Focus the existing bound function."))
		]
		+ SWidgetSwitcher::Slot()
		[
			SNew(SImage)
			.ColorAndOpacity(FSlateColor::UseForeground())
			.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
			.ToolTipText(INVTEXT("Create a BP event bound to this view model event"))
		]
	];
}

FDebugLineItem* FMDViewModelEventDebugLineItem::Duplicate() const
{
	return new FMDViewModelEventDebugLineItem(WeakDelegateProp.Get(), DebugViewModel, bIsFieldNotify, WidgetBP.Get(), ViewModelClass, ViewModelName);
}

FReply FMDViewModelEventDebugLineItem::OnAddOrViewBoundFunctionClicked() const
{
	if (WeakDelegateProp.IsValid())
	{
		FMDViewModelGraphStatics::OnViewModelEventRequestedForBlueprint(WidgetBP.Get(), WeakDelegateProp->GetFName(), ViewModelClass, ViewModelName);
	}

	return FReply::Handled();
}

int32 FMDViewModelEventDebugLineItem::GetAddOrViewBoundFunctionIndex() const
{
	return (!WeakDelegateProp.IsValid() || FMDViewModelGraphStatics::DoesBlueprintBindToViewModelEvent(WidgetBP.Get(), WeakDelegateProp->GetFName(), ViewModelClass, ViewModelName))
		? 0 : 1;
}

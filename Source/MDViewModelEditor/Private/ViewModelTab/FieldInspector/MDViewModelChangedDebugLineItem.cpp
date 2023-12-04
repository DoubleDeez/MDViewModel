#include "ViewModelTab/FieldInspector/MDViewModelChangedDebugLineItem.h"

#include "Util/MDViewModelGraphStatics.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMDragAndDropWrapperButton.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropChanged.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

FMDViewModelChangedDebugLineItem::FMDViewModelChangedDebugLineItem(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, const FMDViewModelAssignmentReference& Assignment)
	: FMDViewModelDebugLineItemBase(BlueprintEditorPtr, Assignment)
{
	SetDisplayText(INVTEXT("On View Model Changed"), INVTEXT("Bind to when this view model is changed"));
}

bool FMDViewModelChangedDebugLineItem::Compare(const FDebugLineItem* BaseOther) const
{
	const FMDViewModelChangedDebugLineItem* Other = static_cast<const FMDViewModelChangedDebugLineItem*>(BaseOther);
	return GetTypeName() == Other->GetTypeName() && Assignment == Other->Assignment;
}

TSharedRef<SWidget> FMDViewModelChangedDebugLineItem::GetNameIcon()
{
	return SNew(SImage)
		.Image(FAppStyle::Get().GetBrush(TEXT("GraphEditor.Function_16x")))
		.ColorAndOpacity(FLinearColor::White);
}

TSharedRef<SWidget> FMDViewModelChangedDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SMDVMDragAndDropWrapperButton)
		.OnGetDragAndDropAction(this, &FMDViewModelChangedDebugLineItem::CreateDragAndDropAction)
		.bCanDrag(this, &FMDViewModelChangedDebugLineItem::CanDrag)
		.ButtonArguments(
			SButton::FArguments()
			.ContentPadding(FMargin(3.0, 2.0))
			.OnClicked(this, &FMDViewModelChangedDebugLineItem::OnAddOrViewBoundVMChangedFunctionClicked)
			.IsEnabled(this, &FMDViewModelChangedDebugLineItem::CanCreateNodes)
		)
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

bool FMDViewModelChangedDebugLineItem::CanDrag() const
{
	return !FMDViewModelGraphStatics::DoesBlueprintBindToViewModelChanged(BlueprintPtr.Get(), Assignment);
}

TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDViewModelChangedDebugLineItem::CreateDragAndDropAction() const
{
	check(CanDrag());

	return FMDVMInspectorDragAndDropChanged::Create(GetViewModelAssignmentReference());
}

FDebugLineItem* FMDViewModelChangedDebugLineItem::Duplicate() const
{
	return new FMDViewModelChangedDebugLineItem(BlueprintEditorPtr, Assignment);
}

bool FMDViewModelChangedDebugLineItem::CanCreateNodes() const
{
	return FMDViewModelDebugLineItemBase::CanCreateNodes() || GetAddOrViewBoundVMChangedFunctionIndex() == 0;
}

FReply FMDViewModelChangedDebugLineItem::OnAddOrViewBoundVMChangedFunctionClicked() const
{
	FMDViewModelGraphStatics::OnViewModelChangedRequestedForBlueprint(BlueprintPtr.Get(), Assignment);
	return FReply::Handled();
}

int32 FMDViewModelChangedDebugLineItem::GetAddOrViewBoundVMChangedFunctionIndex() const
{
	return FMDViewModelGraphStatics::DoesBlueprintBindToViewModelChanged(BlueprintPtr.Get(), Assignment)
		? 0 : 1;
}

#include "ViewModelTab/FieldInspector/MDViewModelEventDebugLineItem.h"

#include "Nodes/MDVMNode_ViewModelEvent.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropEvent.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

FMDViewModelEventDebugLineItem::FMDViewModelEventDebugLineItem(const TWeakPtr<FBlueprintEditor>& BlueprintEditorPtr, const FMDViewModelAssignmentReference& Assignment, const FMulticastDelegateProperty* Prop)
	: FMDViewModelFunctionDebugLineItem(BlueprintEditorPtr, Assignment, Prop->SignatureFunction)
	, WeakDelegateProp(Prop)
{
	SetDisplayText(Prop->GetDisplayNameText(), Prop->GetToolTipText());
}

TSharedRef<SWidget> FMDViewModelEventDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(SButton)
	.ContentPadding(FMargin(3.0, 2.0))
	.OnClicked(this, &FMDViewModelEventDebugLineItem::OnAddOrViewBoundFunctionClicked)
	.IsEnabled(this, &FMDViewModelEventDebugLineItem::CanCreateNodes)
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
	FMDViewModelEventDebugLineItem* Item = new FMDViewModelEventDebugLineItem(BlueprintEditorPtr, Assignment, WeakDelegateProp.Get());
	Item->SetDisplayText(DisplayName, Description);
	Item->UpdateDebugging(bIsDebugging, DebugViewModel);

	return Item;
}

bool FMDViewModelEventDebugLineItem::CanCreateNodes() const
{
	return FMDViewModelDebugLineItemBase::CanCreateNodes() || GetAddOrViewBoundFunctionIndex() == 0;
}

FString FMDViewModelEventDebugLineItem::GenerateSearchString() const
{
	FString Result;

	if (WeakDelegateProp.IsValid())
	{
		const UMDVMNode_ViewModelEvent* Node = FMDViewModelGraphStatics::FindExistingViewModelEventNode(BlueprintPtr.Get(), WeakDelegateProp->GetFName(), Assignment);
		if (IsValid(Node))
		{
			Result += Node->GetFindReferenceSearchString();
		}
	}

	return Result;
}

FFieldVariant FMDViewModelEventDebugLineItem::GetFieldForDefinitionNavigation() const
{
	return FFieldVariant(WeakDelegateProp.Get());
}

bool FMDViewModelEventDebugLineItem::CanDrag() const
{
	return WeakDelegateProp.IsValid() && !FMDViewModelGraphStatics::DoesBlueprintBindToViewModelEvent(BlueprintPtr.Get(), WeakDelegateProp->GetFName(), Assignment);
}

TSharedRef<FMDVMInspectorDragAndDropActionBase> FMDViewModelEventDebugLineItem::CreateDragAndDropAction() const
{
	check(CanDrag());

	return FMDVMInspectorDragAndDropEvent::Create(
		WeakDelegateProp,
		GetViewModelAssignmentReference()
	);
}

FReply FMDViewModelEventDebugLineItem::OnAddOrViewBoundFunctionClicked() const
{
	if (WeakDelegateProp.IsValid())
	{
		FMDViewModelGraphStatics::OnViewModelEventRequestedForBlueprint(BlueprintPtr.Get(), WeakDelegateProp->GetFName(), Assignment);
	}

	return FReply::Handled();
}

int32 FMDViewModelEventDebugLineItem::GetAddOrViewBoundFunctionIndex() const
{
	return (!WeakDelegateProp.IsValid() || FMDViewModelGraphStatics::DoesBlueprintBindToViewModelEvent(BlueprintPtr.Get(), WeakDelegateProp->GetFName(), Assignment))
		? 0 : 1;
}

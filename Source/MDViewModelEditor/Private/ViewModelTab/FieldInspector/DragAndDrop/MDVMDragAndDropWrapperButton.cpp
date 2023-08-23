#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMDragAndDropWrapperButton.h"

#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropActionBase.h"
#include "ViewModelTab/FieldInspector/MDViewModelDebugLineItemBase.h"

void SMDVMDragAndDropWrapperButton::Construct(const FArguments& InArgs, TSharedRef<FMDViewModelDebugLineItemBase> Parent)
{
	bCanDrag = InArgs._bCanDrag;
	LineItem = Parent;

	ButtonStyle.Normal = FSlateBrush(FSlateNoResource());
	ButtonStyle.Pressed = FSlateBrush(FSlateNoResource());
	ButtonStyle.Hovered = FSlateBrush(FSlateNoResource());
	ButtonStyle.Disabled = FSlateBrush(FSlateNoResource());
	ButtonStyle.NormalPadding = FMargin(0);
	ButtonStyle.PressedPadding = FMargin(0);

	SButton::Construct(
		SButton::FArguments()
		.ButtonStyle(&ButtonStyle)
		.Content()
		[
			InArgs._Content.Widget
		]
	);
}

TOptional<EMouseCursor::Type> SMDVMDragAndDropWrapperButton::GetCursor() const
{
	return (bCanDrag.Get(false) && LineItem.IsValid()) ? EMouseCursor::GrabHand : SCompoundWidget::GetCursor();
}

FReply SMDVMDragAndDropWrapperButton::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = SButton::OnMouseButtonDown(MyGeometry, MouseEvent);
	if (bCanDrag.Get(false) && LineItem.IsValid() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Reply = Reply.DetectDrag(AsShared(), EKeys::LeftMouseButton);
	}

	return Reply;
}

FReply SMDVMDragAndDropWrapperButton::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bCanDrag.Get(false) && LineItem.IsValid())
	{
		return FReply::Handled().BeginDragDrop(LineItem->CreateDragAndDropAction());
	}

	return FReply::Unhandled();
}

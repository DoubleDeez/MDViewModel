#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMDragAndDropWrapperButton.h"

#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "ViewModelTab/FieldInspector/DragAndDrop/MDVMInspectorDragAndDropActionBase.h"
#include "ViewModelTab/FieldInspector/MDViewModelDebugLineItemBase.h"

void SMDVMDragAndDropWrapperButton::Construct(const FArguments& InArgs)
{
	bCanDrag = InArgs._bCanDrag;
	OnGetDragAndDropAction = InArgs._OnGetDragAndDropAction;

	SButton::Construct(SButton::FArguments(InArgs._ButtonArguments)
	[
		InArgs._Content.Widget
	]);
}

TOptional<EMouseCursor::Type> SMDVMDragAndDropWrapperButton::GetCursor() const
{
	return CanDrag() ? EMouseCursor::GrabHand : SCompoundWidget::GetCursor();
}

FReply SMDVMDragAndDropWrapperButton::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = SButton::OnMouseButtonDown(MyGeometry, MouseEvent);
	if (CanDrag() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Reply = Reply.DetectDrag(AsShared(), EKeys::LeftMouseButton);
	}

	return Reply;
}

FReply SMDVMDragAndDropWrapperButton::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (CanDrag())
	{
		return FReply::Handled().BeginDragDrop(OnGetDragAndDropAction.Execute());
	}

	return FReply::Unhandled();
}

bool SMDVMDragAndDropWrapperButton::CanDrag() const
{
	return !GEditor->bIsSimulatingInEditor && GEditor->PlayWorld == nullptr && OnGetDragAndDropAction.IsBound() && bCanDrag.Get(false);
}

#pragma once

#include "Widgets/Input/SButton.h"

class FMDViewModelDebugLineItemBase;
class FMDViewModelFunctionDebugLineItem;
class FMDVMInspectorDragAndDropActionBase;

class SMDVMDragAndDropWrapperButton : public SButton
{
	inline static const FButtonStyle DefaultButtonStyle = []()
	{
		FButtonStyle ButtonStyle;
		ButtonStyle.Normal = FSlateBrush(FSlateNoResource());
		ButtonStyle.Pressed = FSlateBrush(FSlateNoResource());
		ButtonStyle.Hovered = FSlateBrush(FSlateNoResource());
		ButtonStyle.Disabled = FSlateBrush(FSlateNoResource());
		ButtonStyle.NormalPadding = FMargin(0);
		ButtonStyle.PressedPadding = FMargin(0);
		return ButtonStyle;
	}();

public:
	DECLARE_DELEGATE_RetVal(TSharedRef<FMDVMInspectorDragAndDropActionBase>, FOnGetDragAndDropAction);

	SLATE_BEGIN_ARGS(SMDVMDragAndDropWrapperButton)
		: _bCanDrag(false)
		, _ButtonArguments(SButton::FArguments().ContentPadding(0).ButtonStyle(&DefaultButtonStyle))
	{}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ATTRIBUTE(bool, bCanDrag)
		SLATE_EVENT(FOnGetDragAndDropAction, OnGetDragAndDropAction)
		SLATE_ARGUMENT(SButton::FArguments, ButtonArguments)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual TOptional<EMouseCursor::Type> GetCursor() const override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	bool CanDrag() const;

	TAttribute<bool> bCanDrag;
	FOnGetDragAndDropAction OnGetDragAndDropAction;
};

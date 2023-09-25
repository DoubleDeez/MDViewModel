#pragma once

#include "Widgets/Input/SButton.h"

class FMDViewModelDebugLineItemBase;
class FMDViewModelFunctionDebugLineItem;

class SMDVMDragAndDropWrapperButton : public SButton
{
public:
	SLATE_BEGIN_ARGS(SMDVMDragAndDropWrapperButton)
	{}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ATTRIBUTE(bool, bCanDrag)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FMDViewModelDebugLineItemBase> Parent);

	virtual TOptional<EMouseCursor::Type> GetCursor() const override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	bool CanDrag() const;

	TAttribute<bool> bCanDrag;
	TSharedPtr<FMDViewModelDebugLineItemBase> LineItem;
	FButtonStyle ButtonStyle;
};

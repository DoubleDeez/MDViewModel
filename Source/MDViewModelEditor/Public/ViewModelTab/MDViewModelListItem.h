#pragma once

#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SCompoundWidget.h"

struct FMDViewModelEditorAssignment;

class SMDViewModelListItem : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDViewModelListItem)
		{
		}

		SLATE_EVENT(FSimpleDelegate, OnEditItemRequested)
		SLATE_EVENT(FSimpleDelegate, OnDeleteItemConfirmed)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FMDViewModelEditorAssignment> Item);

private:
	EVisibility GetButtonVisibility() const;
	EVisibility GetSourceTextVisibility() const;

	FReply OnEditClicked() const;
	FReply OnDeleteClicked() const;

	TSharedPtr<FMDViewModelEditorAssignment> Assignment;

	FSlateBrush BackgroundBrush;
	FButtonStyle ButtonStyle;

	FSimpleDelegate OnEditItemRequested;
	FSimpleDelegate OnDeleteItemConfirmed;
};

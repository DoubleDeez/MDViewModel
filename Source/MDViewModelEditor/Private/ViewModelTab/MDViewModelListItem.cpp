#include "ViewModelTab/MDViewModelListItem.h"

#include "Brushes/SlateColorBrush.h"
#include "DetailLayoutBuilder.h"
#include "Misc/MessageDialog.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"


void SMDViewModelListItem::Construct(const FArguments& InArgs, TSharedPtr<FMDViewModelEditorAssignment> Item)
{
	OnEditItemRequested = InArgs._OnEditItemRequested;
	OnDeleteItemConfirmed = InArgs._OnDeleteItemConfirmed;

	Assignment = Item;

	BackgroundBrush = static_cast<FSlateBrush>(FSlateColorBrush(FLinearColor::Transparent));

	ButtonStyle = FAppStyle::Get().GetWidgetStyle< FButtonStyle >("FlatButton");
	ButtonStyle.NormalPadding = FMargin(2.f);
	ButtonStyle.PressedPadding = FMargin(2.f);

	const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(Assignment->Assignment.ProviderTag);

	const FText SourceText = Item->bIsSuper ? INVTEXT("Super") : FText::GetEmpty();
	const FText SourceToolTipText = Item->bIsSuper
		? INVTEXT("This viewmodel is assigned in a parent blueprint therefore it cannot be edited.")
		: FText::GetEmpty();

	const bool bIsViewModelClassValid = Item->Assignment.ViewModelClass != nullptr;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(5.f)
		.BorderImage(&BackgroundBrush)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(bIsViewModelClassValid ? Item->Assignment.ViewModelClass->GetDisplayNameText() : INVTEXT("NULL"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ToolTipText(bIsViewModelClassValid ? Item->Assignment.ViewModelClass->GetToolTipText() : INVTEXT("This view model is invalid, the class may have been renamed or deleted"))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Top)
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					[
						SNew(SHorizontalBox)
						.Visibility(this, &SMDViewModelListItem::GetButtonVisibility)
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Top)
						[
							SNew(SButton)
							.ButtonStyle(&ButtonStyle)
							.ContentPadding(2.f)
							.OnClicked(this, &SMDViewModelListItem::OnEditClicked)
							.ToolTipText(INVTEXT("Edit this view model assignment"))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
								.Image(FAppStyle::Get().GetBrush("Icons.Edit"))
							]
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Top)
						[
							SNew(SButton)
							.ButtonStyle(&ButtonStyle)
							.ContentPadding(2.f)
							.OnClicked(this, &SMDViewModelListItem::OnDeleteClicked)
							.ToolTipText(INVTEXT("Remove this view model assignment"))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
								.Image(FAppStyle::Get().GetBrush(TEXT("GenericCommands.Delete")))
							]
						]
					]
					+SOverlay::Slot()
					[
						SNew(STextBlock)
						.Visibility(this, &SMDViewModelListItem::GetSourceTextVisibility)
						.Text(SourceText)
						.ToolTipText(SourceToolTipText)
					]
				]
			]
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromName(Item->Assignment.ViewModelName))
					.ToolTipText(INVTEXT("The name of this view model assignment. It must be unique for all view models of the same class on this widget."))
				]
				+SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(SSpacer)
					.Size(FVector2D(10.f, 1.f))
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Font(IDetailLayoutBuilder::GetDetailFontItalic())
					.Text(IsValid(Provider) ? Provider->GetDisplayName() : INVTEXT("Invalid Provider"))
					.ToolTipText(IsValid(Provider) ? Provider->GetDescription() : INVTEXT("The selected provider is not valid, it may have been deleted or is in an unloaded module."))
				]
			]
		]
	];
}

EVisibility SMDViewModelListItem::GetButtonVisibility() const
{
	if (!Assignment->bIsSuper)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}

EVisibility SMDViewModelListItem::GetSourceTextVisibility() const
{
	if (Assignment->bIsSuper)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

FReply SMDViewModelListItem::OnEditClicked() const
{
	OnEditItemRequested.ExecuteIfBound();
	return FReply::Handled();
}

FReply SMDViewModelListItem::OnDeleteClicked() const
{
	const EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgType::YesNo, INVTEXT("Are you sure you want to delete this view model assignment?"));
	if (ReturnType == EAppReturnType::Yes)
	{
		OnDeleteItemConfirmed.ExecuteIfBound();
	}

	return FReply::Handled();
}

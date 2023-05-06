#include "ViewModelTab/MDViewModelListItem.h"

#include "DetailLayoutBuilder.h"
#include "MDViewModelModule.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/VerticalBox.h"
#include "Misc/MessageDialog.h"
#include "Styling/StyleColors.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "ViewModel/MDViewModelBase.h"
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

	const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
	const TSharedPtr<FMDViewModelProviderBase> Provider = ViewModelModule.GetViewModelProvider(Assignment->Assignment.ProviderTag);
	check(Provider.IsValid());

	const FText SourceText = [Item]()
	{
		if (Item->bIsNative && Item->bIsSuper)
		{
			return INVTEXT("Native & Super");
		}

		if (Item->bIsNative)
		{
			return INVTEXT("Native");
		}

		if (Item->bIsSuper)
		{
			return INVTEXT("Super");
		}

		return FText::GetEmpty();
	}();
	const FText SourceToolTipText = [Item]()
	{
		if (Item->bIsNative && Item->bIsSuper)
		{
			return INVTEXT("This viewmodel is assigned natively and in a parent blueprint therefore it cannot be edited.");
		}

		if (Item->bIsNative)
		{
			return INVTEXT("This viewmodel is assigned natively therefore it cannot be edited.");
		}

		if (Item->bIsSuper)
		{
			return INVTEXT("This viewmodel is assigned in a parent blueprint therefore it cannot be edited.");
		}

		return FText::GetEmpty();
	}();

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
					.Text(Item->Assignment.ViewModelClass->GetDisplayNameText())
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
					.ToolTipText(Item->Assignment.ViewModelClass->GetToolTipText())
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
					// TODO - inline editable if non-native
					SNew(STextBlock)
					.Text(FText::FromName(Item->Assignment.ViewModelName))
					.ToolTipText(INVTEXT("The name of this view model instance. It must be unique for all view models of the same class on this widget."))
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
					.Text(Provider->GetDisplayName())
					.ToolTipText(Provider->GetDescription())
				]
			]
		]
	];
}

EVisibility SMDViewModelListItem::GetButtonVisibility() const
{
	if (IsHovered() && !Assignment->bIsNative && !Assignment->bIsSuper)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Hidden;
}

EVisibility SMDViewModelListItem::GetSourceTextVisibility() const
{
	if (/*IsHovered() &&*/ (Assignment->bIsNative || Assignment->bIsSuper))
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

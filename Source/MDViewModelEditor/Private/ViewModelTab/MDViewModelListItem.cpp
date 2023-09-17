#include "ViewModelTab/MDViewModelListItem.h"

#include "BlueprintEditor.h"
#include "BlueprintModes/WidgetBlueprintApplicationModes.h"
#include "Brushes/SlateColorBrush.h"
#include "DetailLayoutBuilder.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Nodes/MDVMNode_GetViewModel.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"
#include "WidgetBlueprint.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"


TSharedRef<FMDVMDragAndDropViewModel> FMDVMDragAndDropViewModel::Create(const FMDViewModelAssignmentReference& InVMAssignment)
{
	TSharedRef<FMDVMDragAndDropViewModel> Action = MakeShared<FMDVMDragAndDropViewModel>();
	Action->VMAssignment = InVMAssignment;
	Action->MouseCursor = EMouseCursor::GrabHandClosed;
	Action->Construct();
	return Action;
}

UEdGraphNode* FMDVMDragAndDropViewModel::CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition)
{
	return FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_GetViewModel>(
		&Graph,
		GraphPosition,
		EK2NewNodeFlags::SelectNewNode,
		[&](UMDVMNode_GetViewModel* NewInstance)
		{
			NewInstance->SetDefaultAssignment(VMAssignment);
		}
	);
}

FText FMDVMDragAndDropViewModel::GetNodeTitle() const
{
	const FText VMClassName = (VMAssignment.ViewModelClass.Get() != nullptr) ? VMAssignment.ViewModelClass.Get()->GetDisplayNameText() : INVTEXT("NULL");
	return FText::Format(INVTEXT("Get {0} ({1})"), VMClassName, FText::FromName(VMAssignment.ViewModelName));
}

void SMDViewModelListItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwningTable, const TSharedPtr<FMDViewModelEditorAssignment>& Item, TWeakPtr<FBlueprintEditor> InBlueprintEditor)
{
	BlueprintEditor = InBlueprintEditor;

	OnEditItemRequested = InArgs._OnEditItemRequested;

	Assignment = Item;

	BackgroundBrush = static_cast<FSlateBrush>(FSlateColorBrush(FLinearColor::Transparent));

	ButtonStyle = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("FlatButton");
	ButtonStyle.NormalPadding = FMargin(2.f);
	ButtonStyle.PressedPadding = FMargin(2.f);

	const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(Assignment->Assignment.ProviderTag);
	const bool bIsViewModelClassValid = Item->Assignment.ViewModelClass != nullptr;

	const FText ViewModelToolTip = [&]()
	{
		if (bIsViewModelClassValid)
		{
			TArray<TSubclassOf<UObject>> SupportedContextObjects;
			Item->Assignment.ViewModelClass.GetDefaultObject()->GetStoredContextObjectTypes(Assignment->Data.ViewModelSettings, GetBlueprint(), SupportedContextObjects);

			FText VMToolTip = Item->Assignment.ViewModelClass->GetToolTipText();

			if (!SupportedContextObjects.IsEmpty())
			{
				VMToolTip = FText::Format(INVTEXT("{0}\n\nStored Context Object {1}|plural(one=Type,other=Types):"), VMToolTip, SupportedContextObjects.Num());

				for (const TSubclassOf<UObject>& Class : SupportedContextObjects)
				{
					VMToolTip = FText::Format(INVTEXT("{0}\n{1}"), VMToolTip, Class->GetDisplayNameText());
				}
			}

			return VMToolTip;
		}

		return INVTEXT("This view model is invalid, the class may have been renamed or deleted");
	}();

	const FText SuperDisplayName = (Assignment->SuperAssignmentOwner != nullptr) ? Assignment->SuperAssignmentOwner->GetDisplayNameText() : INVTEXT("NULL");
	const FText SuperToolTip = FText::Format(INVTEXT("This viewmodel is assigned in a parent blueprint [{0}] therefore it cannot be edited."), SuperDisplayName);

	STableRow<TSharedPtr<FMDViewModelEditorAssignment>>::Construct(
		STableRow<TSharedPtr<FMDViewModelEditorAssignment>>::FArguments()
		.Padding(0.0f)
		.Content()
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
						.ToolTipText(ViewModelToolTip)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(4, 0)
					[
						SNew(STextBlock)
						.Visibility(this, &SMDViewModelListItem::GetSourceTextVisibility)
						.Text(INVTEXT("Super"))
						.ToolTipText(SuperToolTip)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Cursor(EMouseCursor::Default)
						.ButtonStyle(&ButtonStyle)
						.ContentPadding(2.f)
						.OnClicked(this, &SMDViewModelListItem::OnContextButtonClicked)
						.ToolTipText(INVTEXT("Open the context menu for the view model assignment"))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
							.Image(FAppStyle::Get().GetBrush(TEXT("Menu.SubMenuIndicator")))
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
						.ToolTipText(IsValid(Provider) ? Provider->GetDescription(Item->Data.ProviderSettings) : INVTEXT("The selected provider is not valid, it may have been deleted or is in an unloaded module."))
					]
				]
			]
		], OwningTable);
}

TOptional<EMouseCursor::Type> SMDViewModelListItem::GetCursor() const
{
	return EMouseCursor::GrabHand;
}

FReply SMDViewModelListItem::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = STableRow<TSharedPtr<FMDViewModelEditorAssignment>>::OnMouseButtonDown(MyGeometry, MouseEvent);
	if (Assignment.IsValid() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		Reply = Reply.DetectDrag(AsShared(), EKeys::LeftMouseButton);
	}

	return Reply;
}

FReply SMDViewModelListItem::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = STableRow<TSharedPtr<FMDViewModelEditorAssignment>>::OnDragDetected(MyGeometry, MouseEvent);

	if (Assignment.IsValid())
	{
		const FMDViewModelAssignmentReference AssignmentRef = { Assignment->Assignment.ViewModelClass.Get(), Assignment->Assignment.ViewModelName };
		const TSharedRef<FMDVMDragAndDropViewModel> Action = FMDVMDragAndDropViewModel::Create(AssignmentRef);
		return FReply::Handled().BeginDragDrop(Action);
	}

	return Reply;
}

FReply SMDViewModelListItem::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (CanEdit())
	{
		OnEditClicked();
	}

	return FReply::Handled();
}

void SMDViewModelListItem::OnContextMenuOpening(FMenuBuilder& ContextMenuBuilder)
{
	ContextMenuBuilder.BeginSection(TEXT("ViewModel"), INVTEXT("View Model"));

	ContextMenuBuilder.AddMenuEntry(
		INVTEXT("Edit Assignment"),
		INVTEXT("Opens the view model assignment dialog to edit this assignment."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Edit")),
		FUIAction(
			FExecuteAction::CreateSP(this, &SMDViewModelListItem::OnEditClicked),
			FCanExecuteAction::CreateSP(this, &SMDViewModelListItem::CanEdit)
		)
	);

	ContextMenuBuilder.AddMenuEntry(
		FGenericCommands::Get().Copy,
		NAME_None,
		INVTEXT("Copy Assignment"),
		INVTEXT("Copy this view model assignment to the clipboard to be pasted in another blueprint."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("GenericCommands.Copy"))
	);

	ContextMenuBuilder.AddMenuEntry(
		INVTEXT("Find References"),
		INVTEXT("Search for references to this view model in this blueprint."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Find")),
		FUIAction(
			FExecuteAction::CreateSP(this, &SMDViewModelListItem::OnFindReferencesClicked)
		)
		);

	ContextMenuBuilder.AddMenuEntry(
		INVTEXT("Open Source Asset"),
		INVTEXT("Opens the blueprint where the view model assignment was created."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Edit")),
		FUIAction(
			FExecuteAction::CreateSP(this, &SMDViewModelListItem::OnOpenOwnerAssetClicked),
			FCanExecuteAction::CreateSP(this, &SMDViewModelListItem::CanOpenOwnerAsset)
		)
	);

	ContextMenuBuilder.AddMenuEntry(
		FGenericCommands::Get().Duplicate,
		NAME_None,
		INVTEXT("Duplicate Assignment"),
		INVTEXT("Opens the view model assignment dialog prepropulated with this assignment's settings."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Duplicate"))
	);

	ContextMenuBuilder.AddMenuEntry(
		FGenericCommands::Get().Delete,
		NAME_None,
		INVTEXT("Delete Assignment"),
		INVTEXT("Remove this view model assignment."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("GenericCommands.Delete"))
	);

	ContextMenuBuilder.EndSection();
}

EVisibility SMDViewModelListItem::GetSourceTextVisibility() const
{
	if (Assignment.IsValid() && IsValid(Assignment->SuperAssignmentOwner))
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

FReply SMDViewModelListItem::OnContextButtonClicked()
{
	FMenuBuilder ContextMenuBuilder(true, nullptr);
	OnContextMenuOpening(ContextMenuBuilder);

	TSharedPtr<IMenu> Menu = FSlateApplication::Get().PushMenu(
		AsShared(),
		FWidgetPath(),
		ContextMenuBuilder.MakeWidget(),
		FSlateApplication::Get().GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
	);

	return FReply::Handled();
}

void SMDViewModelListItem::OnFindReferencesClicked() const
{
	if (const TSharedPtr<FBlueprintEditor> BPEditor = BlueprintEditor.Pin())
	{
		// Widget BP's have the find window in graph mode
		if (IsValid(Cast<UWidgetBlueprint>(BPEditor->GetBlueprintObj())))
		{
			BPEditor->SetCurrentMode(FWidgetBlueprintApplicationModes::GraphMode);
		}

		BPEditor->SummonSearchUI(true, GenerateSearchString());
	}
}

void SMDViewModelListItem::OnEditClicked() const
{
	OnEditItemRequested.ExecuteIfBound();
}

bool SMDViewModelListItem::CanEdit() const
{
	return Assignment.IsValid() && Assignment->SuperAssignmentOwner == nullptr && !GEditor->bIsSimulatingInEditor && GEditor->PlayWorld == nullptr;
}

void SMDViewModelListItem::OnOpenOwnerAssetClicked() const
{
	if (CanOpenOwnerAsset())
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->OpenEditorForAsset(Assignment->SuperAssignmentOwner->ClassGeneratedBy);
		}
	}
}

bool SMDViewModelListItem::CanOpenOwnerAsset() const
{
	return GEditor != nullptr && Assignment.IsValid() && IsValid(Assignment->SuperAssignmentOwner) && IsValid(Assignment->SuperAssignmentOwner->ClassGeneratedBy);
}

FString SMDViewModelListItem::GenerateSearchString() const
{
	if (Assignment.IsValid())
	{
		return FText::FormatNamed(INVTEXT("Pins(Name=Assignment && DefaultValue=({ClassName}) && DefaultValue=({Name})) || \"{DisplayName} ({Name})\" || \"{DisplayName} - {Name}\""),
			TEXT("ClassName"), FText::FromString(Assignment->Assignment.ViewModelClass->GetPathName()),
			TEXT("DisplayName"), Assignment->Assignment.ViewModelClass->GetDisplayNameText(),
			TEXT("Name"), FText::FromName(Assignment->Assignment.ViewModelName)
		).ToString();
	}

	return TEXT("");
}

UBlueprint* SMDViewModelListItem::GetBlueprint() const
{
	if (const TSharedPtr<FBlueprintEditor> BPEditor = BlueprintEditor.Pin())
	{
		return BPEditor->GetBlueprintObj();
	}

	return nullptr;
}

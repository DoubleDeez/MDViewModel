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
#include "Nodes/MDVMNode_SetViewModel.h"
#include "Util/MDViewModelEditorAssignment.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"
#include "ViewModelTab/MDViewModelEditorCommands.h"
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

void FMDVMDragAndDropViewModel::CreateGetter(FMDVMCachedViewModelNodeParams Params)
{
	if (Params.Graph.IsValid())
	{
		UMDVMNode_GetViewModel* NewNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_GetViewModel>(
			Params.Graph.Get(),
			Params.GraphPosition,
			EK2NewNodeFlags::SelectNewNode,
			[&](UMDVMNode_GetViewModel* NewInstance)
			{
				NewInstance->SetDefaultAssignment(Params.VMAssignment);
			}
		);

		FinalizeNode(NewNode, Params);
	}
}

void FMDVMDragAndDropViewModel::CreateSetter(FMDVMCachedViewModelNodeParams Params)
{
	if (Params.Graph.IsValid())
	{
		UMDVMNode_SetViewModel* NewNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_SetViewModel>(
			Params.Graph.Get(),
			Params.GraphPosition,
			EK2NewNodeFlags::SelectNewNode,
			[&](UMDVMNode_SetViewModel* NewInstance)
			{
				NewInstance->SetDefaultAssignment(Params.VMAssignment);
			}
		);

		FinalizeNode(NewNode, Params);
	}
}

void FMDVMDragAndDropViewModel::FinalizeNode(UEdGraphNode* NewNode, const FMDVMCachedViewModelNodeParams& Params)
{
	if (NewNode)
	{
		if (UEdGraphPin* FromPin = Params.Pin.Get())
		{
			NewNode->AutowireNewNode(FromPin);
		}
		else if (const UEdGraphNode* FromNode = Params.Node.Get())
		{
			const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
			if (UEdGraphPin* FromNodeIn = Schema->FindExecutionPin(*FromNode, EGPD_Input))
			{
				NewNode->AutowireNewNode(FromNodeIn);
			}
			else if (UEdGraphPin* FromNodeOut = Schema->FindExecutionPin(*FromNode, EGPD_Output))
			{
				NewNode->AutowireNewNode(FromNodeOut);
			}
		}
	}
}

FReply FMDVMDragAndDropViewModel::DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
	const bool bModifiedKeysActive = ModifierKeys.IsControlDown() || ModifierKeys.IsAltDown();
	const bool bAutoCreateGetter = bModifiedKeysActive ? ModifierKeys.IsControlDown() : bControlDrag;
	const bool bAutoCreateSetter = bModifiedKeysActive ? ModifierKeys.IsAltDown() : bAltDrag;

	if (bAutoCreateGetter || bAutoCreateSetter)
	{
		bIsGetter = bAutoCreateGetter;
	}
	else if (const UEdGraphPin* FromPin = GetHoveredPin())
	{
		if (FromPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object || FromPin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject)
		{
			const UClass* PinClass = Cast<UClass>(FromPin->PinType.PinSubCategoryObject.Get());
			if (PinClass != nullptr && PinClass->IsChildOf<UMDViewModelBase>())
			{
				bIsGetter = FromPin->Direction == EGPD_Input;
			}
		}
	}

	if (bIsGetter.IsSet())
	{
		return FMDVMInspectorDragAndDropActionBase::DroppedOnPin(ScreenPosition, GraphPosition);
	}

	FMDVMCachedViewModelNodeParams Params = { VMAssignment, GraphPosition, GetHoveredGraph(), GetHoveredNode(), GetHoveredPin() };

	FMenuBuilder MenuBuilder(true, NULL);
	const FText Title = GetNodeTitle();

	MenuBuilder.BeginSection("ViewModelDropped", Title );

	MenuBuilder.AddMenuEntry(
		FText::Format( INVTEXT("Get {0}"), Title ),
		FText::Format( INVTEXT("Create Getter for view model '{0}'\n(Ctrl-drag to automatically create a getter)"), Title ),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FMDVMDragAndDropViewModel::CreateGetter, Params))
	);

	MenuBuilder.AddMenuEntry(
		FText::Format( INVTEXT("Set {0}"), Title ),
		FText::Format( INVTEXT("Create Setter for view model '{0}'\n(Alt-drag to automatically create a setter)"), Title ),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FMDVMDragAndDropViewModel::CreateSetter, Params))
	);

	MenuBuilder.EndSection();

	TSharedPtr<SWidget> MenuParent = HoveredPanelWidget.Pin();
	if (!MenuParent.IsValid())
	{
		MenuParent = FSlateApplication::Get().GetActiveTopLevelWindow();
	}

	FSlateApplication::Get().PushMenu(
		MenuParent.ToSharedRef(),
		FWidgetPath(),
		MenuBuilder.MakeWidget(),
		ScreenPosition,
		FPopupTransitionEffect( FPopupTransitionEffect::ContextMenu)
	);

	return FReply::Handled();
}

UEdGraphNode* FMDVMDragAndDropViewModel::CreateNodeOnDrop(UEdGraph& Graph, const FVector2D& GraphPosition)
{
	if (!bIsGetter.IsSet())
	{
		return nullptr;
	}

	if (bIsGetter.GetValue())
	{
		bIsGetter.Reset();
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
	else
	{
		bIsGetter.Reset();
		return FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_SetViewModel>(
			&Graph,
			GraphPosition,
			EK2NewNodeFlags::SelectNewNode,
			[&](UMDVMNode_SetViewModel* NewInstance)
			{
				NewInstance->SetDefaultAssignment(VMAssignment);
			}
		);
	}
}

FText FMDVMDragAndDropViewModel::GetNodeTitle() const
{
	const FText VMClassName = (VMAssignment.ViewModelClass.Get() != nullptr) ? VMAssignment.ViewModelClass.Get()->GetDisplayNameText() : INVTEXT("NULL");
	return FText::Format(INVTEXT("{0} ({1})"), VMClassName, FText::FromName(VMAssignment.ViewModelName));
}

void SMDViewModelListItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwningTable, const TSharedPtr<FMDViewModelEditorAssignment>& Item, TWeakPtr<FBlueprintEditor> InBlueprintEditor)
{
	CommandList = InArgs._CommandList;
	BlueprintEditor = InBlueprintEditor;
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
		Action->SetAltDrag(MouseEvent.IsAltDown());
		Action->SetCtrlDrag(MouseEvent.IsLeftControlDown() || MouseEvent.IsRightControlDown());
		return FReply::Handled().BeginDragDrop(Action);
	}

	return Reply;
}

void SMDViewModelListItem::OnContextMenuOpening(FMenuBuilder& ContextMenuBuilder)
{
	ContextMenuBuilder.BeginSection(TEXT("ViewModel"), INVTEXT("View Model"));
	{
		ContextMenuBuilder.AddMenuEntry(FMDViewModelEditorCommands::Get().Edit);

		ContextMenuBuilder.AddMenuEntry(
			FGenericCommands::Get().Copy,
			NAME_None,
			INVTEXT("Copy Assignment"),
			INVTEXT("Copy this view model assignment to the clipboard to be pasted in another blueprint."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("GenericCommands.Copy"))
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
	}
	ContextMenuBuilder.EndSection();

	ContextMenuBuilder.BeginSection(TEXT("ViewModelTools"), INVTEXT("View Model Tools"));
	{
		ContextMenuBuilder.AddMenuEntry(
			INVTEXT("Find References"),
			INVTEXT("Search for references to this view model in this blueprint."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Find")),
			FUIAction(
				FExecuteAction::CreateSP(this, &SMDViewModelListItem::OnFindReferencesClicked)
			)
		);

		ContextMenuBuilder.AddMenuEntry(FMDViewModelEditorCommands::Get().GoToDefinition);

		ContextMenuBuilder.AddMenuEntry(
			INVTEXT("Open Source Asset"),
			INVTEXT("Opens the blueprint where the view model assignment was created, if not in the current blueprint."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Edit")),
			FUIAction(
				FExecuteAction::CreateSP(this, &SMDViewModelListItem::OnOpenOwnerAssetClicked),
				FCanExecuteAction::CreateSP(this, &SMDViewModelListItem::CanOpenOwnerAsset)
			)
		);
	}
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
	OwnerTablePtr.Pin()->Private_SetItemSelection(Assignment, true);

	FMenuBuilder ContextMenuBuilder(true, CommandList);
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

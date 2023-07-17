#include "ViewModelTab/MDViewModelAssignmentDialog.h"

#include "Engine/Engine.h"
#include "Customizations/MDViewModelAssignmentEditorObjectCustomization.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "SPrimaryButton.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"
#include "ViewModelProviders/MDViewModelProvider_Cached.h"
#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"
#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"
#include "Widgets/Layout/SUniformGridPanel.h"

void SMDViewModelAssignmentDialog::Construct(const FArguments& InArgs, const TSharedRef<SWindow>& InParentWindow)
{
	bIsEditMode = InArgs._bIsEditMode;
	BPExtensionPtr = InArgs._BPExtensionPtr;
	EditorItem = InArgs._EditorItem;

	ParentWindow = InParentWindow;
	EditorObject = TStrongObjectPtr<UMDViewModelAssignmentEditorObject>(NewObject<UMDViewModelAssignmentEditorObject>());

	if (const UWidgetBlueprint* WidgetBlueprint = GetWidgetBlueprint())
	{
		EditorObject->WidgetSkeletonClass = WidgetBlueprint->SkeletonGeneratedClass;
	}

	if (EditorItem.IsValid())
	{
		EditorObject->PopulateFromAssignment(*EditorItem.Get(), GetWidgetBlueprint());
		OriginalAssignmentName = EditorObject->ViewModelInstanceName;
	}

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
	DetailsViewArgs.bShowScrollBar = true;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	const TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	TSharedRef<SMDViewModelAssignmentDialog> SharedThis = StaticCastSharedRef<SMDViewModelAssignmentDialog>(AsShared());
	DetailsView->RegisterInstancedCustomPropertyLayout(UMDViewModelAssignmentEditorObject::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FMDViewModelAssignmentEditorObjectCustomization::MakeInstance, SharedThis, bIsEditMode));
	DetailsView->SetObject(EditorObject.Get());

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.FillHeight(1.f)
		.HAlign(HAlign_Fill)
		[
			DetailsView
		]
		+SVerticalBox::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.AutoHeight()
		.Padding(8)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
			+SUniformGridPanel::Slot(0,0)
			[
				SNew(SPrimaryButton)
				.Text(INVTEXT("Add View Model"))
				.Icon(FAppStyle::Get().GetBrush(TEXT("EditableComboBox.Add")))
				.Visibility(this, &SMDViewModelAssignmentDialog::GetAddVisibility)
				.OnClicked(this, &SMDViewModelAssignmentDialog::OnAddClicked)
			]
			+SUniformGridPanel::Slot(0,0)
			[
				SNew(SPrimaryButton)
				.Text(INVTEXT("Save Changes"))
				.Visibility(this, &SMDViewModelAssignmentDialog::GetSaveVisibility)
				.OnClicked(this, &SMDViewModelAssignmentDialog::OnSaveClicked)
			]
			+SUniformGridPanel::Slot(1,0)
			[
				SNew(SButton)
				.Text(INVTEXT("Cancel"))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([WeakWindow = TWeakPtr<SWindow>(ParentWindow)]()
				{
					if (const TSharedPtr<SWindow> ParentWindow = WeakWindow.Pin())
					{
						ParentWindow->RequestDestroyWindow();
					}

					return FReply::Handled();
				})
			]
		]
	];
}

UWidgetBlueprint* SMDViewModelAssignmentDialog::GetWidgetBlueprint() const
{
	if (const UMDViewModelWidgetBlueprintExtension* BPExtension = BPExtensionPtr.Get())
	{
		return BPExtension->GetWidgetBlueprint();
	}

	return nullptr;
}

void SMDViewModelAssignmentDialog::OpenAssignmentDialog(UMDViewModelWidgetBlueprintExtension* BPExtension)
{
	const TSharedRef<SWindow> PickerWindow = SNew(SWindow)
		.Title(INVTEXT("Add a View Model"))
		.SizingRule( ESizingRule::UserSized )
		.ClientSize( FVector2D( 700.f, 600.f ))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	const TSharedRef<SMDViewModelAssignmentDialog> AssignmentDialog = SNew(SMDViewModelAssignmentDialog, PickerWindow)
		.BPExtensionPtr(BPExtension);

	PickerWindow->SetContent(AssignmentDialog);

	if (FSlateApplication::Get().GetActiveModalWindow().IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(PickerWindow, FSlateApplication::Get().GetActiveModalWindow().ToSharedRef());
	}
	else if (FGlobalTabmanager::Get()->GetRootWindow().IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(PickerWindow, FGlobalTabmanager::Get()->GetRootWindow().ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(PickerWindow);
	}
}

void SMDViewModelAssignmentDialog::OpenEditDialog(UMDViewModelWidgetBlueprintExtension* BPExtension, TSharedPtr<FMDViewModelEditorAssignment> EditorItem)
{
	const TSharedRef<SWindow> PickerWindow = SNew(SWindow)
		.Title(INVTEXT("Add a View Model"))
		.SizingRule( ESizingRule::UserSized )
		.ClientSize( FVector2D( 700.f, 600.f ))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	const TSharedRef<SMDViewModelAssignmentDialog> AssignmentDialog = SNew(SMDViewModelAssignmentDialog, PickerWindow)
		.BPExtensionPtr(BPExtension)
		.EditorItem(EditorItem)
		.bIsEditMode(true);

	PickerWindow->SetContent(AssignmentDialog);

	if (FSlateApplication::Get().GetActiveModalWindow().IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(PickerWindow, FSlateApplication::Get().GetActiveModalWindow().ToSharedRef());
	}
	else if (FGlobalTabmanager::Get()->GetRootWindow().IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(PickerWindow, FGlobalTabmanager::Get()->GetRootWindow().ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(PickerWindow);
	}
}

EVisibility SMDViewModelAssignmentDialog::GetAddVisibility() const
{
	if (bIsEditMode || !EditorObject.IsValid())
	{
		return EVisibility::Collapsed;
	}

	if (EditorObject->ViewModelInstanceName == NAME_None || EditorObject->ViewModelClass == nullptr)
	{
		return EVisibility::Collapsed;
	}

	if (!IsAssignmentUnique() || !EditorObject->ViewModelProvider.IsValid())
	{
		return EVisibility::Collapsed;
	}

	const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider);
	TArray<FText> UnusedIssues;
	if (!Provider->ValidateProviderSettings(EditorObject->ProviderSettings, GetWidgetBlueprint(), UnusedIssues))
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SMDViewModelAssignmentDialog::GetSaveVisibility() const
{
	if (!bIsEditMode || !EditorObject.IsValid())
	{
		return EVisibility::Collapsed;
	}

	if (EditorObject->ViewModelInstanceName == NAME_None || EditorObject->ViewModelClass == nullptr)
	{
		return EVisibility::Collapsed;
	}

	if (!IsAssignmentUnique() || !EditorObject->ViewModelProvider.IsValid())
	{
		return EVisibility::Collapsed;
	}

	const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider);
	TArray<FText> UnusedIssues;
	if (!IsValid(Provider) || !Provider->ValidateProviderSettings(EditorObject->ProviderSettings, GetWidgetBlueprint(), UnusedIssues))
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

FReply SMDViewModelAssignmentDialog::OnAddClicked() const
{
	if (UMDViewModelWidgetBlueprintExtension* BPExtension = BPExtensionPtr.Get())
	{
		FScopedTransaction Transaction = FScopedTransaction(INVTEXT("Added View Model Assignment"));
		BPExtension->Modify();
		BPExtension->AddAssignment(EditorObject->CreateAssignment());
	}

	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
	}

	return FReply::Handled();
}

FReply SMDViewModelAssignmentDialog::OnSaveClicked() const
{
	if (EditorItem.IsValid())
	{
		if (UMDViewModelWidgetBlueprintExtension* BPExtension = BPExtensionPtr.Get())
		{
			FScopedTransaction Transaction = FScopedTransaction(INVTEXT("Updated View Model Assignment"));
			BPExtension->Modify();
			BPExtension->UpdateAssignment(*EditorItem.Get(), EditorObject->CreateAssignment());
		}
	}

	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
	}

	return FReply::Handled();
}

bool SMDViewModelAssignmentDialog::IsAssignmentUnique() const
{
	if (EditorObject.IsValid())
	{
		if (bIsEditMode && OriginalAssignmentName.IsSet() && EditorObject->ViewModelInstanceName == OriginalAssignmentName.GetValue())
		{
			// We haven't changed the name we came in with, so assume we're still unique
			return true;
		}

		if (const UMDViewModelWidgetBlueprintExtension* Extension = BPExtensionPtr.Get())
		{
			if (!Extension->DoesContainViewModelAssignment(EditorObject->ViewModelClass, FGameplayTag::EmptyTag, EditorObject->ViewModelInstanceName))
			{
				return true;
			}
		}
	}

	return false;
}

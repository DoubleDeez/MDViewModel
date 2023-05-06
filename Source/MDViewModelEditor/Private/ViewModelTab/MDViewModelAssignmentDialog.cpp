#include "ViewModelTab/MDViewModelAssignmentDialog.h"

#include "ClassViewerFilter.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "IDetailCustomization.h"
#include "MDViewModelModule.h"
#include "PropertyEditorModule.h"
#include "SClassViewer.h"
#include "SPrimaryButton.h"
#include "Components/VerticalBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Misc/FeedbackContext.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"
#include "ViewModelTab/MDViewModelClassList.h"
#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SUniformGridPanel.h"


class FEditorObjectCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance(TSharedRef<SMDViewModelAssignmentDialog> Dialog, bool bIsEditMode)
	{
		return MakeShared<FEditorObjectCustomization>(Dialog, bIsEditMode);
	}

	FEditorObjectCustomization(TSharedRef<SMDViewModelAssignmentDialog> InDialog, bool bIsEditMode)
		: Dialog(InDialog)
		, bIsEditMode(bIsEditMode)
	{
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
	{
		TArray<TWeakObjectPtr<UObject>> SelectedObjects;
		DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);
		if (!SelectedObjects.IsEmpty())
		{
			EditorObjectPtr = Cast<UMDViewModelAssignmentEditorObject>(SelectedObjects[0].Get());
		}

		const FResetToDefaultOverride HideResetToDefault = FResetToDefaultOverride::Hide();

		const TSharedRef<IPropertyHandle> ProviderTagHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ViewModelProvider), UMDViewModelAssignmentEditorObject::StaticClass());
		DetailBuilder.EditDefaultProperty(ProviderTagHandle)->OverrideResetToDefault(HideResetToDefault).CustomWidget()
		.NameContent()
		[
			ProviderTagHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.IsEnabled(!bIsEditMode)
			.OnGetMenuContent(this, &FEditorObjectCustomization::OnGetProviderMenuContent)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FEditorObjectCustomization::GetSelectedProviderName)
			]
		];

		const TSharedRef<IPropertyHandle> ProviderSettingsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ProviderSettings), UMDViewModelAssignmentEditorObject::StaticClass());
		if (const UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
		{
			if (EditorObject->ProviderSettings.IsValid())
			{
				DetailBuilder.EditDefaultProperty(ProviderSettingsHandle)->ShouldAutoExpand(true).OverrideResetToDefault(HideResetToDefault);
			}
			else
			{
				DetailBuilder.HideProperty(ProviderSettingsHandle);
			}

			const TSharedRef<IPropertyHandle> ViewModelClassHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ViewModelClass), UMDViewModelAssignmentEditorObject::StaticClass());
			const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
			if (const TSharedPtr<FMDViewModelProviderBase> Provider = ViewModelModule.GetViewModelProvider(EditorObject->ViewModelProvider))
			{
				DetailBuilder.EditDefaultProperty(ViewModelClassHandle)->CustomWidget()
				.OverrideResetToDefault(HideResetToDefault)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.AutoHeight()
					.Padding(2.f, 4.f, 2.f, 2.f)
					[
						ViewModelClassHandle->CreatePropertyNameWidget()
					]
					+SVerticalBox::Slot()
					.FillHeight(1.f)
					.Padding(2.f)
					[
						SNew(SMDViewModelClassList, Provider)
						.IsEnabled(!bIsEditMode)
						.SelectedClass(EditorObjectPtr.IsValid() ? EditorObjectPtr->ViewModelClass : nullptr)
						.OnSelectionChanged(Dialog, &SMDViewModelAssignmentDialog::OnClassListSelectionChanged)
					]
				];
			}
			else
			{
				DetailBuilder.HideProperty(ViewModelClassHandle);
			}
		}

	}

	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override
	{
		CachedDetailBuilder = DetailBuilder;
		CustomizeDetails(*DetailBuilder);
	}

private:
	FText GetSelectedProviderName() const
	{
		if (const UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
		{
			const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
			if (TSharedPtr<FMDViewModelProviderBase> Provider = ViewModelModule.GetViewModelProvider(EditorObject->ViewModelProvider))
			{
				return Provider->GetDisplayName();
			}
			else
			{
				return INVTEXT("Select a View Model Provider...");
			}
		}

		return INVTEXT("[Invalid Editor Object]");
	}

	FText GetSelectedProviderToolTip() const
	{
		if (const UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
		{
			const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
			if (const TSharedPtr<FMDViewModelProviderBase> Provider = ViewModelModule.GetViewModelProvider(EditorObject->ViewModelProvider))
			{
				return Provider->GetDescription();
			}
			else
			{
				return INVTEXT("Select a View Model Provider...");
			}
		}

		return INVTEXT("[Invalid Editor Object]");
	}

	TSharedRef<SWidget> OnGetProviderMenuContent() const
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
		TMap<FGameplayTag, TSharedPtr<FMDViewModelProviderBase>> ViewModelProviders = ViewModelModule.GetViewModelProviders();

		ViewModelProviders.ValueSort([](const TSharedPtr<FMDViewModelProviderBase>& A, const TSharedPtr<FMDViewModelProviderBase>& B)
		{
			if (A.IsValid() != B.IsValid())
			{
				return A.IsValid();
			}

			return A->GetDisplayName().CompareTo(B->GetDisplayName()) < 0;
		});

		for (const auto& Pair : ViewModelProviders)
		{
			if (const TSharedPtr<FMDViewModelProviderBase> Provider = Pair.Value)
			{
				MenuBuilder.AddMenuEntry(
					Provider->GetDisplayName(),
					Provider->GetDescription(),
					FSlateIcon(),
					FUIAction(
						FExecuteAction::CreateSP(this, &FEditorObjectCustomization::OnProviderSelected, Pair.Key)
					));
			}
		}

		return MenuBuilder.MakeWidget();
	}

	void OnProviderSelected(FGameplayTag Tag) const
	{
		if (UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
		{
			EditorObject->ViewModelProvider = Tag;
			const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
			if (const TSharedPtr<FMDViewModelProviderBase> Provider = ViewModelModule.GetViewModelProvider(Tag))
			{
				EditorObject->ProviderSettings.InitializeAs(Provider->GetProviderSettingsStruct());
			}
			else
			{
				EditorObject->ProviderSettings.Reset();
			}

			if (IDetailLayoutBuilder* LayoutBuilder = CachedDetailBuilder.Pin().Get())
			{
				LayoutBuilder->ForceRefreshDetails();
			}
		}
	}

	void OnViewModelClassPicked(UClass* ViewModelClass) const
	{
		if (UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
		{
			EditorObject->ViewModelClass = TSubclassOf<UMDViewModelBase>(ViewModelClass);
		}
	}

	TWeakPtr<IDetailLayoutBuilder> CachedDetailBuilder;
	TWeakObjectPtr<UMDViewModelAssignmentEditorObject> EditorObjectPtr;
	TSharedRef<SMDViewModelAssignmentDialog> Dialog;
	bool bIsEditMode;
};

void SMDViewModelAssignmentDialog::Construct(const FArguments& InArgs, const TSharedRef<SWindow>& InParentWindow)
{
	bIsEditMode = InArgs._bIsEditMode;
	BPExtensionPtr = InArgs._BPExtensionPtr;
	EditorItem = InArgs._EditorItem;

	ParentWindow = InParentWindow;
	EditorObject = TStrongObjectPtr<UMDViewModelAssignmentEditorObject>(NewObject<UMDViewModelAssignmentEditorObject>());
	if (EditorItem.IsValid())
	{
		EditorObject->PopulateFromAssignment(*EditorItem.Get());
	}

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ENameAreaSettings::HideNameArea;
	DetailsViewArgs.bShowScrollBar = true;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	const TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	TSharedRef<SMDViewModelAssignmentDialog> SharedThis = StaticCastSharedRef<SMDViewModelAssignmentDialog>(AsShared());
	DetailsView->RegisterInstancedCustomPropertyLayout(UMDViewModelAssignmentEditorObject::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FEditorObjectCustomization::MakeInstance, SharedThis, bIsEditMode));
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
				.Visibility_Lambda([this]()
				{
					if (!bIsEditMode && EditorObject.IsValid())
					{
						// TODO - check assignment uniqueness
						if (EditorObject->ViewModelProvider.IsValid() && EditorObject->ViewModelInstanceName != NAME_None && SelectedViewModelItem.IsValid())
						{
							return EVisibility::Visible;
						}
					}

					return EVisibility::Collapsed;
				})
				.OnClicked(this, &SMDViewModelAssignmentDialog::OnAddClicked)
			]
			+SUniformGridPanel::Slot(0,0)
			[
				SNew(SPrimaryButton)
				.Text(INVTEXT("Save Changes"))
				.Visibility_Lambda([this]()
				{
					if (bIsEditMode && EditorObject.IsValid())
					{
						// TODO - check assignment uniqueness
						if (EditorObject->ViewModelProvider.IsValid() && EditorObject->ViewModelInstanceName != NAME_None)
						{
							return EVisibility::Visible;
						}
					}

					return EVisibility::Collapsed;
				})
				.OnClicked(this, &SMDViewModelAssignmentDialog::OnSaveClicked)
			]
			+SUniformGridPanel::Slot(1,0)
			[
				SNew(SButton)
				.Text(INVTEXT("Cancel"))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([WeakWindow = ParentWindow.ToWeakPtr()]()
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

void SMDViewModelAssignmentDialog::OpenAssignmentDialog(UMDViewModelWidgetBlueprintExtension* BPExtension)
{
	const TSharedRef<SWindow> PickerWindow = SNew(SWindow)
		.Title(INVTEXT("Add a View Model"))
		.SizingRule( ESizingRule::UserSized )
		.ClientSize( FVector2D( 500.f, 600.f ))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	const TSharedRef<SMDViewModelAssignmentDialog> AssignmentDialog = SNew(SMDViewModelAssignmentDialog, PickerWindow)
		.BPExtensionPtr(BPExtension);

	PickerWindow->SetContent(AssignmentDialog);

	GEditor->EditorAddModalWindow(PickerWindow);
}

void SMDViewModelAssignmentDialog::OpenEditDialog(UMDViewModelWidgetBlueprintExtension* BPExtension, TSharedPtr<FMDViewModelEditorAssignment> EditorItem)
{
	const TSharedRef<SWindow> PickerWindow = SNew(SWindow)
		.Title(INVTEXT("Add a View Model"))
		.SizingRule( ESizingRule::UserSized )
		.ClientSize( FVector2D( 500.f, 600.f ))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	const TSharedRef<SMDViewModelAssignmentDialog> AssignmentDialog = SNew(SMDViewModelAssignmentDialog, PickerWindow)
		.BPExtensionPtr(BPExtension)
		.EditorItem(EditorItem)
		.bIsEditMode(true);

	PickerWindow->SetContent(AssignmentDialog);

	GEditor->EditorAddModalWindow(PickerWindow);
}

FReply SMDViewModelAssignmentDialog::OnAddClicked() const
{
	if (SelectedViewModelItem.IsValid())
	{
		UClass* Class = SelectedViewModelItem->Class.Get();
		if (Class == nullptr)
		{
			GWarn->BeginSlowTask(INVTEXT("Loading View Model Class..."), true);
			Class = LoadObject<UClass>(nullptr, *SelectedViewModelItem->ClassPath.ToString());
			GWarn->EndSlowTask();
		}

		if (Class == nullptr)
		{
			FMessageLog EditorErrors("EditorErrors");
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("ObjectName"), FText::FromString(SelectedViewModelItem->ClassPath.ToString()));
			EditorErrors.Error(FText::Format(INVTEXT("Failed to load class {ObjectName}"), Arguments));
		}

		EditorObject->ViewModelClass = Class;
		if (UMDViewModelWidgetBlueprintExtension* BPExtension = BPExtensionPtr.Get())
		{
			BPExtension->AddAssignment(EditorObject->CreateAssignment());
		}
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
			BPExtension->UpdateAssignment(*EditorItem.Get(), EditorObject->CreateAssignment());
		}
	}

	if (ParentWindow.IsValid())
	{
		ParentWindow->RequestDestroyWindow();
	}

	return FReply::Handled();
}

void SMDViewModelAssignmentDialog::OnClassListSelectionChanged(TSharedPtr<FMDViewModelClassItem> Item, ESelectInfo::Type SelectInfo)
{
	if (!bIsEditMode)
	{
		SelectedViewModelItem = Item;
	}
}

#include "ViewModelTab/MDViewModelAssignmentDialog.h"

#include "ClassViewerFilter.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "IDetailCustomization.h"
#include "MDViewModelModule.h"
#include "PropertyEditorModule.h"
#include "SClassViewer.h"
#include "ScopedTransaction.h"
#include "SPrimaryButton.h"
#include "Components/VerticalBox.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Misc/FeedbackContext.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProviderBase.h"
#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"
#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SUniformGridPanel.h"

class FMDViewModelProviderClassFilter : public IClassViewerFilter
{

public:
	FMDViewModelProviderClassFilter(UMDViewModelProviderBase* Provider)
	{
		if (IsValid(Provider))
		{
			Provider->GetSupportedViewModelClasses(ProviderSupportedViewModelClasses);
		}
	}

	TArray<FMDViewModelSupportedClass> ProviderSupportedViewModelClasses;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs ) override
	{
		const bool bHasValidFlags = !InClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated);
		if (bHasValidFlags)
		{
			for (const FMDViewModelSupportedClass& SupportedClass : ProviderSupportedViewModelClasses)
			{
				if (SupportedClass.Class == InClass || (SupportedClass.bAllowChildClasses && InClass->IsChildOf(SupportedClass.Class)))
				{
					return true;
				}
			}
		}

		return false;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const class IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs) override
	{
		const bool bHasValidFlags = !InUnloadedClassData->HasAnyClassFlags(CLASS_Abstract | CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated);
		if (bHasValidFlags)
		{
			for (const FMDViewModelSupportedClass& SupportedClass : ProviderSupportedViewModelClasses)
			{
				if (SupportedClass.bAllowChildClasses && InUnloadedClassData->IsChildOf(SupportedClass.Class))
				{
					return true;
				}
			}
		}

		return false;
	}

};

// TODO - move to its own file
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

		if (UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
		{
			const TSharedRef<IPropertyHandle> ViewModelClassHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ViewModelClass), UMDViewModelAssignmentEditorObject::StaticClass());
			const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider);

			const TSharedRef<IPropertyHandle> ProviderSettingsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ProviderSettings), UMDViewModelAssignmentEditorObject::StaticClass());
			if (IsValid(Provider) && EditorObject->ProviderSettings.IsValid())
			{
				ProviderSettingsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FEditorObjectCustomization::RefreshDetails));
				DetailBuilder.EditDefaultProperty(ProviderSettingsHandle)->ShouldAutoExpand(true).OverrideResetToDefault(HideResetToDefault);
			}
			else
			{
				DetailBuilder.HideProperty(ProviderSettingsHandle);
			}

			const TSharedRef<IPropertyHandle> ViewModelSettingsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ViewModelSettings), UMDViewModelAssignmentEditorObject::StaticClass());
			if (IsValid(Provider) && EditorObject->ViewModelSettings.IsValid())
			{
				if (Provider->DoesSupportViewModelSettings())
				{
					ViewModelSettingsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FEditorObjectCustomization::RefreshDetails));
					DetailBuilder.EditDefaultProperty(ViewModelSettingsHandle)->ShouldAutoExpand(true).OverrideResetToDefault(HideResetToDefault);
				}
				else
				{
					DetailBuilder.EditDefaultProperty(ViewModelSettingsHandle)->CustomWidget().OverrideResetToDefault(HideResetToDefault)
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.ColorAndOpacity(FLinearColor(1.f, 0.65f, 0.f))
						.Text(INVTEXT("The selected View Model class has Settings but the selected provider does not support View Model Settings so the View Model may not be properly initialized."))
					];
				}
			}
			else
			{
				DetailBuilder.HideProperty(ViewModelSettingsHandle);
			}

			if (IsValid(Provider))
			{
				Provider->OnProviderSettingsInitializedInEditor(EditorObject->ProviderSettings, Dialog->GetWidgetBlueprint());
				
				DetailBuilder.EditDefaultProperty(ViewModelClassHandle)->CustomWidget()
				.NameContent()
				[
					ViewModelClassHandle->CreatePropertyNameWidget()
				]
				.ValueContent()
				[
					SNew(SButton)
					.IsEnabled(!bIsEditMode)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("ComboButton").ButtonStyle)
					.OnClicked(this, &FEditorObjectCustomization::OnClassPickerButtonClicked)
					.Text(this, &FEditorObjectCustomization::GetSelectedClassText)
					.ToolTipText(this, &FEditorObjectCustomization::GetSelectedClassToolTipText)
				];

				TArray<FText> ProviderIssues;
				Provider->ValidateProviderSettings(EditorObject->ProviderSettings, Dialog->GetWidgetBlueprint(), ProviderIssues);

				if (!ProviderIssues.IsEmpty())
				{
					IDetailCategoryBuilder& ProviderValidationCategory = DetailBuilder.EditCategory(TEXT("Provider Validation"));
					ProviderValidationCategory.SetSortOrder(9001);

					for (const FText& Issue : ProviderIssues)
					{
						ProviderValidationCategory.AddCustomRow(Issue).WholeRowContent()
						[
							SNew(STextBlock)
							.Text(Issue)
							.AutoWrapText(true)
						];
					}
				}
			}
			else
			{
				DetailBuilder.HideProperty(ViewModelClassHandle);
			}

			if (const UMDViewModelBase* ViewModelCDO = EditorObject->ViewModelClass.GetDefaultObject())
			{
				TArray<FText> ViewModelIssues;
				ViewModelCDO->ValidateViewModelSettings(EditorObject->ViewModelSettings, Dialog->GetWidgetBlueprint(), ViewModelIssues);

				if (!ViewModelIssues.IsEmpty())
				{
					IDetailCategoryBuilder& ViewModelValidationCategory = DetailBuilder.EditCategory(TEXT("View Model Validation"));
					ViewModelValidationCategory.SetSortOrder(9002);

					for (const FText& Issue : ViewModelIssues)
					{
						ViewModelValidationCategory.AddCustomRow(Issue).WholeRowContent()
						[
							SNew(STextBlock)
							.Text(Issue)
							.AutoWrapText(true)
						];
					}
				}
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
			if (const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider))
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
			if (const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider))
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

		if (GEngine != nullptr)
		{
			TArray<UMDViewModelProviderBase*> ViewModelProviders = GEngine->GetEngineSubsystemArray<UMDViewModelProviderBase>();

			ViewModelProviders.Sort([](const UMDViewModelProviderBase& A, const UMDViewModelProviderBase& B)
			{
				return A.GetDisplayName().CompareTo(B.GetDisplayName()) < 0;
			});

			for (const UMDViewModelProviderBase* Provider : ViewModelProviders)
			{
				if (IsValid(Provider))
				{
					MenuBuilder.AddMenuEntry(
						Provider->GetDisplayName(),
						Provider->GetDescription(),
						FSlateIcon(),
						FUIAction(
							FExecuteAction::CreateSP(this, &FEditorObjectCustomization::OnProviderSelected, Provider->GetProviderTag())
						));
				}
			}
		}

		return MenuBuilder.MakeWidget();
	}

	void OnProviderSelected(FGameplayTag Tag) const
	{
		if (UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
		{
			EditorObject->ViewModelProvider = Tag;
			if (UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider))
			{
				EditorObject->ProviderSettings.InitializeAs(Provider->GetProviderSettingsStruct());
				Provider->OnProviderSettingsInitializedInEditor(EditorObject->ProviderSettings, Dialog->GetWidgetBlueprint());
			}
			else
			{
				EditorObject->ViewModelClass = nullptr;
				EditorObject->ViewModelSettings.Reset();
				EditorObject->ProviderSettings.Reset();
			}

			RefreshDetails();
		}
	}

	void OnViewModelClassPicked(UClass* ViewModelClass) const
	{
		if (UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
		{
			EditorObject->ViewModelClass = TSubclassOf<UMDViewModelBase>(ViewModelClass);

			EditorObject->ViewModelSettings.Reset();

			if (ViewModelClass != nullptr)
			{
				if (const UMDViewModelBase* ViewModelCDO = ViewModelClass->GetDefaultObject<UMDViewModelBase>())
				{
					EditorObject->ViewModelSettings.InitializeAs(ViewModelCDO->GetViewModelSettingsStruct());
				}
			}

			RefreshDetails();
		}
	}

	UClass* GetCurrentViewModelClass() const
	{
		if (const UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
		{
			return EditorObject->ViewModelClass;
		}

		return nullptr;
	}

	FReply OnClassPickerButtonClicked() const
	{
		if (const UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
		{
			const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
			if (UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider))
			{
				FClassViewerInitializationOptions ClassPickerOptions;
				ClassPickerOptions.bShowNoneOption = false;
				ClassPickerOptions.ClassFilters.Add(MakeShareable(new FMDViewModelProviderClassFilter(Provider)));
				ClassPickerOptions.InitiallySelectedClass = GetCurrentViewModelClass();

				UClass* Class = nullptr;
				if (SClassPickerDialog::PickClass(INVTEXT("Select the View Model Class"), ClassPickerOptions, Class, UClass::StaticClass()))
				{
					OnViewModelClassPicked(Class);
				}
			}
		}

		return FReply::Handled();
	}

	FText GetSelectedClassText() const
	{
		if (const UClass* Class = GetCurrentViewModelClass())
		{
			return Class->GetDisplayNameText();
		}

		return INVTEXT("Select a ViewModel Class...");
	}

	FText GetSelectedClassToolTipText() const
	{
		if (const UClass* Class = GetCurrentViewModelClass())
		{
			return Class->GetToolTipText();
		}

		return INVTEXT("Select a ViewModel Class...");
	}

	void RefreshDetails() const
	{
		if (IDetailLayoutBuilder* LayoutBuilder = CachedDetailBuilder.Pin().Get())
		{
			LayoutBuilder->ForceRefreshDetails();
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
			if (const UWidgetBlueprint* WidgetBP = Extension->GetWidgetBlueprint())
			{
				if (const TSubclassOf<UUserWidget> WidgetClass = Cast<UClass>(WidgetBP->GeneratedClass))
				{
					TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
					const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
					ViewModelModule.GetViewModelAssignmentsForWidgetClass(WidgetClass, ViewModelAssignments);

					for (const auto& Pair : ViewModelAssignments)
					{
						if (Pair.Key.ViewModelClass == EditorObject->ViewModelClass && Pair.Key.ViewModelName == EditorObject->ViewModelInstanceName)
						{
							return false;
						}
					}

					return true;
				}
			}
		}
	}

	return false;
}

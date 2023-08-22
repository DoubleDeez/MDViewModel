#include "Customizations/MDViewModelAssignmentEditorObjectCustomization.h"

#include "ClassViewerFilter.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/Engine.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Kismet2/SClassPickerDialog.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/MDViewModelAssignmentDialog.h"
#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"

namespace MDViewModelAssignmentEditorObjectCustomization_Private
{
	bool DoesStructHaveEditableProperties(const UScriptStruct* Struct)
	{
		for (TFieldIterator<const FProperty> It(Struct); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Edit))
			{
				return true;
			}
		}

		return false;
	}
}


class FMDViewModelProviderClassFilter : public IClassViewerFilter
{
public:
	FMDViewModelProviderClassFilter(UMDViewModelProviderBase* Provider)
	{
		if (IsValid(Provider))
		{
			Provider->GetSupportedViewModelClasses(ProviderSupportedViewModelClasses);
			bAllowAbstract = Provider->DoesSupportAbstractViewModelClasses();
		}
	}

	bool bAllowAbstract = false;
	TArray<FMDViewModelSupportedClass> ProviderSupportedViewModelClasses;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs ) override
	{
		if (InClass == UMDViewModelBase::StaticClass())
		{
			return false;
		}
		
		const bool bHasValidFlags = !InClass->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated);
		if (bHasValidFlags && (bAllowAbstract || !InClass->HasAnyClassFlags(CLASS_Abstract)))
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
		const bool bHasValidFlags = !InUnloadedClassData->HasAnyClassFlags(CLASS_Hidden | CLASS_HideDropDown | CLASS_Deprecated);
		if (bHasValidFlags && (bAllowAbstract || !InUnloadedClassData->HasAnyClassFlags(CLASS_Abstract)))
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


FMDViewModelAssignmentEditorObjectCustomization::FMDViewModelAssignmentEditorObjectCustomization(TSharedRef<SMDViewModelAssignmentDialog> InDialog, bool bIsEditMode)
	: Dialog(InDialog)
	, bIsEditMode(bIsEditMode)
{
}

void FMDViewModelAssignmentEditorObjectCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
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
		.OnGetMenuContent(this, &FMDViewModelAssignmentEditorObjectCustomization::OnGetProviderMenuContent)
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(this, &FMDViewModelAssignmentEditorObjectCustomization::GetSelectedProviderName)
		]
	];

	if (UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
	{
		const TSharedRef<IPropertyHandle> ViewModelClassHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ViewModelClass), UMDViewModelAssignmentEditorObject::StaticClass());
		const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider);
		
		const FMDViewModelAssignment Assignment = EditorObject->CreateAssignment().Assignment;

		const TSharedRef<IPropertyHandle> ProviderSettingsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ProviderSettings), UMDViewModelAssignmentEditorObject::StaticClass());
		if (IsValid(Provider) && EditorObject->ProviderSettings.IsValid())
		{
			ProviderSettingsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FMDViewModelAssignmentEditorObjectCustomization::OnProviderPropertyChanged));
			DetailBuilder.EditDefaultProperty(ProviderSettingsHandle)->ShouldAutoExpand(true).OverrideResetToDefault(HideResetToDefault);
		}
		else
		{
			DetailBuilder.HideProperty(ProviderSettingsHandle);
		}

		const TSharedRef<IPropertyHandle> ViewModelSettingsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ViewModelSettings), UMDViewModelAssignmentEditorObject::StaticClass());
		if (IsValid(Provider) && EditorObject->ViewModelSettings.IsValid() && MDViewModelAssignmentEditorObjectCustomization_Private::DoesStructHaveEditableProperties(EditorObject->ViewModelSettings.GetScriptStruct()))
		{
			if (Provider->DoesSupportViewModelSettings())
			{
				ViewModelSettingsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FMDViewModelAssignmentEditorObjectCustomization::OnViewModelPropertyChanged));
				DetailBuilder.EditDefaultProperty(ViewModelSettingsHandle)->ShouldAutoExpand(true).OverrideResetToDefault(HideResetToDefault);
			}
			else if (EditorObject->ViewModelProvider == TAG_MDVMProvider_Manual)
			{
				// Manual provider doesn't initialize view models so we don't need to warn here.
				DetailBuilder.HideProperty(ViewModelSettingsHandle);
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
			Provider->OnProviderSettingsInitializedInEditor(EditorObject->ProviderSettings, Dialog->GetWidgetBlueprint(), Assignment);

			FDetailWidgetRow& VMClassWidget = DetailBuilder.EditDefaultProperty(ViewModelClassHandle)->CustomWidget()
			.NameContent()
			[
				ViewModelClassHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				SNew(SButton)
				.IsEnabled(!bIsEditMode)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("ComboButton").ButtonStyle)
				.OnClicked(this, &FMDViewModelAssignmentEditorObjectCustomization::OnClassPickerButtonClicked)
				.Text(this, &FMDViewModelAssignmentEditorObjectCustomization::GetSelectedClassText)
				.ToolTipText(this, &FMDViewModelAssignmentEditorObjectCustomization::GetSelectedClassToolTipText)
			];

			if (bIsEditMode)
			{
				VMClassWidget.OverrideResetToDefault(HideResetToDefault);
			}

			TArray<FText> ProviderIssues;
			Provider->ValidateProviderSettings(EditorObject->ProviderSettings, Dialog->GetWidgetBlueprint(), Assignment, ProviderIssues);

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
			ViewModelCDO->ValidateViewModelSettings(EditorObject->ViewModelSettings, Dialog->GetWidgetBlueprint(), Assignment, ViewModelIssues);

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

FText FMDViewModelAssignmentEditorObjectCustomization::GetSelectedProviderName() const
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

FText FMDViewModelAssignmentEditorObjectCustomization::GetSelectedProviderToolTip() const
{
	if (const UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
	{
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

TSharedRef<SWidget> FMDViewModelAssignmentEditorObjectCustomization::OnGetProviderMenuContent() const
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
						FExecuteAction::CreateSP(this, &FMDViewModelAssignmentEditorObjectCustomization::OnProviderSelected, Provider->GetProviderTag())
					));
			}
		}
	}

	return MenuBuilder.MakeWidget();
}

void FMDViewModelAssignmentEditorObjectCustomization::OnProviderSelected(FGameplayTag Tag) const
{
	if (UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
	{
		EditorObject->ViewModelProvider = Tag;
		if (const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider))
		{
			EditorObject->ProviderSettings.InitializeAs(Provider->GetProviderSettingsStruct());
			Provider->OnProviderSettingsInitializedInEditor(EditorObject->ProviderSettings, Dialog->GetWidgetBlueprint(), EditorObject->CreateAssignment().Assignment);
		}
		else
		{
			EditorObject->ViewModelClass = nullptr;
			EditorObject->ViewModelSettings.Reset();
			EditorObject->ProviderSettings.Reset();
		}

		OnAssignmentUpdated();
		RefreshDetails();
	}
}

void FMDViewModelAssignmentEditorObjectCustomization::OnViewModelClassPicked(UClass* ViewModelClass) const
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

		OnAssignmentUpdated();
		RefreshDetails();
	}
}

UClass* FMDViewModelAssignmentEditorObjectCustomization::GetCurrentViewModelClass() const
{
	if (const UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
	{
		return EditorObject->ViewModelClass;
	}

	return nullptr;
}

FReply FMDViewModelAssignmentEditorObjectCustomization::OnClassPickerButtonClicked() const
{
	if (const UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
	{
		if (UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider))
		{
			FClassViewerInitializationOptions ClassPickerOptions;
			ClassPickerOptions.bShowNoneOption = false;
			ClassPickerOptions.ClassFilters.Add(MakeShareable(new FMDViewModelProviderClassFilter(Provider)));
			ClassPickerOptions.InitiallySelectedClass = GetCurrentViewModelClass();
			ClassPickerOptions.NameTypeToDisplay = EClassViewerNameTypeToDisplay::Dynamic;

			UClass* Class = nullptr;
			if (SClassPickerDialog::PickClass(INVTEXT("Select the View Model Class"), ClassPickerOptions, Class, UClass::StaticClass()))
			{
				OnViewModelClassPicked(Class);
			}
		}
	}

	return FReply::Handled();
}

FText FMDViewModelAssignmentEditorObjectCustomization::GetSelectedClassText() const
{
	if (const UClass* Class = GetCurrentViewModelClass())
	{
		return Class->GetDisplayNameText();
	}

	return INVTEXT("Select a ViewModel Class...");
}

FText FMDViewModelAssignmentEditorObjectCustomization::GetSelectedClassToolTipText() const
{
	if (const UClass* Class = GetCurrentViewModelClass())
	{
		return Class->GetToolTipText();
	}

	return INVTEXT("Select a ViewModel Class...");
}

void FMDViewModelAssignmentEditorObjectCustomization::RefreshDetails() const
{
	if (IDetailLayoutBuilder* LayoutBuilder = CachedDetailBuilder.Pin().Get())
	{
		LayoutBuilder->ForceRefreshDetails();
	}
}

void FMDViewModelAssignmentEditorObjectCustomization::OnProviderPropertyChanged() const
{
	OnAssignmentUpdated();
	RefreshDetails();
}

void FMDViewModelAssignmentEditorObjectCustomization::OnViewModelPropertyChanged() const
{
	if (UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
	{
		if (const UMDViewModelBase* ViewModelCDO = EditorObject->ViewModelClass.GetDefaultObject())
		{
			ViewModelCDO->OnViewModelSettingsPropertyChanged(EditorObject->ViewModelSettings, Dialog->GetWidgetBlueprint(), EditorObject->CreateAssignment().Assignment);
		}
	}

	OnAssignmentUpdated();

	RefreshDetails();
}

void FMDViewModelAssignmentEditorObjectCustomization::OnAssignmentUpdated() const
{
	if (UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
	{
		if (const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider))
		{
			Provider->OnAssignmentUpdated(EditorObject->ProviderSettings, Dialog->GetWidgetBlueprint(), EditorObject->CreateAssignment().Assignment);
		}
	}
}

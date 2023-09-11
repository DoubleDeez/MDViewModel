#include "Customizations/MDViewModelAssignmentEditorObjectCustomization.h"

#include "ClassViewerFilter.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/Engine.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDetailGroup.h"
#include "Kismet2/SClassPickerDialog.h"
#include "MDViewModelEditorConfig.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/MDViewModelAssignmentDialog.h"
#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"

namespace MDViewModelAssignmentEditorObjectCustomization_Private
{
	const FName VMHiddenPropertyMeta = TEXT("MDVMHidden");
	
	bool DoesStructHaveEditableProperties(const UScriptStruct* Struct)
	{
		for (TFieldIterator<const FProperty> It(Struct); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Edit) && !It->HasMetaData(VMHiddenPropertyMeta))
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
			bAllowAbstract = !Provider->DoesCreateViewModels();
		}
	}

	bool bAllowAbstract = false;
	TArray<FMDViewModelSupportedClass> ProviderSupportedViewModelClasses;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs) override
	{
		if (InClass == UMDViewModelBase::StaticClass() || !InClass->IsChildOf<UMDViewModelBase>())
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
		if (!InUnloadedClassData->IsChildOf(UMDViewModelBase::StaticClass()))
		{
			return false;
		}
		
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


FMDViewModelAssignmentEditorObjectCustomization::FMDViewModelAssignmentEditorObjectCustomization(TSharedRef<SMDViewModelAssignmentDialog> InDialog)
	: Dialog(InDialog)
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
		const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(EditorObject->ViewModelProvider);
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

		const TSharedRef<IPropertyHandle> ViewModelNameHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ViewModelInstanceName), UMDViewModelAssignmentEditorObject::StaticClass());
		const TSharedRef<IPropertyHandle> ViewModelTagHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ViewModelInstanceTag), UMDViewModelAssignmentEditorObject::StaticClass());
		DetailBuilder.EditDefaultProperty(ViewModelNameHandle)->Visibility(TAttribute<EVisibility>::CreateSP(this, &FMDViewModelAssignmentEditorObjectCustomization::GetViewModelNameVisibility));
		DetailBuilder.EditDefaultProperty(ViewModelTagHandle)->Visibility(TAttribute<EVisibility>::CreateSP(this, &FMDViewModelAssignmentEditorObjectCustomization::GetViewModelTagVisibility));
		
		const TSharedRef<IPropertyHandle> ViewModelSettingsHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ViewModelSettings), UMDViewModelAssignmentEditorObject::StaticClass());
		if (IsValid(Provider) && EditorObject->ViewModelSettings.IsValid() && MDViewModelAssignmentEditorObjectCustomization_Private::DoesStructHaveEditableProperties(EditorObject->ViewModelSettings.GetScriptStruct()))
		{
			if (Provider->DoesSupportViewModelSettings())
			{
				ViewModelSettingsHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FMDViewModelAssignmentEditorObjectCustomization::OnViewModelPropertyChanged));
				DetailBuilder.EditDefaultProperty(ViewModelSettingsHandle)->ShouldAutoExpand(true).OverrideResetToDefault(HideResetToDefault);
			}
			else if (Provider->DoesCreateViewModels())
			{
				// Provider doesn't initialize view models so we don't need to warn here.
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

		int32 CustomSortOrder = 9000;

		const FMDViewModelAssignment Assignment = EditorObject->CreateAssignment().Assignment;
		const TSharedRef<IPropertyHandle> ViewModelClassHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMDViewModelAssignmentEditorObject, ViewModelClass), UMDViewModelAssignmentEditorObject::StaticClass());
		if (IsValid(Provider))
		{
			Provider->OnProviderSettingsInitializedInEditor(EditorObject->ProviderSettings, Dialog->GetBlueprint(), Assignment);

			const FComboButtonStyle& ComboButtonStyle = FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("ComboButton");
			
			DetailBuilder.EditDefaultProperty(ViewModelClassHandle)->CustomWidget()
			.NameContent()
			[
				ViewModelClassHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				SNew(SButton)
				.ButtonStyle(&ComboButtonStyle.ButtonStyle)
				.OnClicked(this, &FMDViewModelAssignmentEditorObjectCustomization::OnClassPickerButtonClicked)
				.ToolTipText(this, &FMDViewModelAssignmentEditorObjectCustomization::GetSelectedClassToolTipText)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Visibility(EVisibility::HitTestInvisible)
						.Text(this, &FMDViewModelAssignmentEditorObjectCustomization::GetSelectedClassText)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					.Padding(ComboButtonStyle.DownArrowPadding)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						.VAlign(VAlign_Top)
						.Padding(FMargin(ComboButtonStyle.ShadowOffset.X, ComboButtonStyle.ShadowOffset.Y, 0, 0))
						[
							SNew(SImage)
							.Image(&ComboButtonStyle.DownArrowImage)
							.ColorAndOpacity(ComboButtonStyle.ShadowColorAndOpacity)
						]
						+ SOverlay::Slot()
						.VAlign(VAlign_Top)
						[
							SNew(SImage)
							.Image(&ComboButtonStyle.DownArrowImage)
							.ColorAndOpacity(FSlateColor::UseForeground())
						]
					]
				]
			];

			TArray<FText> ProviderIssues;
			Provider->ValidateProviderSettings(EditorObject->ProviderSettings, Dialog->GetBlueprint(), Assignment, ProviderIssues);

			if (!ProviderIssues.IsEmpty())
			{
				IDetailCategoryBuilder& ProviderValidationCategory = DetailBuilder.EditCategory(TEXT("Provider Validation"));
				ProviderValidationCategory.SetSortOrder(++CustomSortOrder);

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
			ViewModelCDO->ValidateViewModelSettings(EditorObject->ViewModelSettings, Dialog->GetBlueprint(), Assignment, ViewModelIssues);

			if (!ViewModelIssues.IsEmpty())
			{
				IDetailCategoryBuilder& ViewModelValidationCategory = DetailBuilder.EditCategory(TEXT("View Model Validation"));
				ViewModelValidationCategory.SetSortOrder(++CustomSortOrder);

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

			// Context object hinting
			if (IsValid(Provider) && Provider->DoesCreateViewModels())
			{
				UBlueprint* Blueprint = Dialog->GetBlueprint();

				TArray<TSubclassOf<UObject>> ProviderExpectedContextObjectClasses;
				Provider->GetExpectedContextObjectTypes(EditorObject->ProviderSettings, EditorObject->ViewModelSettings, Blueprint, ProviderExpectedContextObjectClasses);
				ProviderExpectedContextObjectClasses = ProviderExpectedContextObjectClasses.FilterByPredicate([](const TSubclassOf<UObject>& Class) { return Class != nullptr; });
				
				TArray<TSubclassOf<UObject>> ViewModelSupportedContextObjectClasses;
				ViewModelCDO->GetSupportedContextObjectTypes(EditorObject->ViewModelSettings, Blueprint, ViewModelSupportedContextObjectClasses);
				ViewModelSupportedContextObjectClasses = ViewModelSupportedContextObjectClasses.FilterByPredicate([](const TSubclassOf<UObject>& Class) { return Class != nullptr; });

				// Determine which provided classes match what the view model supports
				TSet<TSubclassOf<UObject>> ExactMatches;
				TSet<TSubclassOf<UObject>> PossibleMatches;

				for (const TSubclassOf<UObject>& VMContextClass : ViewModelSupportedContextObjectClasses)
				{
					for (const TSubclassOf<UObject>& ProvidedContextClass : ProviderExpectedContextObjectClasses)
					{
						if (VMContextClass == ProvidedContextClass)
						{
							ExactMatches.Add(VMContextClass);
						}
						else if (ProvidedContextClass->IsChildOf(VMContextClass) || ProvidedContextClass->ImplementsInterface(VMContextClass))
						{
							ExactMatches.Add(VMContextClass);
							ExactMatches.Add(ProvidedContextClass);
						}
						else if (VMContextClass->IsChildOf(ProvidedContextClass) || TSubclassOf<UInterface>(VMContextClass.Get()) != nullptr)
						{
							// The supported type is a child of the provided type so the actual provided type _could_ be the supported type
							// OR
							// The provided type _could_ implement an interface so it's a maybe
							PossibleMatches.Add(VMContextClass);
							PossibleMatches.Add(ProvidedContextClass);
						}
					}
				}
				
				IDetailCategoryBuilder& ContextObjectTypeCheckCategory = DetailBuilder.EditCategory(TEXT("Context Object Type Check"));
				ContextObjectTypeCheckCategory.SetSortOrder(++CustomSortOrder);

				IDetailGroup& Group = ContextObjectTypeCheckCategory.AddGroup(TEXT("ContextObjectClasses"), INVTEXT("Context Object Classes"), false, true);
				Group.HeaderRow()
				.NameContent()
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.Text(ProviderExpectedContextObjectClasses.Num() > 1 ? INVTEXT("Provided Context Object Types") : INVTEXT("Provided Context Object Type"))
					.ToolTipText(ProviderExpectedContextObjectClasses.Num() > 1 ? INVTEXT("The provider will initialize the view model with one of these context object types") : INVTEXT("The provider will initialize the view model with this context object type"))
				]
				.ValueContent()
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 8))
					.Text(ViewModelSupportedContextObjectClasses.Num() > 1 ? INVTEXT("Supported View Model Context Object Types") : INVTEXT("Supported View Model Context Object Type"))
					.ToolTipText(ViewModelSupportedContextObjectClasses.Num() > 1 ? INVTEXT("The view model supports being initialized with any of these context object types") : INVTEXT("The view model can be initialized with this context object type"))
				];

				auto SortingFunc = [&] (const TSubclassOf<UObject>& A, const TSubclassOf<UObject>& B)
				{
					if (ExactMatches.Contains(A) != ExactMatches.Contains(B))
					{
						return ExactMatches.Contains(A);
					}

					if (PossibleMatches.Contains(A) != PossibleMatches.Contains(B))
					{
						return PossibleMatches.Contains(A);
					}

					return A->GetDisplayNameText().CompareTo(B->GetDisplayNameText()) < 0;
				};
				
				ProviderExpectedContextObjectClasses.StableSort(SortingFunc);
				ViewModelSupportedContextObjectClasses.StableSort(SortingFunc);

				const FSlateFontInfo RegularStyle = FCoreStyle::GetDefaultFontStyle("Regular", 8);
				const FSlateFontInfo BoldStyle = FCoreStyle::GetDefaultFontStyle("Bold", 8);

				const FSlateColor ExactMatchColor = FColorList::LimeGreen;
				const FSlateColor PossibleMatchColor = FLinearColor(1.f, 0.65f, 0.f);
				const FSlateColor NoMatchColor = FLinearColor(0.5f, 0.5f, 0.5f);

				const int32 NumRows = FMath::Max(ProviderExpectedContextObjectClasses.Num(), ViewModelSupportedContextObjectClasses.Num());
				for (int32 i = 0; i < NumRows; ++i)
				{
					const TSubclassOf<UObject> ProvidedContextClass = ProviderExpectedContextObjectClasses.IsValidIndex(i) ? ProviderExpectedContextObjectClasses[i] : nullptr;
					const TSubclassOf<UObject> VMContextClass = ViewModelSupportedContextObjectClasses.IsValidIndex(i) ? ViewModelSupportedContextObjectClasses[i] : nullptr;
					const bool bIsProvidedClassInMatches = ExactMatches.Contains(ProvidedContextClass) || PossibleMatches.Contains(VMContextClass);
					const bool bIsVMClassInMatches = ExactMatches.Contains(VMContextClass) || PossibleMatches.Contains(VMContextClass);
					
					Group.AddWidgetRow()
					.NameContent()
					[
						IsValid(ProvidedContextClass)
							? SNew(STextBlock)
							.Text(ProvidedContextClass->GetDisplayNameText())
							.Font(bIsProvidedClassInMatches ? BoldStyle : RegularStyle)
							.ColorAndOpacity(bIsProvidedClassInMatches ? (ExactMatches.Contains(ProvidedContextClass) ? ExactMatchColor : PossibleMatchColor) : NoMatchColor)
							: SNew(STextBlock)
					]
					.ValueContent()
					[
						IsValid(VMContextClass)
							? SNew(STextBlock)
							.Text(VMContextClass->GetDisplayNameText())
							.Font(bIsVMClassInMatches ? BoldStyle : RegularStyle)
							.ColorAndOpacity(bIsVMClassInMatches ? (ExactMatches.Contains(VMContextClass) ? ExactMatchColor : PossibleMatchColor) : NoMatchColor)
							: SNew(STextBlock)
					];
				}
				
				const TTuple<FText, FSlateColor> ContextObjectHint = [&]() -> TTuple<FText, FSlateColor>
				{
					if (ProviderExpectedContextObjectClasses.IsEmpty())
					{
						return { INVTEXT("This Provider does not specify its supplied context objects for the current settings."), NoMatchColor };
					}
					
					if (ViewModelSupportedContextObjectClasses.IsEmpty())
					{
						return { INVTEXT("This view model does not specify its supported context objects for the current settings. Override GetSupportedContextObjectTypes to specify them."), NoMatchColor };
					}
					
					if (PossibleMatches.IsEmpty() && ExactMatches.IsEmpty())
					{
						return { INVTEXT("This view model does not support any of the provided context objects."), NoMatchColor };
					}

					if (ExactMatches.IsEmpty())
					{
						return { INVTEXT("This view model might support the provided context object, compatibility can only be determined at runtime"), PossibleMatchColor };
					}
					
					bool bAllClassesHaveExactMatch = true;
					for (const TSubclassOf<UObject>& ProvidedContextClass : ProviderExpectedContextObjectClasses)
					{
						if (!ExactMatches.Contains(ProvidedContextClass))
						{
							bAllClassesHaveExactMatch = false;
							break;
						}
					}

					if (bAllClassesHaveExactMatch)
					{
						if (ProviderExpectedContextObjectClasses.Num() > 1)
						{
							return { INVTEXT("The selected view model supports all the potentially provided context objects."), ExactMatchColor };
						}
						else
						{
							return { INVTEXT("The selected view model supports the provided context object."), ExactMatchColor };
						}
					}

					return { INVTEXT("The selected view model only supports some of the potentially provided context objects, compatibility can only be determined at runtime."), PossibleMatchColor };
				}();
				
				ContextObjectTypeCheckCategory.AddCustomRow(ContextObjectHint.Key).WholeRowContent()
				[
					SNew(SBox)
					.Padding(4.f)
					[
						SNew(STextBlock)
						.Text(ContextObjectHint.Key)
						.ColorAndOpacity(ContextObjectHint.Value)
						.AutoWrapText(true)
					]
				];
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
			return Provider->GetDescription(EditorObject->ProviderSettings);
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
					Provider->GetDescription({}),
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
			Provider->OnProviderSettingsInitializedInEditor(EditorObject->ProviderSettings, Dialog->GetBlueprint(), EditorObject->CreateAssignment().Assignment);
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
			ClassPickerOptions.ClassFilters.Add(MakeShared<FMDViewModelProviderClassFilter>(Provider));
			ClassPickerOptions.InitiallySelectedClass = GetCurrentViewModelClass();
			ClassPickerOptions.NameTypeToDisplay = GetDefault<UMDViewModelEditorConfig>()->GetNameTypeToDisplay();

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
			ViewModelCDO->OnViewModelSettingsPropertyChanged(EditorObject->ViewModelSettings, Dialog->GetBlueprint(), EditorObject->CreateAssignment().Assignment);
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
			Provider->OnAssignmentUpdated(EditorObject->ProviderSettings, Dialog->GetBlueprint(), EditorObject->CreateAssignment().Assignment);
		}
	}
}

EVisibility FMDViewModelAssignmentEditorObjectCustomization::GetViewModelNameVisibility() const
{
	if (const UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
	{
		if (EditorObject->bOverrideName && GetDefault<UMDViewModelEditorConfig>()->bUseGameplayTagsForViewModelNaming)
		{
			return EVisibility::Collapsed;
		}
		else
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FMDViewModelAssignmentEditorObjectCustomization::GetViewModelTagVisibility() const
{
	if (const UMDViewModelAssignmentEditorObject* EditorObject = EditorObjectPtr.Get())
	{
		if (EditorObject->bOverrideName && GetDefault<UMDViewModelEditorConfig>()->bUseGameplayTagsForViewModelNaming)
		{
			return EVisibility::Visible;
		}
		else
		{
			return EVisibility::Collapsed;
		}
	}

	return EVisibility::Collapsed;
}

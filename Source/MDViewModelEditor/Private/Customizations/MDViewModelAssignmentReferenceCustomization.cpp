#include "Customizations/MDViewModelAssignmentReferenceCustomization.h"

#include "ClassViewerFilter.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "MDViewModelModule.h"
#include "PropertyHandle.h"
#include "Blueprint/UserWidget.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "ViewModel/MDViewModelBase.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"

class FMDViewModelClassFilter : public IClassViewerFilter
{

public:
	FMDViewModelClassFilter(UClass* WidgetOwnerClass)
		: WidgetClass(WidgetOwnerClass)
	{
		if (WidgetClass.IsValid())
		{
			const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
			TSet<TSubclassOf<UMDViewModelBase>> ViewModelClasses;
			ViewModelModule.GetViewModelClassesForWidgetClass(WidgetClass.Get(), ViewModelClasses);

			Algo::Transform(ViewModelClasses, SupportedViewModelClasses, [](const TSubclassOf<UMDViewModelBase>& Class){ return Class.Get(); });
		}
	}

	TWeakObjectPtr<UClass> WidgetClass;

	TSet<const UClass*> SupportedViewModelClasses;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs ) override
	{
		return InFilterFuncs->IfInClassesSet(SupportedViewModelClasses, InClass) == EFilterReturn::Passed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const class IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< class FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return InFilterFuncs->IfInClassesSet(SupportedViewModelClasses, InUnloadedClassData) == EFilterReturn::Passed;
	}

};

TSharedRef<IPropertyTypeCustomization> FMDViewModelAssignmentReferenceCustomization::MakeInstance()
{
	return MakeShared<FMDViewModelAssignmentReferenceCustomization>();
}

void FMDViewModelAssignmentReferenceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle,
                                                                   FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = PropertyHandle;
}

void FMDViewModelAssignmentReferenceCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	uint32 NumChildren = 0;
	PropertyHandle->GetNumChildren(NumChildren);
	for (uint32 i = 0; i < NumChildren; ++i)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(i);
		if (!ChildHandle.IsValid())
		{
			continue;
		}

		if (ChildHandle->GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(FMDViewModelAssignmentReference, ViewModelClass))
		{
			ClassHandle = ChildHandle;

			ChildBuilder.AddCustomRow(FText::GetEmpty())
			.NameContent()
			[
				ChildHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("ComboButton").ButtonStyle)
				.OnClicked(this, &FMDViewModelAssignmentReferenceCustomization::OnClassPickerButtonClicked)
				.Text(this, &FMDViewModelAssignmentReferenceCustomization::GetSelectedClassText)
				.ToolTipText(this, &FMDViewModelAssignmentReferenceCustomization::GetSelectedClassToolTipText)
			];
		}
		else
		{
			// TODO - Disable bIsViewModelName if view model class not selected
			// TODO - Change to a dropdown of the registered viewmodel names based on the selected ViewModelClass
			const bool bIsViewModelName = ChildHandle->GetProperty()->GetFName() == GET_MEMBER_NAME_CHECKED(FMDViewModelAssignmentReference, ViewModelName);
			ChildBuilder.AddProperty(ChildHandle.ToSharedRef()).IsEnabled(true);
		}
	}
}

FMDViewModelAssignmentReference* FMDViewModelAssignmentReferenceCustomization::GetAssignmentReference() const
{
	void* DataPtr = nullptr;
	if (StructHandle->GetValueData(DataPtr) == FPropertyAccess::Result::Success)
	{
		return static_cast<FMDViewModelAssignmentReference*>(DataPtr);
	}

	return nullptr;
}

UClass* FMDViewModelAssignmentReferenceCustomization::GetWidgetOwnerClass() const
{
	if (const FMDViewModelAssignmentReference* VMAssignment = GetAssignmentReference())
	{
		if (VMAssignment->OnGetWidgetClass.IsBound())
		{
			return VMAssignment->OnGetWidgetClass.Execute();
		}
		else
		{
			return UUserWidget::StaticClass();
		}
	}

	return nullptr;
}

UClass* FMDViewModelAssignmentReferenceCustomization::GetCurrentViewModelClass() const
{
	if (const FMDViewModelAssignmentReference* VMAssignment = GetAssignmentReference())
	{
		return VMAssignment->ViewModelClass.LoadSynchronous();
	}

	return nullptr;
}

FReply FMDViewModelAssignmentReferenceCustomization::OnClassPickerButtonClicked() const
{
	FClassViewerInitializationOptions ClassPickerOptions;
	ClassPickerOptions.bShowNoneOption = true;
	ClassPickerOptions.ClassFilters.Add(MakeShareable(new FMDViewModelClassFilter(GetWidgetOwnerClass())));
	ClassPickerOptions.InitiallySelectedClass = GetCurrentViewModelClass();

	UClass* Class = nullptr;
	if (SClassPickerDialog::PickClass(ClassHandle->GetPropertyDisplayName(), ClassPickerOptions, Class, UClass::StaticClass()))
	{
		ClassHandle->SetValue(Class);
	}

	return FReply::Handled();
}

FText FMDViewModelAssignmentReferenceCustomization::GetSelectedClassText() const
{
	if (const FMDViewModelAssignmentReference* VMAssignment = GetAssignmentReference())
	{
		if (VMAssignment->ViewModelClass != nullptr)
		{
			return VMAssignment->ViewModelClass->GetDisplayNameText();
		}
	}

	return INVTEXT("Select a ViewModel Class...");
}

FText FMDViewModelAssignmentReferenceCustomization::GetSelectedClassToolTipText() const
{
	if (const FMDViewModelAssignmentReference* VMAssignment = GetAssignmentReference())
	{
		if (VMAssignment->ViewModelClass != nullptr)
		{
			return VMAssignment->ViewModelClass->GetToolTipText();
		}
	}

	return INVTEXT("Select a ViewModel Class...");
}

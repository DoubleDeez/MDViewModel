#include "Customizations/MDViewModelAssignmentReferenceCustomization.h"

#include "Blueprint/UserWidget.h"
#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "MDViewModelModule.h"
#include "PropertyHandle.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "ViewModel/MDViewModelBase.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"


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
	const FMDViewModelAssignmentReference* Reference = GetAssignmentReference();
	if (Reference != nullptr && Reference->OnGetWidgetClass.IsBound())
	{
		ChildBuilder.AddCustomRow(FText::GetEmpty())
		.NameContent()
		[
			StructHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SComboButton)
			.OnGetMenuContent(this, &FMDViewModelAssignmentReferenceCustomization::MakeAssignmentMenu)
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FMDViewModelAssignmentReferenceCustomization::GetSelectedAssignmentText)
			]
		];
	}
	else
	{
		ChildBuilder.AddCustomRow(FText::GetEmpty()).WholeRowContent()
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(FText::Format(INVTEXT("{0}.OnGetWidgetClass must be bound to set the assignment reference in the editor"), StructHandle->GetPropertyDisplayName()))
		];
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

TSharedRef<SWidget> FMDViewModelAssignmentReferenceCustomization::MakeAssignmentMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);
	if (UClass* WidgetClass = GetWidgetOwnerClass())
	{
		const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
		ViewModelModule.GetViewModelAssignmentsForWidgetClass(WidgetClass, ViewModelAssignments);

		for (const auto& Pair : ViewModelAssignments)
		{
			MenuBuilder.AddMenuEntry(
				FText::Format(INVTEXT("{0} ({1})"), Pair.Key.ViewModelClass->GetDisplayNameText(), FText::FromName(Pair.Key.ViewModelName)),
				Pair.Key.ViewModelClass->GetToolTipText(),
				FSlateIcon(),
				FExecuteAction::CreateSP(this, &FMDViewModelAssignmentReferenceCustomization::SetSelectedAssignment, Pair.Key)
			);
		}
	}

	return MenuBuilder.MakeWidget();
}

void FMDViewModelAssignmentReferenceCustomization::SetSelectedAssignment(FMDViewModelAssignment Assignment) const
{
	if (FMDViewModelAssignmentReference* VMAssignment = GetAssignmentReference())
	{
		StructHandle->NotifyPreChange();
		VMAssignment->ViewModelClass = Assignment.ViewModelClass;
		VMAssignment->ViewModelName = Assignment.ViewModelName;
		StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
		StructHandle->NotifyFinishedChangingProperties();
	}
}

FText FMDViewModelAssignmentReferenceCustomization::GetSelectedAssignmentText() const
{
	if (const FMDViewModelAssignmentReference* VMAssignment = GetAssignmentReference())
	{
		if (!VMAssignment->ViewModelClass.IsNull() && IsValid(VMAssignment->ViewModelClass.Get()))
		{
			return FText::Format(INVTEXT("{0} ({1})"), VMAssignment->ViewModelClass->GetDisplayNameText(), FText::FromName(VMAssignment->ViewModelName));
		}
	}

	return INVTEXT("Select an Assignment...");
}

#include "Customizations/MDViewModelAssignmentReferenceCustomization.h"

#include "Blueprint/UserWidget.h"
#include "DetailWidgetRow.h"
#include "EdGraphSchema_K2.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "MDViewModelModule.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprint.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"

namespace MDViewModelAssignmentReferenceCustomization_Private
{
	bool IsSelfPin(const UEdGraphPin& Pin)
	{
		if (Pin.GetSchema()->IsSelfPin(Pin))
		{
			return true;
		}

		if (Pin.PinType.PinSubCategory == UEdGraphSchema_K2::PSC_Self)
		{
			return true;
		}
		
		const FString DefaultToSelfString = Pin.GetOwningNode()->GetPinMetaData(Pin.PinName, FBlueprintMetadata::MD_DefaultToSelf);
		if (DefaultToSelfString == Pin.PinName.ToString())
		{
			return true;
		}

		return false;
	}
}


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

/**********************************************************************************************************************/

TSharedRef<SWidget> SMDViewModelAssignmentReferenceGraphPin::GetDefaultValueWidget()
{
	// Can't set default value of reference types that aren't auto create
	if (GraphPinObj->PinType.bIsReference && !UEdGraphSchema_K2::IsAutoCreateRefTerm(GraphPinObj))
	{
		return SGraphPin::GetDefaultValueWidget();
	}
	
	return SNew(SComboButton)
		   .Visibility(this, &SGraphPin::GetDefaultValueVisibility)
		   .OnGetMenuContent(this, &SMDViewModelAssignmentReferenceGraphPin::MakeAssignmentMenu)
		   .ButtonContent()
		   [
			   SNew(STextBlock)
			   .Text(this, &SMDViewModelAssignmentReferenceGraphPin::GetSelectedAssignmentText)
		   ];
}

UClass* SMDViewModelAssignmentReferenceGraphPin::GetWidgetOwnerClass() const
{
	static const FName VMAssignmentMeta = TEXT("VMAssignment");
	if (UEdGraphNode* OwningNode = GraphPinObj->GetOwningNode())
	{
		for (const UEdGraphPin* Pin : OwningNode->GetAllPins())
		{
			// Is this pin our Widget?
			const FString AssignmentMeta = OwningNode->GetPinMetaData(Pin->GetFName(), VMAssignmentMeta);
			if (AssignmentMeta != GraphPinObj->GetName())
			{
				continue;
			}
			
			UClass* PinClass = ResolveWidgetClassFromPin(Pin);
			if (IsValid(PinClass))
			{
				return PinClass;
			}
		}
	}
	
	return nullptr;
}

UClass* SMDViewModelAssignmentReferenceGraphPin::ResolveWidgetClassFromPin(const UEdGraphPin* Pin) const
{
	if (Pin == nullptr)
	{
		return nullptr;
	}
	
	// If the pin connected, check it instead since its class might be more specific
	Pin = (Pin->LinkedTo.IsEmpty() || Pin->LinkedTo[0] == nullptr) ? Pin : Pin->LinkedTo[0];

	// If it's a self pin, get the Widget BP generated class
	if (MDViewModelAssignmentReferenceCustomization_Private::IsSelfPin(*Pin))
	{
		if (const UWidgetBlueprint* WidgetBP = GraphPinObj->GetOwningNode()->GetTypedOuter<UWidgetBlueprint>())
		{
			return WidgetBP->GeneratedClass;
		}
	}
	
	UClass* PinClass = Cast<UClass>(Pin->PinType.PinSubCategoryObject.Get());
	if (!IsValid(PinClass))
	{
		return nullptr;
	}

	if (PinClass->IsChildOf<UUserWidget>())
	{
		return PinClass;
	}
	
	return nullptr;
}

TSharedRef<SWidget> SMDViewModelAssignmentReferenceGraphPin::MakeAssignmentMenu()
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
				FExecuteAction::CreateSP(this, &SMDViewModelAssignmentReferenceGraphPin::SetSelectedAssignment, Pair.Key)
			);
		}
	}

	return MenuBuilder.MakeWidget();
}

void SMDViewModelAssignmentReferenceGraphPin::SetSelectedAssignment(FMDViewModelAssignment Assignment) const
{
	FString FinalValue;
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = Assignment.ViewModelClass;
	AssignmentReference.ViewModelName = Assignment.ViewModelName;

	FMDViewModelAssignmentReference::StaticStruct()->ExportText(FinalValue, &AssignmentReference, &AssignmentReference, nullptr, PPF_SerializedAsImportText, nullptr);

	if (FinalValue != GraphPinObj->GetDefaultAsString())
	{
		const FScopedTransaction Transaction(NSLOCTEXT("GraphEditor", "ChangePinValue", "Change Pin Value"));
		GraphPinObj->Modify();
		GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, FinalValue);
	}
}

FText SMDViewModelAssignmentReferenceGraphPin::GetSelectedAssignmentText() const
{
	const FString DefaultString = GraphPinObj->GetDefaultAsString();
	if (!DefaultString.IsEmpty())
	{
		FMDViewModelAssignmentReference Assignment;
		UScriptStruct* PinLiteralStructType = FMDViewModelAssignmentReference::StaticStruct();
		PinLiteralStructType->ImportText(*DefaultString, &Assignment, nullptr, PPF_SerializedAsImportText, GError, PinLiteralStructType->GetName());

		return FText::Format(INVTEXT("{0} ({1})"), Assignment.ViewModelClass.LoadSynchronous()->GetDisplayNameText(), FText::FromName(Assignment.ViewModelName));
	}

	return INVTEXT("Select an Assignment...");
}

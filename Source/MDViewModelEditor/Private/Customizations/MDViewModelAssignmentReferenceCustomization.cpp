#include "Customizations/MDViewModelAssignmentReferenceCustomization.h"

#include "DetailWidgetRow.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "Logging/TokenizedMessage.h"
#include "Runtime/Launch/Resources/Version.h"
#include "PropertyHandle.h"
#include "ScopedTransaction.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Util/MDViewModelGraphStatics.h"
#include "Util/MDViewModelUtils.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelTab/MDViewModelAssignmentEditorObject.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboButton.h"

DECLARE_DELEGATE_OneParam(FMDVMAROnAssignmentSelected, FMDViewModelAssignment);

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

	TSharedRef<SWidget> MakeAssignmentMenu(const TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& ViewModelAssignments, bool bShowNonManualAssignments, FMDVMAROnAssignmentSelected&& Callback)
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		TArray<FMDViewModelAssignment> ManualAssignments;
		TArray<FMDViewModelAssignment> NonManualAssignments;
		for (const auto& Pair : ViewModelAssignments)
		{
			const UMDViewModelProviderBase* Provider = MDViewModelUtils::FindViewModelProvider(Pair.Key.ProviderTag);
			if (!IsValid(Provider))
			{
				continue;
			}

			if (Provider->DoesAllowManualSetting())
			{
				ManualAssignments.Add(Pair.Key);
			}
			else
			{
				NonManualAssignments.Add(Pair.Key);
			}
		}

		auto SortFunc = [](const FMDViewModelAssignment& A, const FMDViewModelAssignment& B)
		{
			if (!IsValid(A.ViewModelClass) || !IsValid(B.ViewModelClass))
			{
				return IsValid(A.ViewModelClass);
			}

			const FText ADisplayName = FText::Format(INVTEXT("{0} ({1})"), A.ViewModelClass->GetDisplayNameText(), FText::FromName(A.ViewModelName));
			const FText BDisplayName = FText::Format(INVTEXT("{0} ({1})"), B.ViewModelClass->GetDisplayNameText(), FText::FromName(B.ViewModelName));

			return A.ViewModelClass->GetDisplayNameText().CompareTo(B.ViewModelClass->GetDisplayNameText()) < 0;
		};
		ManualAssignments.Sort(SortFunc);
		NonManualAssignments.Sort(SortFunc);

		auto PopulateMenu = [&MenuBuilder, Callback = MoveTemp(Callback)](const TArray<FMDViewModelAssignment>& Assignments)
		{
			for (const FMDViewModelAssignment& Assignment : Assignments)
			{
				MenuBuilder.AddMenuEntry(
					FText::Format(INVTEXT("{0} ({1})"), Assignment.ViewModelClass->GetDisplayNameText(), FText::FromName(Assignment.ViewModelName)),
					Assignment.ViewModelClass->GetToolTipText(),
					FSlateIcon(),
					FExecuteAction::CreateLambda([Assignment, Callback]()
					{
						Callback.ExecuteIfBound(Assignment);
					})
				);
			}
		};

		MenuBuilder.BeginSection(TEXT("Manual"), INVTEXT("Manual Assignments"));
		PopulateMenu(ManualAssignments);
		MenuBuilder.EndSection();

		if (bShowNonManualAssignments)
		{
			MenuBuilder.BeginSection(TEXT("NonManual"), INVTEXT("Non-Manual Assignments"));
			PopulateMenu(NonManualAssignments);
			MenuBuilder.EndSection();
		}

		return MenuBuilder.MakeWidget();
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
	if (Reference != nullptr && Reference->GetBoundObjectClass() != nullptr)
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
			.Text(FText::Format(INVTEXT("{0}.OnGetBoundObjectClass must be bound to set the assignment reference in the editor"), StructHandle->GetPropertyDisplayName()))
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

UClass* FMDViewModelAssignmentReferenceCustomization::GetBoundObjectClass() const
{
	if (const FMDViewModelAssignmentReference* VMAssignment = GetAssignmentReference())
	{
		return VMAssignment->GetBoundObjectClass();
	}

	return nullptr;
}

TSharedRef<SWidget> FMDViewModelAssignmentReferenceCustomization::MakeAssignmentMenu()
{
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
	if (UClass* BoundObjectClass = GetBoundObjectClass())
	{
		TArray<UPackage*> AssignmentPackages;
		StructHandle->GetOuterPackages(AssignmentPackages);

		TArray<UObject*> AssignmentObjects;
		StructHandle->GetOuterObjects(AssignmentObjects);

		const UMDViewModelAssignmentEditorObject* EditorObject = AssignmentObjects.Num() == 1 ? Cast<UMDViewModelAssignmentEditorObject>(AssignmentObjects[0]) : nullptr;

		const UBlueprint* BoundBlueprint = Cast<UBlueprint>(BoundObjectClass->ClassGeneratedBy);
		const bool bIsReferenceInSamePackage = IsValid(BoundBlueprint) && AssignmentPackages.Num() == 1 && BoundObjectClass->GetOuterUPackage() == AssignmentPackages[0];
		const bool bIsReferenceFromEditorObjectInSamePackage = IsValid(EditorObject) && EditorObject->BlueprintPtr.Get() == BoundBlueprint;

		if (bIsReferenceInSamePackage || bIsReferenceFromEditorObjectInSamePackage)
		{
			FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(BoundBlueprint, ViewModelAssignments);
		}
		else
		{
			MDViewModelUtils::GetViewModelAssignments(BoundObjectClass, ViewModelAssignments);
		}
	}

	auto Delegate = FMDVMAROnAssignmentSelected::CreateSP(this, &FMDViewModelAssignmentReferenceCustomization::SetSelectedAssignment);
	return MDViewModelAssignmentReferenceCustomization_Private::MakeAssignmentMenu(ViewModelAssignments, StructHandle->GetBoolMetaData(TEXT("MDVMShowNonManual")), MoveTemp(Delegate));
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

void SMDViewModelAssignmentReferenceGraphPin::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphPin::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	ValidateDefaultValue();
}

void SMDViewModelAssignmentReferenceGraphPin::GetWidgetViewModelAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments) const
{
	const UEdGraphPin* WidgetPin = GetWidgetPin();
	if (WidgetPin == nullptr)
	{
		return;
	}

	const UBlueprint* Blueprint = GraphPinObj->GetOwningNode()->GetTypedOuter<UBlueprint>();

	// If it's a self pin, we can get the up-to-date assignments that haven't been compiled yet
	if (Blueprint != nullptr && MDViewModelAssignmentReferenceCustomization_Private::IsSelfPin(*WidgetPin))
	{
		FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(Blueprint, OutViewModelAssignments);
	}
	else
	{
		UClass* ObjectClass = Cast<UClass>(WidgetPin->PinType.PinSubCategoryObject.Get());
		MDViewModelUtils::GetViewModelAssignments(ObjectClass, OutViewModelAssignments);
	}
}

const UEdGraphPin* SMDViewModelAssignmentReferenceGraphPin::GetWidgetPin() const
{
	static const FName VMAssignmentMeta = TEXT("VMAssignment");
	if (UEdGraphNode* OwningNode = GraphPinObj->GetOwningNode())
	{
		for (const UEdGraphPin* Pin : OwningNode->GetAllPins())
		{
			const UClass* ObjectClass = Cast<UClass>(Pin->PinType.PinSubCategoryObject.Get());
			if (ObjectClass == nullptr)
			{
				continue;
			}

			// Is this pin our Widget?
			const FString AssignmentMeta = OwningNode->GetPinMetaData(Pin->GetFName(), VMAssignmentMeta);
			if (AssignmentMeta != GraphPinObj->GetName())
			{
				continue;
			}

			// If the pin connected, check it instead since its class might be more specific
			return (Pin->LinkedTo.IsEmpty() || Pin->LinkedTo[0] == nullptr) ? Pin : Pin->LinkedTo[0];
		}
	}

	return nullptr;
}

const UEdGraphPin* SMDViewModelAssignmentReferenceGraphPin::GetViewModelPin() const
{
	static const FName VMAssignmentMeta = TEXT("VMAssignment");
	if (UEdGraphNode* OwningNode = GraphPinObj->GetOwningNode())
	{
		for (const UEdGraphPin* Pin : OwningNode->GetAllPins())
		{
			TSubclassOf<UMDViewModelBase> ViewModelClass = Cast<UClass>(Pin->PinType.PinSubCategoryObject.Get());
			if (ViewModelClass == nullptr)
			{
				// Not a view model pin
				continue;
			}

			// Is this pin our view model?
			const FString AssignmentMeta = OwningNode->GetPinMetaData(Pin->GetFName(), VMAssignmentMeta);
			if (AssignmentMeta != GraphPinObj->GetName())
			{
				continue;
			}

			// If the pin connected, check it instead since its class might be more specific
			return (Pin->LinkedTo.IsEmpty() || Pin->LinkedTo[0] == nullptr) ? Pin : Pin->LinkedTo[0];
		}
	}

	return nullptr;
}

UClass* SMDViewModelAssignmentReferenceGraphPin::GetConnectedObjectClass() const
{
	const UEdGraphPin* WidgetPin = GetWidgetPin();
	if (WidgetPin == nullptr)
	{
		return nullptr;
	}

	UClass* ObjectClass = Cast<UClass>(WidgetPin->PinType.PinSubCategoryObject.Get());

	const UBlueprint* Blueprint = GraphPinObj->GetOwningNode()->GetTypedOuter<UBlueprint>();
	if (Blueprint != nullptr && MDViewModelAssignmentReferenceCustomization_Private::IsSelfPin(*WidgetPin))
	{
		ObjectClass = (Blueprint->SkeletonGeneratedClass) ? Blueprint->SkeletonGeneratedClass : Blueprint->GeneratedClass;
	}

	return ObjectClass;
}

TSubclassOf<UMDViewModelBase> SMDViewModelAssignmentReferenceGraphPin::GetConnectedViewModelClass() const
{
	const UEdGraphPin* ViewModelPin = GetViewModelPin();
	if (ViewModelPin == nullptr)
	{
		return nullptr;
	}

	return Cast<UClass>(ViewModelPin->PinType.PinSubCategoryObject.Get());
}

TSharedRef<SWidget> SMDViewModelAssignmentReferenceGraphPin::MakeAssignmentMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
	GetWidgetViewModelAssignments(ViewModelAssignments);

	const bool bShowNonManualAssignments = GraphPinObj != nullptr && GraphPinObj->GetOwningNode() != nullptr
		&& GraphPinObj->GetOwningNode()->GetPinMetaData(GraphPinObj->GetFName(), TEXT("MDVMShowNonManual")).ToBool();

	auto Delegate = FMDVMAROnAssignmentSelected::CreateSP(this, &SMDViewModelAssignmentReferenceGraphPin::SetSelectedAssignment);
	return MDViewModelAssignmentReferenceCustomization_Private::MakeAssignmentMenu(ViewModelAssignments, bShowNonManualAssignments, MoveTemp(Delegate));
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

	if (UEdGraphNode* OwningNode = GraphPinObj->GetOwningNode())
	{
		OwningNode->ReconstructNode();
	}
}

FMDViewModelAssignmentReference SMDViewModelAssignmentReferenceGraphPin::GetSelectedAssignment() const
{
	FMDViewModelAssignmentReference Assignment;

	const FString DefaultString = GraphPinObj->GetDefaultAsString();
	if (!DefaultString.IsEmpty())
	{
		UScriptStruct* PinLiteralStructType = FMDViewModelAssignmentReference::StaticStruct();
		PinLiteralStructType->ImportText(*DefaultString, &Assignment, nullptr, PPF_SerializedAsImportText, GError, PinLiteralStructType->GetName());
	}

	return Assignment;
}

FText SMDViewModelAssignmentReferenceGraphPin::GetSelectedAssignmentText() const
{
	const FMDViewModelAssignmentReference Assignment = GetSelectedAssignment();
	if (Assignment.IsAssignmentValid())
	{
		return FText::Format(INVTEXT("{0} ({1})"), Assignment.ViewModelClass.LoadSynchronous()->GetDisplayNameText(), FText::FromName(Assignment.ViewModelName));
	}

	return INVTEXT("Select an Assignment...");
}

void SMDViewModelAssignmentReferenceGraphPin::ValidateDefaultValue() const
{
	const UClass* ObjectClass = nullptr;

	enum class EAssignmentErrorType : uint8
	{
		None,
		AssignmentNotFound,
		AssignmentNotCompatible
	};

	const FMDViewModelAssignmentReference AssignmentReference = GetSelectedAssignment();
	const TSubclassOf<UMDViewModelBase> ViewModelClass = GetConnectedViewModelClass();
	const TSubclassOf<UMDViewModelBase> AssignedClass = AssignmentReference.ViewModelClass.LoadSynchronous();

	const EAssignmentErrorType ErrorType = [&]()
	{
		if (GraphPinObj->HasAnyConnections() || GraphPinObj->GetDefaultAsString().IsEmpty())
		{
			return EAssignmentErrorType::None;
		}

		if (GraphPinObj->PinType.bIsReference && !UEdGraphSchema_K2::IsAutoCreateRefTerm(GraphPinObj))
		{
			return EAssignmentErrorType::None;
		}

		// Only validate a valid assignment object
		if (!AssignmentReference.IsAssignmentValid())
		{
			return EAssignmentErrorType::None;
		}

		ObjectClass = GetConnectedObjectClass();

		if (ObjectClass == nullptr)
		{
			return EAssignmentErrorType::None;
		}

		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
		GetWidgetViewModelAssignments(ViewModelAssignments);

		for (const auto& Pair : ViewModelAssignments)
		{
			if (Pair.Key.ViewModelClass == AssignmentReference.ViewModelClass.Get()
				&& Pair.Key.ViewModelName == AssignmentReference.ViewModelName)
			{
				// If a view model has been tagged for validation, check that it's class is compatible with the assignment
				if (ViewModelClass != nullptr && !AssignedClass->IsChildOf(ViewModelClass) && !ViewModelClass->IsChildOf(AssignedClass))
				{
					return EAssignmentErrorType::AssignmentNotCompatible;
				}

				return EAssignmentErrorType::None;
			}
		}

		return EAssignmentErrorType::AssignmentNotFound;
	}();

	if (UEdGraphNode* OwningNode = GraphPinObj->GetOwningNode())
	{
		const bool bShowError = ErrorType != EAssignmentErrorType::None;
		bool bDidChange = bShowError != OwningNode->bHasCompilerMessage;
		if (ErrorType == EAssignmentErrorType::AssignmentNotFound)
		{
			bDidChange |= !OwningNode->ErrorMsg.StartsWith(TEXT("ViewModelAssignmentNotFound: "));
			bDidChange |= OwningNode->ErrorType != EMessageSeverity::Error;
			OwningNode->ErrorType = EMessageSeverity::Error;
			OwningNode->ErrorMsg = FString::Printf(TEXT("ViewModelAssignmentNotFound: The specified assignment does not exist on %s"), *ObjectClass->GetDisplayNameText().ToString());
			OwningNode->bHasCompilerMessage = true;
		}
		else if (ErrorType == EAssignmentErrorType::AssignmentNotCompatible)
		{
			bDidChange |= !OwningNode->ErrorMsg.StartsWith(TEXT("ViewModelAssignmentNotCompatible: "));
			bDidChange |= OwningNode->ErrorType != EMessageSeverity::Error;
			OwningNode->ErrorType = EMessageSeverity::Error;
			OwningNode->ErrorMsg = FString::Printf(TEXT("ViewModelAssignmentNotCompatible: The connected view model is of type [%s] but is not compatible with assignment's type [%s]."), *ViewModelClass->GetDisplayNameText().ToString(), *AssignedClass->GetDisplayNameText().ToString());
			OwningNode->bHasCompilerMessage = true;
		}
		else if (OwningNode->ErrorMsg.StartsWith(TEXT("ViewModelAssignment")))
		{
			bDidChange = true;
			OwningNode->ErrorType = (int32)EMessageSeverity::Info + 1;
			OwningNode->ErrorMsg = {};
			OwningNode->bHasCompilerMessage = false;
		}

		const TSharedPtr<SGraphNode> GraphNode = OwnerNodePtr.Pin();
		if (bDidChange && GraphNode.IsValid())
		{
			GraphNode->RefreshErrorInfo();
		}
	}
}

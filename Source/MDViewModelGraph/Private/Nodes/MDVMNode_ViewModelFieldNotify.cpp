#include "Nodes/MDVMNode_ViewModelFieldNotify.h"

#include "Bindings/MDViewModelFieldNotifyBinding.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprint.h"
#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"

bool UMDVMNode_ViewModelFieldNotify::Modify(bool bAlwaysMarkDirty)
{
	CachedNodeTitle.MarkDirty();

	return Super::Modify(bAlwaysMarkDirty);
}

void UMDVMNode_ViewModelFieldNotify::ReconstructNode()
{
	FFieldVariant Field = GetTargetFieldNotify();
	if (!Field.IsValid())
	{
		if (const FProperty* NewProp = FMemberReference::FindRemappedField<FProperty>(ViewModelClass, FieldNotifyName))
		{
			FieldNotifyName = NewProp->GetFName();
		}
		else if (const UFunction* NewFunc = FMemberReference::FindRemappedField<UFunction>(ViewModelClass, FieldNotifyName))
		{
			FieldNotifyName = NewFunc->GetFName();
		}

		Field = GetTargetFieldNotify();
	}

	constexpr bool bIsConsideredSelfContext = false;
	if (const FProperty* Property = Field.Get<FProperty>())
	{
		FieldNotifyReference.SetFromField<FProperty>(Property, bIsConsideredSelfContext);
	}
	else if (const UFunction* Function = Field.Get<UFunction>())
	{
		FieldNotifyReference.SetFromField<UFunction>(Function, bIsConsideredSelfContext);
	}

	CachedNodeTitle.MarkDirty();

	Super::ReconstructNode();
}

FText UMDVMNode_ViewModelFieldNotify::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (CachedNodeTitle.IsOutOfDate(this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("FieldNotifyName"), GetTargetFieldNotifyDisplayName());
		Args.Add(TEXT("ViewModelClass"), ViewModelClass != nullptr ? ViewModelClass->GetDisplayNameText() : INVTEXT("NULL"));
		Args.Add(TEXT("ViewModelName"), FText::FromString(ViewModelName.ToString()));

		CachedNodeTitle.SetCachedText(FText::Format(INVTEXT("{FieldNotifyName} ({ViewModelClass} - {ViewModelName})"), Args), this);
	}

	return CachedNodeTitle;
}

FText UMDVMNode_ViewModelFieldNotify::GetTooltipText() const
{
	const FFieldVariant Field = GetTargetFieldNotify();
	if (const FProperty* Property = Field.Get<FProperty>())
	{
		return Property->GetToolTipText();
	}
	else if (const UFunction* Function = Field.Get<UFunction>())
	{
		return Function->GetToolTipText();
	}
	else
	{
		return FText::FromName(FieldNotifyName);
	}
}

bool UMDVMNode_ViewModelFieldNotify::HasDeprecatedReference() const
{
	const FFieldVariant Field = GetTargetFieldNotify();
	if (const FProperty* Property = Field.Get<FProperty>())
	{
		return Property->HasAnyPropertyFlags(CPF_Deprecated);
	}
	else if (const UFunction* Function = Field.Get<UFunction>())
	{
		return Function->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction);
	}

	return false;
}

FEdGraphNodeDeprecationResponse UMDVMNode_ViewModelFieldNotify::GetDeprecationResponse(EEdGraphNodeDeprecationType DeprecationType) const
{
	FEdGraphNodeDeprecationResponse Response = Super::GetDeprecationResponse(DeprecationType);
	if (DeprecationType == EEdGraphNodeDeprecationType::NodeHasDeprecatedReference)
	{
		const FFieldVariant Field = GetTargetFieldNotify();
		if (Field.IsA<FProperty>())
		{
			Response.MessageType = EEdGraphNodeDeprecationMessageType::Warning;
			Response.MessageText = FBlueprintEditorUtils::GetDeprecatedMemberUsageNodeWarning(GetTargetFieldNotifyDisplayName(), INVTEXT("The bound Field Notify property is deprecated"));
		}
		else if (const UFunction* Function = Field.Get<UFunction>())
		{
			Response.MessageType = EEdGraphNodeDeprecationMessageType::Warning;
			const FText DetailedMessage = FText::FromString(Function->GetMetaData(FBlueprintMetadata::MD_DeprecationMessage));
			Response.MessageText = FBlueprintEditorUtils::GetDeprecatedMemberUsageNodeWarning(GetTargetFieldNotifyDisplayName(), DetailedMessage);
		}
	}

	return Response;
}

void UMDVMNode_ViewModelFieldNotify::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	if (const FProperty* FieldNotifyProp = ResolveFieldNotifyPropertyType())
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		constexpr EEdGraphPinDirection Direction = EGPD_Output;

		UEdGraphPin* Pin = CreatePin(Direction, NAME_None, FieldNotifyProp->GetFName());
		Pin->PinToolTip = GetTooltipText().ToString();
		K2Schema->ConvertPropertyToPinType(FieldNotifyProp, /*out*/ Pin->PinType);
		K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
	}
}

UClass* UMDVMNode_ViewModelFieldNotify::GetDynamicBindingClass() const
{
	return UMDViewModelFieldNotifyBinding::StaticClass();
}

void UMDVMNode_ViewModelFieldNotify::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UMDViewModelFieldNotifyBinding* ComponentBindingObject = CastChecked<UMDViewModelFieldNotifyBinding>(BindingObject);

	FMDViewModelFieldNotifyBindingEntry Binding;
	Binding.ViewModelClass = ViewModelClass;
	Binding.ViewModelName = ViewModelName;
	Binding.FieldNotifyName = FieldNotifyName;
	Binding.FunctionNameToBind = CustomFunctionName;

	CachedNodeTitle.MarkDirty();
	ComponentBindingObject->ViewModelFieldNotifyBindings.Add(Binding);
}

void UMDVMNode_ViewModelFieldNotify::HandleVariableRenamed(UBlueprint* InBlueprint, UClass* InVariableClass, UEdGraph* InGraph, const FName& InOldVarName,
	const FName& InNewVarName)
{
	if (InVariableClass && InVariableClass->IsChildOf(ViewModelClass))
	{
		if (InOldVarName == FieldNotifyName)
		{
			Modify();
			FieldNotifyName = InNewVarName;
		}
	}
}

void UMDVMNode_ViewModelFieldNotify::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	const UBlueprint* BP = GetBlueprint();
	if (BP == nullptr)
	{
		MessageLog.Error(TEXT("@@ cannot find a valid Blueprint."), this);
	}
	else if (!ViewModelClass)
	{
		MessageLog.Error(TEXT("@@ does not have a valid View Model reference! Delete and recreate the event."), this);
	}
	else
	{
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
		FMDViewModelGraphStatics::SearchViewModelAssignmentsForBlueprint(BP, ViewModelAssignments, ViewModelClass, FGameplayTag::EmptyTag, ViewModelName);

		// Only Native assignments are valid during compile, we need to go through the BP otherwise
		if (ViewModelAssignments.IsEmpty())
		{
			if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(BP))
			{
				if (const UMDViewModelWidgetBlueprintExtension* Extension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
				{
					if (!Extension->DoesContainViewModelAssignment(ViewModelClass, FGameplayTag::EmptyTag, ViewModelName))
					{
						MessageLog.Error(*FString::Printf(TEXT("@@ is bound to a view model named [%s] of class [%s] but the view model is not assigned to this widget."), *ViewModelName.ToString(), *ViewModelClass->GetDisplayNameText().ToString()), this);
					}
				}
			}
			else
			{
				// TODO - Actor View Models
			}
		}

		if (!GetTargetFieldNotify().IsValid())
		{
			if (!FMemberReference::FindRemappedField<FProperty>(ViewModelClass, FieldNotifyName)
				&& !FMemberReference::FindRemappedField<UFunction>(ViewModelClass, FieldNotifyName))
			{
				MessageLog.Error(*FString::Printf(TEXT("@@ is attempting to bind to a non-existent field notify [%s] on View Model of class [%s]"), *FieldNotifyName.ToString(), *ViewModelClass->GetDisplayNameText().ToString()), this);
			}
		}
	}

	UK2Node_EditablePinBase::ValidateNodeDuringCompilation(MessageLog);
}

bool UMDVMNode_ViewModelFieldNotify::IsFunctionEntryCompatible(const UK2Node_FunctionEntry* EntryNode) const
{
	// Since K2Node_CustomEvent can't be extended, we have to do this const_cast to inject our pin into the entry node

	UK2Node_FunctionEntry* MutableEntryNode = const_cast<UK2Node_FunctionEntry*>(EntryNode);
	if (const FProperty* FieldNotifyProp = ResolveFieldNotifyPropertyType())
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		constexpr EEdGraphPinDirection Direction = EGPD_Output;

		UEdGraphPin* Pin = MutableEntryNode->CreatePin(Direction, NAME_None, FieldNotifyProp->GetFName());
		Pin->PinToolTip = GetTooltipText().ToString();
		K2Schema->ConvertPropertyToPinType(FieldNotifyProp, /*out*/ Pin->PinType);
		K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
	}

	return Super::IsFunctionEntryCompatible(EntryNode);
}

bool UMDVMNode_ViewModelFieldNotify::IsUsedByAuthorityOnlyDelegate() const
{
	const FFieldVariant Field = GetTargetFieldNotify();
	if (const FProperty* Property = Field.Get<FProperty>())
	{
		return Property->HasAnyPropertyFlags(CPF_BlueprintAuthorityOnly);
	}
	else if (const UFunction* Function = Field.Get<UFunction>())
	{
		return Function->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly);
	}

	return false;
}

FText UMDVMNode_ViewModelFieldNotify::GetTargetFieldNotifyDisplayName() const
{
	const FFieldVariant Field = GetTargetFieldNotify();
	if (const FProperty* Property = Field.Get<FProperty>())
	{
		return Property->GetDisplayNameText();
	}
	else if (const UFunction* Function = Field.Get<UFunction>())
	{
		return Function->GetDisplayNameText();
	}

	return FText::FromName(FieldNotifyName);
}

void UMDVMNode_ViewModelFieldNotify::InitializeViewModelFieldNotifyParams(TSubclassOf<UMDViewModelBase> InViewModelClass, const FName& InViewModelName, const FName& InFieldNotifyName)
{
	if (InViewModelClass != nullptr)
	{
		ViewModelClass = InViewModelClass;
		ViewModelName = InViewModelName;
		FieldNotifyName = InFieldNotifyName;

		bOverrideFunction = false;
		bInternalEvent = true;
		OnAssignmentChanged();
	}
}

void UMDVMNode_ViewModelFieldNotify::OnAssignmentChanged()
{
	Super::OnAssignmentChanged();

	constexpr bool bIsConsideredSelfContext = false;
	const FFieldVariant Field = GetTargetFieldNotify();
	if (const FProperty* Property = Field.Get<FProperty>())
	{
		FieldNotifyReference.SetFromField<FProperty>(Property, bIsConsideredSelfContext);
	}
	else if (const UFunction* Function = Field.Get<UFunction>())
	{
		FieldNotifyReference.SetFromField<UFunction>(Function, bIsConsideredSelfContext);
	}
	
	CustomFunctionName = FName(*FString::Printf(TEXT("BndEvt__%s_%s_%s_%s"), *GetBlueprint()->GetName(), *ViewModelClass->GetName(), *GetName(), *FieldNotifyReference.GetMemberName().ToString()));
}

FFieldVariant UMDVMNode_ViewModelFieldNotify::GetTargetFieldNotify() const
{
	return FindUFieldOrFProperty(ViewModelClass, FieldNotifyName);
}

const FProperty* UMDVMNode_ViewModelFieldNotify::ResolveFieldNotifyPropertyType() const
{
	const FFieldVariant Field = GetTargetFieldNotify();
	if (const FProperty* Property = Field.Get<FProperty>())
	{
		return Property;
	}
	else if (const UFunction* Function = Field.Get<UFunction>())
	{
		return Function->GetReturnProperty();
	}

	return nullptr;
}

#include "Nodes/MDVMNode_ViewModelEvent.h"

#include "Bindings/MDViewModelDelegateBinding.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Util/MDViewModelGraphStatics.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprintExtension.h"
#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"

bool UMDVMNode_ViewModelEvent::Modify(bool bAlwaysMarkDirty)
{
	CachedNodeTitle.MarkDirty();

	return Super::Modify(bAlwaysMarkDirty);
}

void UMDVMNode_ViewModelEvent::ReconstructNode()
{
	// We need to fixup our event reference as it may have changed or been redirected
	const FMulticastDelegateProperty* TargetDelegateProp = GetTargetDelegateProperty();

	// If we couldn't find the target delegate, then try to find it in the property remap table
	if (!TargetDelegateProp)
	{
		if (const FMulticastDelegateProperty* NewProperty = FMemberReference::FindRemappedField<FMulticastDelegateProperty>(Assignment.ViewModelClass.LoadSynchronous(), DelegatePropertyName))
		{
			// Found a remapped property, update the node
			TargetDelegateProp = NewProperty;
			DelegatePropertyName = NewProperty->GetFName();
		}
	}

	if (TargetDelegateProp && TargetDelegateProp->SignatureFunction)
	{
		EventReference.SetFromField<UFunction>(TargetDelegateProp->SignatureFunction, false);
	}

	CachedNodeTitle.MarkDirty();

	Super::ReconstructNode();
}

FText UMDVMNode_ViewModelEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (CachedNodeTitle.IsOutOfDate(this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("DelegatePropertyName"), GetTargetDelegateDisplayName());
		Args.Add(TEXT("ViewModelClass"), GetViewModelClassName());
		Args.Add(TEXT("ViewModelName"), FText::FromString(Assignment.ViewModelName.ToString()));

		CachedNodeTitle.SetCachedText(FText::Format(INVTEXT("{DelegatePropertyName} ({ViewModelClass} - {ViewModelName})"), Args), this);
	}

	return CachedNodeTitle;
}

FText UMDVMNode_ViewModelEvent::GetTooltipText() const
{
	if (const FMulticastDelegateProperty* TargetDelegateProp = GetTargetDelegateProperty())
	{
		return TargetDelegateProp->GetToolTipText();
	}
	else
	{
		return FText::FromName(DelegatePropertyName);
	}
}

bool UMDVMNode_ViewModelEvent::HasDeprecatedReference() const
{
	if (const FMulticastDelegateProperty* DelegateProperty = GetTargetDelegateProperty())
	{
		return DelegateProperty->HasAnyPropertyFlags(CPF_Deprecated);
	}

	return false;
}

FEdGraphNodeDeprecationResponse UMDVMNode_ViewModelEvent::GetDeprecationResponse(EEdGraphNodeDeprecationType DeprecationType) const
{
	FEdGraphNodeDeprecationResponse Response = Super::GetDeprecationResponse(DeprecationType);
	if (DeprecationType == EEdGraphNodeDeprecationType::NodeHasDeprecatedReference)
	{
		const UFunction* Function = EventReference.ResolveMember<UFunction>(GetBlueprintClassFromNode());
		if (ensureMsgf(Function != nullptr, TEXT("This node should not be able to report having a deprecated reference if the event override cannot be resolved.")))
		{
			Response.MessageType = EEdGraphNodeDeprecationMessageType::Warning;
			const FText DetailedMessage = FText::FromString(Function->GetMetaData(FBlueprintMetadata::MD_DeprecationMessage));
			Response.MessageText = FBlueprintEditorUtils::GetDeprecatedMemberUsageNodeWarning(GetTargetDelegateDisplayName(), DetailedMessage);
		}
	}

	return Response;
}

UClass* UMDVMNode_ViewModelEvent::GetDynamicBindingClass() const
{
	return UMDViewModelDelegateBinding::StaticClass();
}

void UMDVMNode_ViewModelEvent::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UMDViewModelDelegateBinding* ComponentBindingObject = CastChecked<UMDViewModelDelegateBinding>(BindingObject);

	FMDViewModelDelegateBindingEntry Binding;
	Binding.ViewModelClass = Assignment.ViewModelClass.LoadSynchronous();
	Binding.ViewModelName = Assignment.ViewModelName;
	Binding.DelegatePropertyName = DelegatePropertyName;
	Binding.FunctionNameToBind = CustomFunctionName;

	CachedNodeTitle.MarkDirty();
	ComponentBindingObject->ViewModelDelegateBindings.Add(Binding);
}

void UMDVMNode_ViewModelEvent::HandleVariableRenamed(UBlueprint* InBlueprint, UClass* InVariableClass, UEdGraph* InGraph, const FName& InOldVarName, const FName& InNewVarName)
{
	if (InVariableClass && InVariableClass->IsChildOf(Assignment.ViewModelClass.Get()))
	{
		if (InOldVarName == DelegatePropertyName)
		{
			Modify();
			DelegatePropertyName = InNewVarName;
		}
	}
}

void UMDVMNode_ViewModelEvent::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	const UBlueprint* BP = GetBlueprint();
	if (BP == nullptr)
	{
		MessageLog.Error(TEXT("@@ cannot find a valid Blueprint."), this);
	}
	else if (!Assignment.IsAssignmentValid())
	{
		MessageLog.Error(TEXT("@@ does not have a valid View Model reference! Delete and recreate the event."), this);
	}
	else
	{
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
		FMDViewModelGraphStatics::SearchViewModelAssignmentsForBlueprint(BP, ViewModelAssignments, Assignment.ViewModelClass.LoadSynchronous(), FGameplayTag::EmptyTag, Assignment.ViewModelName);

		if (ViewModelAssignments.IsEmpty())
		{
			MessageLog.Error(*FString::Printf(TEXT("@@ is bound to a view model named [%s] of class [%s] but the view model is not assigned to this widget."), *Assignment.ViewModelName.ToString(), *GetViewModelClassName().ToString()), this);
		}

		if (!GetTargetDelegateProperty() && !FMemberReference::FindRemappedField<FMulticastDelegateProperty>(Assignment.ViewModelClass.LoadSynchronous(), DelegatePropertyName))
		{
			MessageLog.Error(*FString::Printf(TEXT("@@ is attempting to bind to a non-existent delegate [%s] on View Model of class [%s]"), *DelegatePropertyName.ToString(), *GetViewModelClassName().ToString()), this);
		}
	}

	Super::ValidateNodeDuringCompilation(MessageLog);
}

bool UMDVMNode_ViewModelEvent::IsUsedByAuthorityOnlyDelegate() const
{
	const FMulticastDelegateProperty* TargetDelegateProp = GetTargetDelegateProperty();
	return (TargetDelegateProp && TargetDelegateProp->HasAnyPropertyFlags(CPF_BlueprintAuthorityOnly));
}

FMulticastDelegateProperty* UMDVMNode_ViewModelEvent::GetTargetDelegateProperty() const
{
	return FindFProperty<FMulticastDelegateProperty>(Assignment.ViewModelClass.LoadSynchronous(), DelegatePropertyName);
}

FText UMDVMNode_ViewModelEvent::GetTargetDelegateDisplayName() const
{
	const FMulticastDelegateProperty* Prop = GetTargetDelegateProperty();
	return Prop ? Prop->GetDisplayNameText() : FText::FromName(DelegatePropertyName);
}

void UMDVMNode_ViewModelEvent::InitializeViewModelEventParams(const FMDViewModelAssignmentReference& InAssignment, const FName& InDelegatePropertyName)
{
	if (InAssignment.IsAssignmentValid())
	{
		Assignment = InAssignment;
		DelegatePropertyName = InDelegatePropertyName;

		bOverrideFunction = false;
		bInternalEvent = true;
		OnAssignmentChanged();
	}
}

void UMDVMNode_ViewModelEvent::OnAssignmentChanged()
{
	Super::OnAssignmentChanged();

	constexpr bool bIsConsideredSelfContext = false;
	const FMulticastDelegateProperty* Prop = GetTargetDelegateProperty();
	EventReference.SetFromField<UFunction>(Prop->SignatureFunction, bIsConsideredSelfContext);

	CustomFunctionName = FName(*FString::Printf(TEXT("BndEvt__%s_%s_%s_%s"), *GetBlueprint()->GetName(), *GetNameSafe(Assignment.ViewModelClass.LoadSynchronous()), *GetName(), *EventReference.GetMemberName().ToString()));
}

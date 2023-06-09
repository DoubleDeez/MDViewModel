#include "Nodes/MDVMNode_ViewModelChanged.h"

#include "Bindings/MDViewModelChangedBinding.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_FunctionEntry.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MDViewModelModule.h"
#include "ViewModel/MDViewModelBase.h"
#include "WidgetBlueprint.h"
#include "WidgetExtensions/MDViewModelWidgetBlueprintExtension.h"

namespace MDVMNode_ViewModelChanged_Private
{
	const FName OldViewModelPin = TEXT("OldViewModel");
	const FName NewViewModelPin = TEXT("NewViewModel");

	void AllocatePins(UK2Node* Node, TSubclassOf<UMDViewModelBase> ViewModelClass)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		constexpr EEdGraphPinDirection Direction = EGPD_Output;

		{
			UEdGraphPin* Pin = Node->CreatePin(Direction, NAME_None, OldViewModelPin);
			Pin->PinToolTip = TEXT("The view model that was previous set.");
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
			Pin->PinType.PinSubCategoryObject = ViewModelClass;
			K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
		}

		{
			UEdGraphPin* Pin = Node->CreatePin(Direction, NAME_None, NewViewModelPin);
			Pin->PinToolTip = TEXT("The new view model that was set.");
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
			Pin->PinType.PinSubCategoryObject = ViewModelClass;
			K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
		}
	}
}

bool UMDVMNode_ViewModelChanged::Modify(bool bAlwaysMarkDirty)
{
	CachedNodeTitle.MarkDirty();

	return Super::Modify(bAlwaysMarkDirty);
}

void UMDVMNode_ViewModelChanged::ReconstructNode()
{
	CachedNodeTitle.MarkDirty();

	Super::ReconstructNode();
}

FText UMDVMNode_ViewModelChanged::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (CachedNodeTitle.IsOutOfDate(this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ViewModelClass"), ViewModelClass != nullptr ? ViewModelClass->GetDisplayNameText() : INVTEXT("NULL"));
		Args.Add(TEXT("ViewModelName"), FText::FromString(ViewModelName.ToString()));

		CachedNodeTitle.SetCachedText(FText::Format(INVTEXT("On View Model Changed ({ViewModelClass} - {ViewModelName})"), Args), this);
	}

	return CachedNodeTitle;
}

FText UMDVMNode_ViewModelChanged::GetTooltipText() const
{
	return GetNodeTitle(ENodeTitleType::FullTitle);
}

void UMDVMNode_ViewModelChanged::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	MDVMNode_ViewModelChanged_Private::AllocatePins(this, ViewModelClass);
}

UClass* UMDVMNode_ViewModelChanged::GetDynamicBindingClass() const
{
	return UMDViewModelChangedBinding::StaticClass();
}

void UMDVMNode_ViewModelChanged::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UMDViewModelChangedBinding* ComponentBindingObject = CastChecked<UMDViewModelChangedBinding>(BindingObject);

	FMDViewModelChangedBindingEntry Binding;
	Binding.ViewModelClass = ViewModelClass;
	Binding.ViewModelName = ViewModelName;
	Binding.FunctionNameToBind = CustomFunctionName;

	CachedNodeTitle.MarkDirty();
	ComponentBindingObject->ViewModelChangedBindings.Add(Binding);
}

void UMDVMNode_ViewModelChanged::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	const UWidgetBlueprint* const BP = Cast<UWidgetBlueprint>(GetBlueprint());
	if (BP == nullptr)
	{
		MessageLog.Error(TEXT("@@ can only be used in Widget Blueprints."), this);
	}
	else if (!ViewModelClass)
	{
		MessageLog.Error(TEXT("@@ does not have a valid View Model reference! Delete and recreate the event."), this);
	}
	else
	{
		const FMDViewModelModule& ViewModelModule = FModuleManager::LoadModuleChecked<FMDViewModelModule>(TEXT("MDViewModel"));
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
		ViewModelModule.SearchViewModelAssignments(ViewModelAssignments, BP->GeneratedClass.Get(), ViewModelClass, FGameplayTag::EmptyTag, ViewModelName);

		// Only Native assignments are valid during compile, we need to go through the BP otherwise
		if (ViewModelAssignments.IsEmpty())
		{
			if (const UMDViewModelWidgetBlueprintExtension* Extension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(BP))
			{
				if (!Extension->DoesContainViewModelAssignment(ViewModelClass, FGameplayTag::EmptyTag, ViewModelName))
				{
					MessageLog.Error(*FString::Printf(TEXT("@@ is bound to a view model named [%s] of class [%s] but the view model is not assigned to this widget."), *ViewModelName.ToString(), *ViewModelClass->GetDisplayNameText().ToString()), this);
				}
			}
		}
	}

	UK2Node_EditablePinBase::ValidateNodeDuringCompilation(MessageLog);
}

bool UMDVMNode_ViewModelChanged::IsFunctionEntryCompatible(const UK2Node_FunctionEntry* EntryNode) const
{
	// Since K2Node_CustomEvent can't be extended, we have to do this const_cast to inject our pins into the entry node
	UK2Node_FunctionEntry* MutableEntryNode = const_cast<UK2Node_FunctionEntry*>(EntryNode);
	MDVMNode_ViewModelChanged_Private::AllocatePins(MutableEntryNode, ViewModelClass);

	return Super::IsFunctionEntryCompatible(EntryNode);
}

void UMDVMNode_ViewModelChanged::InitializeViewModelChangedParams(TSubclassOf<UMDViewModelBase> InViewModelClass, const FName& InViewModelName)
{
	if (InViewModelClass != nullptr)
	{
		ViewModelClass = InViewModelClass;
		ViewModelName = InViewModelName;

		CustomFunctionName = FName(*FString::Printf(TEXT("BndEvt__%s_%s_%s_Changed"), *GetBlueprint()->GetName(), *ViewModelClass->GetName(), *GetName()));
		bOverrideFunction = false;
		bInternalEvent = true;
		CachedNodeTitle.MarkDirty();
	}
}

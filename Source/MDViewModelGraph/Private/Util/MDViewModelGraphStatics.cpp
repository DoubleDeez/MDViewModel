#include "Util/MDViewModelGraphStatics.h"

#include "BlueprintExtensions/MDViewModelActorBlueprintExtension.h"
#include "BlueprintExtensions/MDViewModelAssignableInterface.h"
#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"
#include "Components/MDViewModelAssignmentComponent.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Nodes/MDVMNode_ViewModelChanged.h"
#include "Nodes/MDVMNode_ViewModelEvent.h"
#include "Nodes/MDVMNode_ViewModelFieldNotify.h"
#include "ViewModel/MDViewModelBase.h"

const FName FMDViewModelGraphStatics::VMHiddenMeta = TEXT("MDVMHidden");

void FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(const UBlueprint* Blueprint, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments)
{
	if (Blueprint != nullptr)
	{
		const TObjectPtr<UBlueprintExtension>* ExtensionPtr = Blueprint->GetExtensions().FindByPredicate([](const TObjectPtr<UBlueprintExtension> BPExtension)
		{
			return BPExtension != nullptr && BPExtension->Implements<UMDViewModelAssignableInterface>();
		});

		UClass* ParentClass = Blueprint->ParentClass;
		const UBlueprint* ParentBP = IsValid(ParentClass) ? Cast<UBlueprint>(ParentClass->ClassGeneratedBy) : nullptr;

		if (ExtensionPtr != nullptr)
		{
			if (const IMDViewModelAssignableInterface* Extension = Cast<IMDViewModelAssignableInterface>(ExtensionPtr->Get()))
			{
				Extension->GetAllAssignments(OutViewModelAssignments);
			}
		}
		else if (Blueprint->bBeingCompiled && IsValid(ParentBP) && ParentBP->bBeingCompiled)
		{
			// If we're compiling this BP and the parent BP, we'll need to search the Parent BP instead of its Class
			GetViewModelAssignmentsForBlueprint(ParentBP, OutViewModelAssignments);
		}
		else
		{
			// If this BP doesn't have assignments, get the parent assignments
			MDViewModelUtils::GetViewModelAssignments(ParentClass, OutViewModelAssignments);
		}
	}
}

void FMDViewModelGraphStatics::SearchViewModelAssignmentsForBlueprint(const UBlueprint* Blueprint, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName)
{
	GetViewModelAssignmentsForBlueprint(Blueprint, OutViewModelAssignments);

	// Remove assignments that don't match the filter
	for (auto It = OutViewModelAssignments.CreateIterator(); It; ++It)
	{
		if (ProviderTag.IsValid() && !ProviderTag.MatchesTagExact(It.Key().ProviderTag))
		{
			It.RemoveCurrent();
			continue;
		}

		if (ViewModelName != NAME_None && ViewModelName != It.Key().ViewModelName)
		{
			It.RemoveCurrent();
			continue;
		}

		if (ViewModelClass != nullptr && ViewModelClass != It.Key().ViewModelClass)
		{
			It.RemoveCurrent();
			continue;
		}
	}
}

bool FMDViewModelGraphStatics::DoesBlueprintContainViewModelAssignments(const UBlueprint* Blueprint, TSubclassOf<UMDViewModelBase> ViewModelClass, const FGameplayTag& ProviderTag, const FName& ViewModelName)
{
	TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> ViewModelAssignments;
	SearchViewModelAssignmentsForBlueprint(Blueprint, ViewModelAssignments, ViewModelClass, ProviderTag, ViewModelName);
	return !ViewModelAssignments.IsEmpty();
}

bool FMDViewModelGraphStatics::DoesBlueprintBindToViewModelEvent(const UBlueprint* BP, const FName& EventName, const FMDViewModelAssignmentReference& Assignment)
{
	return FindExistingViewModelEventNode(BP, EventName, Assignment) != nullptr;
}

void FMDViewModelGraphStatics::OnViewModelEventRequestedForBlueprint(const UBlueprint* BP, const FName& EventName, const FMDViewModelAssignmentReference& Assignment)
{
	if (BP == nullptr)
	{
		return;
	}

	const UMDVMNode_ViewModelEvent* Node = FindExistingViewModelEventNode(BP, EventName, Assignment);
	if (Node == nullptr)
	{
		if (UEdGraph* TargetGraph = BP->GetLastEditedUberGraph())
		{
			const FVector2D NewNodePos = TargetGraph->GetGoodPlaceForNewNode();

			Node = FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_ViewModelEvent>(
				TargetGraph,
				NewNodePos,
				EK2NewNodeFlags::SelectNewNode,
				[&](UMDVMNode_ViewModelEvent* NewInstance)
				{
					NewInstance->InitializeViewModelEventParams(Assignment, EventName);
				}
			);
		}
	}

	FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Node);
}

UMDVMNode_ViewModelEvent* FMDViewModelGraphStatics::FindExistingViewModelEventNode(const UBlueprint* BP, const FName& EventName, const FMDViewModelAssignmentReference& Assignment)
{
	if (BP == nullptr)
	{
		return nullptr;
	}

	TArray<UMDVMNode_ViewModelEvent*> EventNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(BP, EventNodes);
	for (auto NodeIter = EventNodes.CreateIterator(); NodeIter; ++NodeIter)
	{
		UMDVMNode_ViewModelEvent* BoundEvent = *NodeIter;
		if (BoundEvent->Assignment == Assignment && BoundEvent->DelegatePropertyName == EventName)
		{
			return BoundEvent;
		}
	}

	return nullptr;
}

bool FMDViewModelGraphStatics::DoesBlueprintBindToViewModelFieldNotify(const UBlueprint* BP, const FName& FieldNotifyName, const FMDViewModelAssignmentReference& Assignment)
{
	return FindExistingViewModelFieldNotifyNode(BP, FieldNotifyName, Assignment) != nullptr;
}

void FMDViewModelGraphStatics::OnViewModelFieldNotifyRequestedForBlueprint(const UBlueprint* BP, const FName& FieldNotifyName, const FMDViewModelAssignmentReference& Assignment)
{
	if (BP == nullptr)
	{
		return;
	}

	const UMDVMNode_ViewModelFieldNotify* Node = FindExistingViewModelFieldNotifyNode(BP, FieldNotifyName, Assignment);
	if (Node == nullptr)
	{
		if (UEdGraph* TargetGraph = BP->GetLastEditedUberGraph())
		{
			const FVector2D NewNodePos = TargetGraph->GetGoodPlaceForNewNode();

			Node = FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_ViewModelFieldNotify>(
				TargetGraph,
				NewNodePos,
				EK2NewNodeFlags::SelectNewNode,
				[&](UMDVMNode_ViewModelFieldNotify* NewInstance)
				{
					NewInstance->InitializeViewModelFieldNotifyParams(Assignment, FieldNotifyName);
				}
			);
		}
	}

	FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Node);
}

UMDVMNode_ViewModelFieldNotify* FMDViewModelGraphStatics::FindExistingViewModelFieldNotifyNode(const UBlueprint* BP, const FName& FieldNotifyName, const FMDViewModelAssignmentReference& Assignment)
{
	if (BP == nullptr)
	{
		return nullptr;
	}

	TArray<UMDVMNode_ViewModelFieldNotify*> EventNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(BP, EventNodes);
	for (auto NodeIter = EventNodes.CreateIterator(); NodeIter; ++NodeIter)
	{
		UMDVMNode_ViewModelFieldNotify* BoundEvent = *NodeIter;
		if (BoundEvent->Assignment == Assignment && BoundEvent->FieldNotifyName == FieldNotifyName)
		{
			return BoundEvent;
		}
	}

	return nullptr;
}

bool FMDViewModelGraphStatics::DoesBlueprintBindToViewModelChanged(const UBlueprint* BP, const FMDViewModelAssignmentReference& Assignment)
{
	return FindExistingViewModelChangedNode(BP, Assignment) != nullptr;
}

void FMDViewModelGraphStatics::OnViewModelChangedRequestedForBlueprint(const UBlueprint* BP, const FMDViewModelAssignmentReference& Assignment)
{
	if (BP == nullptr)
	{
		return;
	}

	const UMDVMNode_ViewModelChanged* Node = FindExistingViewModelChangedNode(BP, Assignment);
	if (Node == nullptr)
	{
		if (UEdGraph* TargetGraph = BP->GetLastEditedUberGraph())
		{
			const FVector2D NewNodePos = TargetGraph->GetGoodPlaceForNewNode();

			Node = FEdGraphSchemaAction_K2NewNode::SpawnNode<UMDVMNode_ViewModelChanged>(
				TargetGraph,
				NewNodePos,
				EK2NewNodeFlags::SelectNewNode,
				[&](UMDVMNode_ViewModelChanged* NewInstance)
				{
					NewInstance->InitializeViewModelChangedParams(Assignment);
				}
			);
		}
	}

	FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Node);
}

UMDVMNode_ViewModelChanged* FMDViewModelGraphStatics::FindExistingViewModelChangedNode(const UBlueprint* BP, const FMDViewModelAssignmentReference& Assignment)
{
	if (BP == nullptr)
	{
		return nullptr;
	}

	TArray<UMDVMNode_ViewModelChanged*> EventNodes;
	FBlueprintEditorUtils::GetAllNodesOfClass(BP, EventNodes);
	for (auto NodeIter = EventNodes.CreateIterator(); NodeIter; ++NodeIter)
	{
		UMDVMNode_ViewModelChanged* BoundEvent = *NodeIter;
		if (BoundEvent->Assignment == Assignment)
		{
			return BoundEvent;
		}
	}

	return nullptr;
}

bool FMDViewModelGraphStatics::DoesBlueprintUseAssignment(UBlueprint* BP, const FMDViewModelAssignmentReference& Assignment)
{
	if (!Assignment.IsAssignmentValid())
	{
		return false;
	}

	TArray<UBlueprint*> DependentBlueprints;
	FBlueprintEditorUtils::FindDependentBlueprints(BP, DependentBlueprints);
	DependentBlueprints.Add(BP);

	for (const UBlueprint* Blueprint : DependentBlueprints)
	{
		if (!IsValid(Blueprint))
		{
			continue;
		}

		TArray<UEdGraphNode*> GraphNodes;
		FBlueprintEditorUtils::GetAllNodesOfClass(BP, GraphNodes);

		for (const UEdGraphNode* Node : GraphNodes)
		{
			if (!IsValid(Node))
			{
				continue;
			}

			// Check for FMDViewModelAssignmentReference or FMDViewModelAssignment Properties whose value reference the assignment
			for (TFieldIterator<const FStructProperty> It(Node->GetClass()); It; ++It)
			{
				const FStructProperty* Prop = *It;
				if (Prop == nullptr)
				{
					continue;
				}

				if (Prop->Struct == FMDViewModelAssignmentReference::StaticStruct())
				{
					FMDViewModelAssignmentReference PropAssignment;
					Prop->GetValue_InContainer(Node, &PropAssignment);
					if (PropAssignment == Assignment)
					{
						return true;
					}
				}
				else if (Prop->Struct == FMDViewModelAssignment::StaticStruct())
				{
					FMDViewModelAssignment PropAssignment;
					Prop->GetValue_InContainer(Node, &PropAssignment);
					if (FMDViewModelAssignmentReference(PropAssignment) == Assignment)
					{
						return true;
					}
				}
			}

			// Check for FMDViewModelAssignmentReference or FMDViewModelAssignment Pins whose default value references the assignment
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin == nullptr || Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Struct || Pin->Direction != EGPD_Input || !Pin->LinkedTo.IsEmpty())
				{
					continue;
				}

				if (Pin->PinType.PinSubCategoryObject == FMDViewModelAssignmentReference::StaticStruct())
				{
					FMDViewModelAssignmentReference PinAssignment;
					const FString DefaultString = Pin->GetDefaultAsString();
					if (!DefaultString.IsEmpty())
					{
						UScriptStruct* PinLiteralStructType = FMDViewModelAssignmentReference::StaticStruct();
						PinLiteralStructType->ImportText(*DefaultString, &PinAssignment, nullptr, PPF_SerializedAsImportText, GError, PinLiteralStructType->GetName());
					}

					if (PinAssignment == Assignment)
					{
						return true;
					}
				}
				else if (Pin->PinType.PinSubCategoryObject == FMDViewModelAssignment::StaticStruct())
				{
					FMDViewModelAssignment PinAssignment;
					const FString DefaultString = Pin->GetDefaultAsString();
					if (!DefaultString.IsEmpty())
					{
						UScriptStruct* PinLiteralStructType = FMDViewModelAssignment::StaticStruct();
						PinLiteralStructType->ImportText(*DefaultString, &PinAssignment, nullptr, PPF_SerializedAsImportText, GError, PinLiteralStructType->GetName());
					}

					if (FMDViewModelAssignmentReference(PinAssignment) == Assignment)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

IMDViewModelAssignableInterface* FMDViewModelGraphStatics::GetOrCreateAssignableInterface(UBlueprint* BP)
{
	if (UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(BP))
	{
		if (UMDViewModelWidgetBlueprintExtension* Extension = UWidgetBlueprintExtension::RequestExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			Extension->SetFlags(RF_Transactional);
			return Extension;
		}
	}
	else if (IsValid(BP) && IsValid(BP->GeneratedClass) && BP->GeneratedClass->IsChildOf<AActor>())
	{
		const TObjectPtr<UBlueprintExtension>* Extension = BP->GetExtensions().FindByPredicate([](const TObjectPtr<UBlueprintExtension>& Extension)
		{
			return IsValid(Extension) && Extension->IsA<UMDViewModelActorBlueprintExtension>();
		});

		if (Extension != nullptr && IsValid(*Extension))
		{
			return Cast<IMDViewModelAssignableInterface>(*Extension);
		}

		UMDViewModelActorBlueprintExtension* NewExtension = NewObject<UMDViewModelActorBlueprintExtension>(BP, NAME_None, RF_Transactional);
		BP->AddExtension(NewExtension);
		return NewExtension;
	}

	return nullptr;
}

IMDViewModelAssignableInterface* FMDViewModelGraphStatics::GetAssignableInterface(const UBlueprint* BP)
{
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(BP))
	{
		if (UMDViewModelWidgetBlueprintExtension* Extension = UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP))
		{
			Extension->SetFlags(RF_Transactional);
			return Extension;
		}
	}
	else if (IsValid(BP) && IsValid(BP->GeneratedClass) && BP->GeneratedClass->IsChildOf<AActor>())
	{
		const TObjectPtr<UBlueprintExtension>* ExtensionPtr = BP->GetExtensions().FindByPredicate([](const TObjectPtr<UBlueprintExtension>& Extension)
		{
			return IsValid(Extension) && Extension->IsA<UMDViewModelActorBlueprintExtension>();
		});

		IMDViewModelAssignableInterface* Extension = (ExtensionPtr != nullptr) ? Cast<IMDViewModelAssignableInterface>(*ExtensionPtr) : nullptr;

		if (UObject* ExtensionObject = Cast<UObject>(Extension))
		{
			ExtensionObject->SetFlags(RF_Transactional);
		}

		return Extension;
	}

	return nullptr;
}

UMDViewModelAssignmentComponent* FMDViewModelGraphStatics::GetOrCreateAssignmentComponentTemplate(UBlueprintGeneratedClass* BPClass)
{
	check(IsValid(BPClass) && BPClass->IsChildOf<AActor>());

	TArray<UBlueprintGeneratedClass*> Hierarchy;
	UBlueprint::GetBlueprintHierarchyFromClass(BPClass, Hierarchy);

	// Find the most authoritative assignment component (our nearest parent)
	USCS_Node* ComponentNode = nullptr;
	for (UBlueprintGeneratedClass* Class : Hierarchy)
	{
		if (!IsValid(Class) || !IsValid(Class->SimpleConstructionScript))
		{
			continue;
		}

		USCS_Node* const* CurrentComponentNodePtr = Class->SimpleConstructionScript->GetAllNodes().FindByPredicate([](const USCS_Node* Node)
		{
			return IsValid(Node) && IsValid(Node->ComponentTemplate) && Node->ComponentTemplate->IsA<UMDViewModelAssignmentComponent>();
		});
		USCS_Node* CurrentComponentNode = (CurrentComponentNodePtr != nullptr) ? *CurrentComponentNodePtr : nullptr;

		if (Class != BPClass && CurrentComponentNode != nullptr)
		{
			constexpr bool bCreateIfNecessary = true;
			UInheritableComponentHandler* ICH = BPClass->GetInheritableComponentHandler(bCreateIfNecessary);

			const FComponentKey Key(CurrentComponentNode);

			const bool bCanCreateOverriddenComponent = IsValid(BPClass->ClassDefaultObject);
			UActorComponent* Component = bCanCreateOverriddenComponent
				? ICH->CreateOverridenComponentTemplate(Key)
				: ICH->GetOverridenComponentTemplate(Key);

			// Clean up any pre-existing components, which can happen if we had assignments before our parents did
			if (IsValid(BPClass) && IsValid(BPClass->SimpleConstructionScript))
			{
				const TArray<USCS_Node*> AllNodes = BPClass->SimpleConstructionScript->GetAllNodes();
				for (USCS_Node* Node : AllNodes)
				{
					if (Node->ComponentTemplate->IsA<UMDViewModelAssignmentComponent>())
					{
						BPClass->SimpleConstructionScript->RemoveNode(Node);
					}
				}
			}

			return Cast<UMDViewModelAssignmentComponent>(Component);
		}
		else if (Class == BPClass)
		{
			ComponentNode = CurrentComponentNode;
		}
	}

	// We don't have any parent assignments, use our existing component or create one if needed
	if (!IsValid(ComponentNode) && IsValid(BPClass->SimpleConstructionScript))
	{
		ComponentNode = BPClass->SimpleConstructionScript->CreateNode(UMDViewModelAssignmentComponent::StaticClass(), TEXT("MDViewModelAssignments"));
		ComponentNode->ComponentTemplate->CreationMethod = EComponentCreationMethod::Native;
		BPClass->SimpleConstructionScript->AddNode(ComponentNode);
	}

	return Cast<UMDViewModelAssignmentComponent>(ComponentNode->ComponentTemplate);
}

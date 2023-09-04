#include "Util/MDViewModelGraphStatics.h"

#include "BlueprintExtensions/MDViewModelActorBlueprintExtension.h"
#include "BlueprintExtensions/MDViewModelAssignableInterface.h"
#include "BlueprintExtensions/MDViewModelWidgetBlueprintExtension.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Nodes/MDVMNode_ViewModelChanged.h"
#include "Nodes/MDVMNode_ViewModelEvent.h"
#include "Nodes/MDVMNode_ViewModelFieldNotify.h"
#include "ViewModel/MDViewModelBase.h"

void FMDViewModelGraphStatics::GetViewModelAssignmentsForBlueprint(const UBlueprint* Blueprint, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments)
{
	if (Blueprint != nullptr)
	{
		const TObjectPtr<UBlueprintExtension>* ExtensionPtr = Blueprint->GetExtensions().FindByPredicate([](const TObjectPtr<UBlueprintExtension> BPExtension)
		{
			return BPExtension != nullptr && BPExtension->Implements<UMDViewModelAssignableInterface>();
		});

		if (ExtensionPtr != nullptr)
		{
			if (const IMDViewModelAssignableInterface* Extension = Cast<IMDViewModelAssignableInterface>(ExtensionPtr->Get()))
			{
				Extension->GetAllAssignments(OutViewModelAssignments);
			}
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

bool FMDViewModelGraphStatics::DoesBlueprintBindToViewModelEvent(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
{
	return FindExistingViewModelEventNode(BP, EventName, ViewModelClass, ViewModelName) != nullptr;
}

void FMDViewModelGraphStatics::OnViewModelEventRequestedForBlueprint(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
{
	if (BP == nullptr)
	{
		return;
	}

	const UMDVMNode_ViewModelEvent* Node = FindExistingViewModelEventNode(BP, EventName, ViewModelClass, ViewModelName);
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
					NewInstance->InitializeViewModelEventParams(ViewModelClass, ViewModelName, EventName);
				}
			);
		}
	}

	FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Node);
}

UMDVMNode_ViewModelEvent* FMDViewModelGraphStatics::FindExistingViewModelEventNode(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
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
		if (BoundEvent->ViewModelClass == ViewModelClass && BoundEvent->ViewModelName == ViewModelName && BoundEvent->DelegatePropertyName == EventName)
		{
			return BoundEvent;
		}
	}

	return nullptr;
}

bool FMDViewModelGraphStatics::DoesBlueprintBindToViewModelFieldNotify(const UBlueprint* BP, const FName& FieldNotifyName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
{
	return FindExistingViewModelFieldNotifyNode(BP, FieldNotifyName, ViewModelClass, ViewModelName) != nullptr;
}

void FMDViewModelGraphStatics::OnViewModelFieldNotifyRequestedForBlueprint(const UBlueprint* BP, const FName& FieldNotifyName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
{
	if (BP == nullptr)
	{
		return;
	}

	const UMDVMNode_ViewModelFieldNotify* Node = FindExistingViewModelFieldNotifyNode(BP, FieldNotifyName, ViewModelClass, ViewModelName);
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
					NewInstance->InitializeViewModelFieldNotifyParams(ViewModelClass, ViewModelName, FieldNotifyName);
				}
			);
		}
	}

	FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Node);
}

UMDVMNode_ViewModelFieldNotify* FMDViewModelGraphStatics::FindExistingViewModelFieldNotifyNode(const UBlueprint* BP, const FName& FieldNotifyName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
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
		if (BoundEvent->ViewModelClass == ViewModelClass && BoundEvent->ViewModelName == ViewModelName && BoundEvent->FieldNotifyName == FieldNotifyName)
		{
			return BoundEvent;
		}
	}

	return nullptr;
}

bool FMDViewModelGraphStatics::DoesBlueprintBindToViewModelChanged(const UBlueprint* BP, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
{
	return FindExistingViewModelChangedNode(BP, ViewModelClass, ViewModelName) != nullptr;
}

void FMDViewModelGraphStatics::OnViewModelChangedRequestedForBlueprint(const UBlueprint* BP, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
{
	if (BP == nullptr)
	{
		return;
	}

	const UMDVMNode_ViewModelChanged* Node = FindExistingViewModelChangedNode(BP, ViewModelClass, ViewModelName);
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
					NewInstance->InitializeViewModelChangedParams(ViewModelClass, ViewModelName);
				}
			);
		}
	}

	FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Node);
}

UMDVMNode_ViewModelChanged* FMDViewModelGraphStatics::FindExistingViewModelChangedNode(const UBlueprint* BP, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
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
		if (BoundEvent->ViewModelClass == ViewModelClass && BoundEvent->ViewModelName == ViewModelName)
		{
			return BoundEvent;
		}
	}

	return nullptr;
}

IMDViewModelAssignableInterface* FMDViewModelGraphStatics::GetOrCreateAssignableInterface(UBlueprint* BP)
{
	if (UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(BP))
	{
		return UWidgetBlueprintExtension::RequestExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP);
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

		UMDViewModelActorBlueprintExtension* NewExtension = NewObject<UMDViewModelActorBlueprintExtension>(BP);
		BP->AddExtension(NewExtension);
		return NewExtension;
	}

	return nullptr;
}

IMDViewModelAssignableInterface* FMDViewModelGraphStatics::GetAssignableInterface(const UBlueprint* BP)
{
	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(BP))
	{
		return UWidgetBlueprintExtension::GetExtension<UMDViewModelWidgetBlueprintExtension>(WidgetBP);
	}
	else if (IsValid(BP) && IsValid(BP->GeneratedClass) && BP->GeneratedClass->IsChildOf<AActor>())
	{
		const TObjectPtr<UBlueprintExtension>* Extension = BP->GetExtensions().FindByPredicate([](const TObjectPtr<UBlueprintExtension>& Extension)
		{
			return IsValid(Extension) && Extension->IsA<UMDViewModelActorBlueprintExtension>();
		});

		return (Extension != nullptr) ? Cast<IMDViewModelAssignableInterface>(*Extension) : nullptr;
	}

	return nullptr;
}

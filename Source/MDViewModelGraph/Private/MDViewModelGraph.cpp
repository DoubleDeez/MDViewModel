#include "MDViewModelGraph.h"

#include "EdGraphSchema_K2_Actions.h"
#include "MDViewModelEditorModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Nodes/MDVMNode_ViewModelEvent.h"
#include "ViewModel/MDViewModelBase.h"

#define LOCTEXT_NAMESPACE "FMDViewModelGraphModule"

void FMDViewModelGraphModule::StartupModule()
{
#if WITH_EDITOR
	FMDViewModelEditorModule& ViewModelEditorModule = FModuleManager::LoadModuleChecked<FMDViewModelEditorModule>(TEXT("MDViewModelEditor"));
	ViewModelEditorModule.DoesBlueprintBindToViewModelEvent.BindStatic(&FMDViewModelGraphModule::DoesBlueprintBindToViewModelEvent);
	ViewModelEditorModule.OnViewModelEventRequestedForBlueprint.BindStatic(&FMDViewModelGraphModule::OnViewModelEventRequestedForBlueprint);
#endif
}

void FMDViewModelGraphModule::ShutdownModule()
{
#if WITH_EDITOR
    if (FMDViewModelEditorModule* ViewModelEditorModule = FModuleManager::GetModulePtr<FMDViewModelEditorModule>(TEXT("MDViewModelEditor")))
    {
    	ViewModelEditorModule->DoesBlueprintBindToViewModelEvent.Unbind();
    	ViewModelEditorModule->OnViewModelEventRequestedForBlueprint.Unbind();
    }
#endif
}

bool FMDViewModelGraphModule::DoesBlueprintBindToViewModelEvent(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass,
	const FName& ViewModelName)
{
	return FindExistingNode(BP, EventName, ViewModelClass, ViewModelName) != nullptr;
}

void FMDViewModelGraphModule::OnViewModelEventRequestedForBlueprint(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass,
	const FName& ViewModelName)
{
	if (BP == nullptr)
	{
		return;
	}

	const UMDVMNode_ViewModelEvent* Node = FindExistingNode(BP, EventName, ViewModelClass, ViewModelName);
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

UMDVMNode_ViewModelEvent* FMDViewModelGraphModule::FindExistingNode(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName)
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

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMDViewModelGraphModule, MDViewModelGraph)
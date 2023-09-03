#pragma once

#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"

struct FMDViewModelAssignmentData;
struct FMDViewModelAssignment;
class UMDVMNode_ViewModelChanged;
class UMDVMNode_ViewModelEvent;
class UMDVMNode_ViewModelFieldNotify;
class UMDViewModelBase;
class UBlueprint;


class MDVIEWMODELGRAPH_API FMDViewModelGraphStatics
{
public:
	static void GetViewModelAssignmentsForBlueprint(const UBlueprint* Blueprint, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments);
	static void SearchViewModelAssignmentsForBlueprint(const UBlueprint* Blueprint, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None);

	static bool DoesBlueprintBindToViewModelEvent(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	static void OnViewModelEventRequestedForBlueprint(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	static UMDVMNode_ViewModelEvent* FindExistingViewModelEventNode(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);

	static bool DoesBlueprintBindToViewModelFieldNotify(const UBlueprint* BP, const FName& FieldNotifyName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	static void OnViewModelFieldNotifyRequestedForBlueprint(const UBlueprint* BP, const FName& FieldNotifyName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	static UMDVMNode_ViewModelFieldNotify* FindExistingViewModelFieldNotifyNode(const UBlueprint* BP, const FName& FieldNotifyName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);

	static bool DoesBlueprintBindToViewModelChanged(const UBlueprint* BP, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	static void OnViewModelChangedRequestedForBlueprint(const UBlueprint* BP, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	static UMDVMNode_ViewModelChanged* FindExistingViewModelChangedNode(const UBlueprint* BP, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
};

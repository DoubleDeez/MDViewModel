#pragma once

#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"

struct FMDViewModelAssignmentReference;
struct FMDViewModelAssignmentData;
struct FMDViewModelAssignment;
class UBlueprintGeneratedClass;
class IMDViewModelAssignableInterface;
class UMDViewModelAssignmentComponent;
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
	static bool DoesBlueprintContainViewModelAssignments(const UBlueprint* Blueprint, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None);

	static bool DoesBlueprintBindToViewModelEvent(const UBlueprint* BP, const FName& EventName, const FMDViewModelAssignmentReference& Assignment);
	static void OnViewModelEventRequestedForBlueprint(const UBlueprint* BP, const FName& EventName, const FMDViewModelAssignmentReference& Assignment);
	static UMDVMNode_ViewModelEvent* FindExistingViewModelEventNode(const UBlueprint* BP, const FName& EventName, const FMDViewModelAssignmentReference& Assignment);

	static bool DoesBlueprintBindToViewModelFieldNotify(const UBlueprint* BP, const FName& FieldNotifyName, const FMDViewModelAssignmentReference& Assignment);
	static void OnViewModelFieldNotifyRequestedForBlueprint(const UBlueprint* BP, const FName& FieldNotifyName, const FMDViewModelAssignmentReference& Assignment);
	static UMDVMNode_ViewModelFieldNotify* FindExistingViewModelFieldNotifyNode(const UBlueprint* BP, const FName& FieldNotifyName, const FMDViewModelAssignmentReference& Assignment);

	static bool DoesBlueprintBindToViewModelChanged(const UBlueprint* BP, const FMDViewModelAssignmentReference& Assignment);
	static void OnViewModelChangedRequestedForBlueprint(const UBlueprint* BP, const FMDViewModelAssignmentReference& Assignment);
	static UMDVMNode_ViewModelChanged* FindExistingViewModelChangedNode(const UBlueprint* BP, const FMDViewModelAssignmentReference& Assignment);

	// Slow - checks all pins and all properties on all graphs on the BP and all dependent BPs for the references to the assignment
	static bool DoesBlueprintUseAssignment(UBlueprint* BP, const FMDViewModelAssignmentReference& Assignment);

	static IMDViewModelAssignableInterface* GetOrCreateAssignableInterface(UBlueprint* BP);
	static IMDViewModelAssignableInterface* GetAssignableInterface(const UBlueprint* BP);

	static UMDViewModelAssignmentComponent* GetOrCreateAssignmentComponentTemplate(UBlueprintGeneratedClass* BPClass);
};

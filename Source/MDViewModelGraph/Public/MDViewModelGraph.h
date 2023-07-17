#pragma once

#include "Modules/ModuleManager.h"
#include "Templates/SubclassOf.h"

struct FMDViewModelAssignmentData;
struct FMDViewModelAssignment;
class UWidgetBlueprint;
class UMDVMNode_ViewModelChanged;
class UMDVMNode_ViewModelEvent;
class UMDVMNode_ViewModelFieldNotify;
class UMDViewModelBase;
class UBlueprint;

class FMDViewModelGraphModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

	static void GetViewModelAssignmentsForWidgetBlueprint(const UWidgetBlueprint* WidgetBP, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments);

	MDVIEWMODELGRAPH_API static bool DoesBlueprintBindToViewModelEvent(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	MDVIEWMODELGRAPH_API static void OnViewModelEventRequestedForBlueprint(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	MDVIEWMODELGRAPH_API static UMDVMNode_ViewModelEvent* FindExistingViewModelEventNode(const UBlueprint* BP, const FName& EventName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);

	MDVIEWMODELGRAPH_API static bool DoesBlueprintBindToViewModelFieldNotify(const UBlueprint* BP, const FName& FieldNotifyName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	MDVIEWMODELGRAPH_API static void OnViewModelFieldNotifyRequestedForBlueprint(const UBlueprint* BP, const FName& FieldNotifyName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	MDVIEWMODELGRAPH_API static UMDVMNode_ViewModelFieldNotify* FindExistingViewModelFieldNotifyNode(const UBlueprint* BP, const FName& FieldNotifyName, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);

	MDVIEWMODELGRAPH_API static bool DoesBlueprintBindToViewModelChanged(const UBlueprint* BP, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	MDVIEWMODELGRAPH_API static void OnViewModelChangedRequestedForBlueprint(const UBlueprint* BP, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	MDVIEWMODELGRAPH_API static UMDVMNode_ViewModelChanged* FindExistingViewModelChangedNode(const UBlueprint* BP, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
};

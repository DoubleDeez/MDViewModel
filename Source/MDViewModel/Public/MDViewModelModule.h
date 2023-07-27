#pragma once

#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "Modules/ModuleManager.h"
#include "Templates/SubclassOf.h"
#include "Util/MDViewModelAssignment.h"
#include "Util/MDViewModelAssignmentData.h"

class UMDViewModelProviderBase;
class UGameInstance;
class UUserWidget;
class UMDViewModelBase;

class MDVIEWMODEL_API FMDViewModelModule : public IModuleInterface
{
public:
	// TODO - Move all this to a util class
	
	static void GetViewModelClassesForWidgetClass(TSubclassOf<UUserWidget> WidgetClass, TSet<TSubclassOf<UMDViewModelBase>>& OutViewModelClasses);
	static void GetViewModelAssignmentsForWidgetClass(TSubclassOf<UUserWidget> WidgetClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments);
	static void SearchViewModelAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UUserWidget> WidgetClass, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None);

	static bool DoesClassOrSuperClassHaveAssignments(TSubclassOf<UUserWidget> WidgetClass);
};

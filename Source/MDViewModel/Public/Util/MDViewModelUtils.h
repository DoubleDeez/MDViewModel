#pragma once

#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "UObject/NameTypes.h"

class IMDViewModelRuntimeInterface;
struct FGameplayTag;
struct FMDViewModelAssignmentData;
struct FMDViewModelAssignment;

class IMDVMCompiledAssignmentsInterface;
class UClass;
class UGameInstance;
class UMDViewModelBase;
class UMDViewModelProviderBase;

namespace MDViewModelUtils
{
	// The default name given to view models
	MDVIEWMODEL_API extern const FName DefaultViewModelName;

	MDVIEWMODEL_API UMDViewModelProviderBase* FindViewModelProvider(const FGameplayTag& ProviderTag);

	MDVIEWMODEL_API void GetViewModelAssignments(UClass* ObjectClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments);
	MDVIEWMODEL_API void SearchViewModelAssignments(UClass* ObjectClass, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None);
	MDVIEWMODEL_API bool HasViewModelAssignments(UClass* ObjectClass);

	MDVIEWMODEL_API IMDVMCompiledAssignmentsInterface* GetCompiledAssignmentsInterface(UClass* ObjectClass);
	MDVIEWMODEL_API IMDViewModelRuntimeInterface* GetViewModelRuntimeInterface(const UObject* Object);
	MDVIEWMODEL_API IMDViewModelRuntimeInterface* GetOrCreateViewModelRuntimeInterface(UObject* Object);

	MDVIEWMODEL_API const FProperty* GetFunctionReturnProperty(const UFunction* Function);
}

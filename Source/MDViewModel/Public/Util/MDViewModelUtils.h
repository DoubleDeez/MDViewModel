#pragma once

#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "UObject/NameTypes.h"

struct FGameplayTag;
struct FMDViewModelAssignmentData;
struct FMDViewModelAssignment;

class UClass;
class UGameInstance;
class UMDViewModelBase;
class UMDViewModelProviderBase;
class UUserWidget;

namespace MDViewModelUtils
{
	// The default name given to view models
	MDVIEWMODEL_API extern const FName DefaultViewModelName;

	MDVIEWMODEL_API UMDViewModelProviderBase* FindViewModelProvider(const FGameplayTag& ProviderTag);

	MDVIEWMODEL_API void GetViewModelAssignmentsForWidgetClass(TSubclassOf<UUserWidget> WidgetClass, bool bIncludeAncestorAssignments, TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments);
	MDVIEWMODEL_API void SearchViewModelAssignments(TMap<FMDViewModelAssignment, FMDViewModelAssignmentData>& OutViewModelAssignments, TSubclassOf<UUserWidget> WidgetClass, TSubclassOf<UMDViewModelBase> ViewModelClass = nullptr, const FGameplayTag& ProviderTag = FGameplayTag::EmptyTag, const FName& ViewModelName = NAME_None);

	MDVIEWMODEL_API bool DoesClassOrSuperClassHaveAssignments(TSubclassOf<UUserWidget> WidgetClass);
}

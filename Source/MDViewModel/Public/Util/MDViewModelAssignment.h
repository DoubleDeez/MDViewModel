#pragma once

#include "GameplayTagContainer.h"
#include "MDViewModelUtils.h"
#include "Templates/SubclassOf.h"
#include "ViewModelProviders/MDViewModelProvider_Manual.h"
#include "MDViewModelAssignment.generated.h"

class UMDViewModelBase;

/**
 * Data to track the assignments of view model classes
 */
USTRUCT()
struct MDVIEWMODEL_API FMDViewModelAssignment
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	UPROPERTY()
	FGameplayTag ProviderTag = TAG_MDVMProvider_Manual;

	UPROPERTY()
	FName ViewModelName = MDViewModelUtils::DefaultViewModelName;

#if WITH_EDITORONLY_DATA
	// Store class name so that this Assignment is still unique if ViewModelClass is renamed/deleted
	UPROPERTY()
	FName ViewModelClassName = NAME_None;

	void UpdateViewModelClassName();
#endif

	bool IsValid() const;

	bool operator==(const FMDViewModelAssignment& Other) const;

	bool operator!=(const FMDViewModelAssignment& Other) const
	{
		return !(*this == Other);
	}
};

inline uint32 GetTypeHash(const FMDViewModelAssignment& Assignment)
{
	return HashCombine(
		HashCombine(GetTypeHash(Assignment.ViewModelClass), GetTypeHash(Assignment.ProviderTag)),
		GetTypeHash(Assignment.ViewModelName));
}

MDVIEWMODEL_API FCbWriter& operator<<(FCbWriter& Writer, const FMDViewModelAssignment& Assignment);

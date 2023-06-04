#pragma once

#include "Templates/SubclassOf.h"
#include "MDViewModelInstanceKey.generated.h"

class UMDViewModelBase;

USTRUCT()
struct MDVIEWMODEL_API FMDViewModelInstanceKey
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "View Model")
	FName ViewModelName = NAME_None;

	UPROPERTY(VisibleAnywhere, Category = "View Model")
	TSubclassOf<UMDViewModelBase> ViewModelClass;

	bool IsValid() const;

	bool operator==(const FMDViewModelInstanceKey& Other) const;

	friend uint32 GetTypeHash(const FMDViewModelInstanceKey& Key)
	{
		return HashCombine(GetTypeHash(Key.ViewModelName), GetTypeHash(Key.ViewModelClass));
	}
};

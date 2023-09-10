#pragma once

#include "InstancedStruct.h"
#include "UObject/Interface.h"
#include "UObject/WeakInterfacePtr.h"
#include "Util/MDViewModelInstanceKey.h"
#include "MDViewModelCacheInterface.generated.h"

class UMDViewModelBase;

UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class UMDViewModelCacheInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for classes that act as a View Model Cache
 */
class MDVIEWMODEL_API IMDViewModelCacheInterface
{
	GENERATED_BODY()

public:
	UMDViewModelBase* GetOrCreateViewModel(const UObject* WorldContextObject, const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings = {});
	
	UMDViewModelBase* GetViewModel(const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass) const;

	bool IsShutdown() const { return bIsShutdown; }

	FSimpleMulticastDelegate OnShuttingDown;

protected:
	void BroadcastShutdown();

	virtual UObject* GetViewModelOwner() const = 0;

	virtual TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& GetViewModelCache() = 0;

	virtual FString GetCacheDebugName() const = 0;

	virtual const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& GetViewModelCache() const;

private:
	bool bIsShutdown = false;
};

FORCEINLINE uint32 GetTypeHash(const TWeakInterfacePtr<IMDViewModelCacheInterface>& WeakInterfacePtr)
{
	return WeakInterfacePtr.GetWeakObjectPtr().GetWeakPtrTypeHash();
}

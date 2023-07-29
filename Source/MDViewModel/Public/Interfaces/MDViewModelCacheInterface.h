#pragma once

#include "InstancedStruct.h"
#include "UObject/Interface.h"
#include "Util/MDViewModelInstanceKey.h"
#include "MDViewModelCacheInterface.generated.h"

class UMDViewModelBase;

// This class does not need to be modified.
UINTERFACE()
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
	UMDViewModelBase* GetOrCreateViewModel(const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings = {});
	
	UMDViewModelBase* GetViewModel(const FName& CachedViewModelKey, TSubclassOf<UMDViewModelBase> ViewModelClass) const;

	bool IsShutdown() const { return bIsShutdown; }

	using ViewModelCacheMap = TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>;
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnViewModelCacheShuttingDown, const ViewModelCacheMap&);
	FOnViewModelCacheShuttingDown OnViewModelCacheShuttingDown;

protected:
	void BroadcastShutdown();

	virtual UObject* GetViewModelOwner() const = 0;

	virtual TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& GetViewModelCache() = 0;

	virtual const TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>>& GetViewModelCache() const;

private:
	bool bIsShutdown = false;
};

#pragma once

#include "Runtime/Launch/Resources/Version.h"
#include "UObject/Interface.h"
#include "UObject/WeakInterfacePtr.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "MDViewModelRuntimeInterface.generated.h"

class APlayerController;
class UMDViewModelBase;
class UGameInstance;
class ULocalPlayer;
class UWorld;
struct FInstancedStruct;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FMDVMOnViewModelSetDynamic, UMDViewModelBase*, OldViewModel, UMDViewModelBase*, NewViewModel);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMDVMOnViewModelSet, UMDViewModelBase* /*OldViewModel*/, UMDViewModelBase* /*NewViewModel*/);

UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class UMDViewModelRuntimeInterface : public UInterface
{
	GENERATED_BODY()
};

// Interface for an object that can have view models set on it
class MDVIEWMODEL_API IMDViewModelRuntimeInterface
{
	GENERATED_BODY()

public:
	virtual UClass* GetOwningObjectClass() const;
	virtual UObject* GetOwningObject() const = 0;

	virtual UGameInstance* ResolveGameInstance() const = 0;
	virtual UWorld* ResolveWorld() const = 0;
	virtual ULocalPlayer* ResolveOwningLocalPlayer() const = 0;
	virtual APlayerController* ResolveOwningPlayer() const = 0;

	const TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>>& GetViewModels() const;

	UMDViewModelBase* SetViewModel(UMDViewModelBase* ViewModel, const FMDViewModelAssignmentReference& Assignment);
	UMDViewModelBase* SetViewModelOfClass(const UObject* WorldContextObject, UObject* ContextObject, const FMDViewModelAssignmentReference& Assignment, const FInstancedStruct& ViewModelSettings);
	UMDViewModelBase* GetViewModel(const FMDViewModelAssignmentReference& Assignment) const;
	void ClearViewModel(const FMDViewModelAssignmentReference& Assignment);

	FDelegateHandle ListenForAnyViewModelChanged(FSimpleDelegate&& Delegate);
	void StopListeningForAnyViewModelChanged(FDelegateHandle& Handle);
	void StopListeningForAnyViewModelChanged(const void* BoundObject);

	FDelegateHandle ListenForChanges(FMDVMOnViewModelSet::FDelegate&& Delegate, const FMDViewModelAssignmentReference& Assignment);
	void StopListeningForChanges(FDelegateHandle& Handle, const FMDViewModelAssignmentReference& Assignment);
	void StopListeningForAllNativeViewModelsChanged(const void* BoundObject);

	void ListenForChanges(FMDVMOnViewModelSetDynamic&& Delegate, const FMDViewModelAssignmentReference& Assignment);
	void StopListeningForChanges(const FMDVMOnViewModelSetDynamic& Delegate, const FMDViewModelAssignmentReference& Assignment);
	void StopListeningForAllDynamicViewModelsChanged(const UObject* BoundObject);

	bool IsListeningForChanges(const UObject* BoundObject, const FMDViewModelAssignmentReference& Assignment) const;

	bool CanManuallySetViewModelForAssignment(const FMDViewModelAssignmentReference& Assignment) const;

	TWeakInterfacePtr<IMDViewModelRuntimeInterface> MakeWeak() { return TWeakInterfacePtr<IMDViewModelRuntimeInterface>(this); }

	FSimpleMulticastDelegate OnBeginDestroy;

protected:
	virtual TMap<FMDViewModelAssignmentReference, TObjectPtr<UMDViewModelBase>>& GetViewModels() = 0;

	void PopulateViewModels();
	void CleanUpViewModels();

	void BroadcastViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, const FMDViewModelAssignmentReference& Assignment);

private:
	FSimpleMulticastDelegate OnAnyViewModelSetDelegates;
	TMap<FMDViewModelAssignmentReference, FMDVMOnViewModelSet> OnViewModelSetDelegates;
	TMap<FMDViewModelAssignmentReference, TArray<FMDVMOnViewModelSetDynamic>> OnViewModelSetDynamicDelegates;
};

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 1
FORCEINLINE uint32 GetTypeHash(const TWeakInterfacePtr<IMDViewModelRuntimeInterface>& WeakInterfacePtr)
{
	return GetTypeHash(WeakInterfacePtr.GetWeakObjectPtr());
}
#else
FORCEINLINE uint32 GetTypeHash(const TWeakInterfacePtr<IMDViewModelRuntimeInterface>& WeakInterfacePtr)
{
	return WeakInterfacePtr.GetWeakObjectPtr().GetWeakPtrTypeHash();
}
#endif

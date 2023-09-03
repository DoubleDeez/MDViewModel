#pragma once

#include "Extensions/UserWidgetExtension.h"
#include "Templates/SubclassOf.h"
#include "UObject/Package.h"
#include "Util/MDViewModelInstanceKey.h"
#include "Util/MDViewModelUtils.h"
#include "MDViewModelWidgetExtension.generated.h"

struct FGameplayTag;
struct FInstancedStruct;
class UMDViewModelBase;

DECLARE_DYNAMIC_DELEGATE_TwoParams(FMDVMOnViewModelSetDynamic, UMDViewModelBase*, OldViewModel, UMDViewModelBase*, NewViewModel);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMDVMOnViewModelSet, UMDViewModelBase* /*OldViewModel*/, UMDViewModelBase* /*NewViewModel*/);

/**
 * A widget extension to track a widget's view models
 */
UCLASS(BlueprintType)
class MDVIEWMODEL_API UMDViewModelWidgetExtension : public UUserWidgetExtension
{
	GENERATED_BODY()

#pragma region Core
public:
	virtual void Construct() override;
	virtual void Destruct() override;
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Get or Create View Model Extension", meta = (DefaultToSelf = "Widget"))
	static UMDViewModelWidgetExtension* GetOrCreate(UUserWidget* Widget);

	FSimpleMulticastDelegate OnBeginDestroy;

private:
	void PopulateViewModels();
	void CleanUpViewModels();

	UPROPERTY(Transient)
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> ViewModels;
#pragma endregion Core

#pragma region Assignment
public:
	template<typename T>
	T* SetViewModelOfClass(const UObject* WorldContextObject, UObject* ContextObject, const FInstancedStruct& ViewModelSettings, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), FName ViewModelName = MDViewModelUtils::DefaultViewModelName);

	UMDViewModelBase* SetViewModel(UMDViewModelBase* ViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	UMDViewModelBase* SetViewModelOfClass(const UObject* WorldContextObject, UObject* ContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);

	UMDViewModelBase* GetViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName) const;

	void ClearViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
#pragma endregion Assignment

#pragma region Delegates
public:
	FDelegateHandle ListenForChanges(FMDVMOnViewModelSet::FDelegate&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	void StopListeningForChanges(FDelegateHandle& Handle, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	void StopListeningForAllNativeViewModelsChanged(const void* BoundObject);

	void ListenForChanges(FMDVMOnViewModelSetDynamic&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	void StopListeningForChanges(const FMDVMOnViewModelSetDynamic& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	void StopListeningForAllDynamicViewModelsChanged(const UObject* BoundObject);

	bool IsListeningForChanges(const UObject* BoundObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName) const;

private:
	void BroadcastViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	TMap<FMDViewModelInstanceKey, FMDVMOnViewModelSet> OnViewModelSetDelegates;
	TMap<FMDViewModelInstanceKey, TArray<FMDVMOnViewModelSetDynamic>> OnViewModelSetDynamicDelegates;
#pragma endregion Delegates
};

template <typename T>
T* UMDViewModelWidgetExtension::SetViewModelOfClass(const UObject* WorldContextObject, UObject* ContextObject, const FInstancedStruct& ViewModelSettings, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(SetViewModelOfClass(WorldContextObject, ContextObject, ViewModelClass, ViewModelSettings, ViewModelName));
}

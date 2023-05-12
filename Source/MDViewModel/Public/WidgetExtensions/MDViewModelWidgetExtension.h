#pragma once

#include "CoreMinimal.h"
#include "Extensions/UserWidgetExtension.h"
#include "Templates/SubclassOf.h"
#include "Util/MDViewModelInstanceKey.h"
#include "Util/MDViewModelUtils.h"
#include "MDViewModelWidgetExtension.generated.h"

struct FGameplayTag;
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
	virtual void Initialize() override;
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Get or Create View Model Extension", meta = (DefaultToSelf = "Widget"))
	static UMDViewModelWidgetExtension* GetOrCreate(UUserWidget* Widget);

private:
	void PopulateViewModels();
	void CleanUpViewModels();

	UPROPERTY(Transient)
	TMap<FMDViewModelInstanceKey, TObjectPtr<UMDViewModelBase>> ViewModels;
#pragma endregion Core

#pragma region Assignment
public:
	template<typename T>
	T* SetViewModelOfClass(TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), FName ViewModelName = MDViewModelUtils::DefaultViewModelName);

	UMDViewModelBase* SetViewModel(UMDViewModelBase* ViewModel, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	UMDViewModelBase* SetViewModelOfClass(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName, UObject* Outer = GetTransientPackage());

	UMDViewModelBase* GetViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName) const;

protected:
	void OnProviderViewModelUpdated(TSubclassOf<UMDViewModelBase> ViewModelClass, FGameplayTag ProviderTag);
#pragma endregion Assignment

#pragma region Delegates
public:
	FDelegateHandle ListenForChanges(FMDVMOnViewModelSet::FDelegate&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	void StopListeningForChanges(FDelegateHandle& Handle, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	void StopListeningForAllNativeViewModelsChanged(const void* BoundObject);
	
	void ListenForChanges(FMDVMOnViewModelSetDynamic&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	void StopListeningForChanges(const FMDVMOnViewModelSetDynamic& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	void StopListeningForAllDynamicViewModelsChanged(const UObject* BoundObject);

private:
	void BroadcastViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, const FName& ViewModelName);
	TMap<FMDViewModelInstanceKey, FMDVMOnViewModelSet> OnViewModelSetDelegates;
	TMap<FMDViewModelInstanceKey, TArray<FMDVMOnViewModelSetDynamic>> OnViewModelSetDynamicDelegates;
#pragma endregion Delegates
};

template <typename T>
T* UMDViewModelWidgetExtension::SetViewModelOfClass(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(SetViewModelOfClass(ViewModelClass, ViewModelName));
}

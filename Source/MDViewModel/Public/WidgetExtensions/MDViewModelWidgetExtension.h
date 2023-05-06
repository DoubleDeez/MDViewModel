#pragma once

#include "CoreMinimal.h"
#include "Extensions/UserWidgetExtension.h"
#include "Templates/SubclassOf.h"
#include "Util/MDViewModelUtils.h"
#include "MDViewModelWidgetExtension.generated.h"

struct FGameplayTag;
class UMDViewModelBase;

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMDVMOnViewModelAssignedDynamic, UMDViewModelBase*, OldViewModel, UMDViewModelBase*, NewViewModel, FName, ViewModelName);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMDVMOnViewModelAssigned, UMDViewModelBase* /*OldViewModel*/, UMDViewModelBase* /*NewViewModel*/);

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
	TMap<FName, TObjectPtr<UMDViewModelBase>> ViewModels;
#pragma endregion Core

#pragma region Assignment
public:	
	template<typename T>
	T* AssignViewModelOfClass(TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	
	UMDViewModelBase* AssignViewModel(UMDViewModelBase* ViewModel, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	UMDViewModelBase* AssignViewModelOfClass(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	
	UMDViewModelBase* GetViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName) const;
	
protected:
	void OnProviderViewModelUpdated(TSubclassOf<UMDViewModelBase> ViewModelClass, FGameplayTag ProviderTag);
#pragma endregion Assignment

#pragma region Delegates
public:
	FDelegateHandle ListenForChanges(FMDVMOnViewModelAssigned::FDelegate&& Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	void StopListeningForChanges(FDelegateHandle& Handle, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = MDViewModelUtils::DefaultViewModelName);
	
private:
	void BroadcastViewModelChanged(UMDViewModelBase* OldViewModel, UMDViewModelBase* NewViewModel, const FName& ViewModelName);
	TMap<FName, FMDVMOnViewModelAssigned> OnViewModelAssignedDelegates;
#pragma endregion Delegates
};

template <typename T>
T* UMDViewModelWidgetExtension::AssignViewModelOfClass(TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(AssignViewModelOfClass(ViewModelClass, ViewModelName));
}

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "MDViewModelUtils.h"
#include "Templates/SubclassOf.h"
#include "WidgetExtensions/MDViewModelWidgetExtension.h"
#include "MDViewModelFunctionLibrary.generated.h"

class UMDViewModelBase;
class UUserWidget;

/**
 *
 */
UCLASS()
class MDVIEWMODEL_API UMDViewModelFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget"))
	static UMDViewModelBase* SetViewModel(UUserWidget* Widget, UMDViewModelBase* ViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget"))
	static void ClearViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeterminesOutputType="ViewModelClass"))
	static UMDViewModelBase* SetViewModelOfClass(UUserWidget* Widget, UObject* ContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings, FName ViewModelName = TEXT("Default"));

	template<typename T>
	static T* SetViewModelOfClass(UUserWidget* Widget, UObject* ContextObject, const FInstancedStruct& ViewModelSettings, FName ViewModelName = MDViewModelUtils::DefaultViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass());

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeterminesOutputType="ViewModelClass"))
	static UMDViewModelBase* GetViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget"))
	static bool DoesWidgetHaveViewModelClassAssigned(const UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, TSubclassOf<UMDViewModelBase>& OutAssignedViewModelClass, bool bIncludeChildClasses = true);

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget"))
	static void BindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget"))
	static void UnbindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget"))
	static void UnbindAllViewModelChangedEvent(UUserWidget* Widget);

	template<typename T>
	static T* GetViewModel(UUserWidget* Widget, FName ViewModelName = MDViewModelUtils::DefaultViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass());
};

template <typename T>
T* UMDViewModelFunctionLibrary::SetViewModelOfClass(UUserWidget* Widget, UObject* ContextObject, const FInstancedStruct& ViewModelSettings, FName ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(SetViewModelOfClass(Widget, ContextObject, ViewModelClass, ViewModelSettings, ViewModelName));
}

template <typename T>
T* UMDViewModelFunctionLibrary::GetViewModel(UUserWidget* Widget, FName ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(GetViewModel(Widget, ViewModelClass, ViewModelName));
}

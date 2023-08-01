#pragma once

#include "InstancedStruct.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MDViewModelUtils.h"
#include "Templates/SubclassOf.h"
#include "WidgetExtensions/MDViewModelWidgetExtension.h"
#include "MDViewModelFunctionLibrary.generated.h"

struct FMDViewModelAssignmentReference;
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
	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Set View Model node."))
	static UMDViewModelBase* SetViewModel(UUserWidget* Widget, UMDViewModelBase* ViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));
	// Set a view model on a widget
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Set View Model", meta = (DefaultToSelf = "Widget", AutoCreateRefTerm = "Assignment", DeterminesOutputType = "ViewModel"))
	static UMDViewModelBase* BP_SetViewModel(UPARAM(meta = (VMAssignment = "Assignment")) UUserWidget* Widget, UMDViewModelBase* ViewModel, const FMDViewModelAssignmentReference& Assignment);

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Clear View Model node."))
	static void ClearViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));
	// Clear a view model that's been set on a widget
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Clear View Model", meta = (DefaultToSelf = "Widget", AutoCreateRefTerm = "Assignment"))
	static void BP_ClearViewModel(UPARAM(meta = (VMAssignment = "Assignment")) UUserWidget* Widget, const FMDViewModelAssignmentReference& Assignment);

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeterminesOutputType="ViewModelClass", WorldContext="WorldContextObject", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Set View Model of Class node."))
	static UMDViewModelBase* SetViewModelOfClass(const UObject* WorldContextObject, UUserWidget* Widget, UObject* ContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings, FName ViewModelName = TEXT("Default"));
	// Create a view model and immediately set it on a widget
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Set View Model of Class", meta = (DefaultToSelf = "Widget", WorldContext="WorldContextObject", AutoCreateRefTerm = "Assignment,ViewModelSettings", BlueprintInternalUseOnly = "true"))
	static UMDViewModelBase* BP_SetViewModelOfClass(const UObject* WorldContextObject, UPARAM(meta = (VMAssignment = "Assignment")) UUserWidget* Widget, UObject* ContextObject, const FMDViewModelAssignmentReference& Assignment, const FInstancedStruct& ViewModelSettings);
	template<typename T>
	static T* SetViewModelOfClass(const UObject* WorldContextObject, UUserWidget* Widget, UObject* ContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), FName ViewModelName = MDViewModelUtils::DefaultViewModelName, const FInstancedStruct& ViewModelSettings = {});

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeterminesOutputType="ViewModelClass", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Get View Model node."))
	static UMDViewModelBase* GetViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));
	// Get a view model instance that's been assigned to a widget
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Get View Model", meta = (DefaultToSelf = "Widget", AutoCreateRefTerm = "Assignment", BlueprintInternalUseOnly = "true"))
	static UMDViewModelBase* BP_GetViewModel(UPARAM(meta = (VMAssignment = "Assignment")) UUserWidget* Widget, const FMDViewModelAssignmentReference& Assignment);

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (AutoCreateRefTerm = "ViewModelSettings", DeterminesOutputType="ViewModelClass", WorldContext="WorldContextObject"))
	static UMDViewModelBase* FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName CachedViewModelKey = TEXT("Default"), const FInstancedStruct& ViewModelSettings = FInstancedStruct());

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (AutoCreateRefTerm = "ViewModelSettings", DeterminesOutputType="ViewModelClass", WorldContext="WorldContextObject"))
	static UMDViewModelBase* FindCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName CachedViewModelKey = TEXT("Default"));

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget"))
	static bool DoesWidgetHaveViewModelClassAssigned(const UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, TSubclassOf<UMDViewModelBase>& OutAssignedViewModelClass, bool bIncludeChildClasses = true);

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Bind View Model Changed Event node."))
	static void BindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));
	// Bind to a specific view model changing
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Bind View Model Changed Event", meta = (DefaultToSelf = "Widget", AutoCreateRefTerm = "Assignment"))
	static void BP_BindViewModelChangedEvent(UPARAM(meta = (VMAssignment = "Assignment")) UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, const FMDViewModelAssignmentReference& Assignment);

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Unbind View Model Changed Event node."))
	static void UnbindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));
	// Unbind from the view model changed Event
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Unbind View Model Changed Event", meta = (DefaultToSelf = "Widget", AutoCreateRefTerm = "Assignment"))
	static void BP_UnbindViewModelChangedEvent(UPARAM(meta = (VMAssignment = "Assignment")) UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, const FMDViewModelAssignmentReference& Assignment);

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget"))
	static void UnbindAllViewModelChangedEvent(UUserWidget* Widget);

	template<typename T>
	static T* GetViewModel(UUserWidget* Widget, FName ViewModelName = MDViewModelUtils::DefaultViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass());
};

template <typename T>
T* UMDViewModelFunctionLibrary::SetViewModelOfClass(const UObject* WorldContextObject, UUserWidget* Widget, UObject* ContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName, const FInstancedStruct& ViewModelSettings)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(SetViewModelOfClass(WorldContextObject, Widget, ContextObject, ViewModelClass, ViewModelSettings, ViewModelName));
}

template <typename T>
T* UMDViewModelFunctionLibrary::GetViewModel(UUserWidget* Widget, FName ViewModelName, TSubclassOf<UMDViewModelBase> ViewModelClass)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(GetViewModel(Widget, ViewModelClass, ViewModelName));
}

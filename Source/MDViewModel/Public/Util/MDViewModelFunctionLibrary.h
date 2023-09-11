#pragma once

#include "InstancedStruct.h"
#include "Interfaces/MDViewModelRuntimeInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MDViewModelAssignmentReference.h"
#include "MDViewModelUtils.h"
#include "Templates/SubclassOf.h"
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
	// Set a view model on a widget
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Set View Model", meta = (DefaultToSelf = "Widget", AutoCreateRefTerm = "Assignment", DeterminesOutputType = "ViewModel"))
	static UMDViewModelBase* BP_SetViewModel(UPARAM(meta = (VMAssignment = "Assignment", DisplayName = "Object")) UObject* Widget, UPARAM(meta = (VMAssignment = "Assignment")) UMDViewModelBase* ViewModel, const FMDViewModelAssignmentReference& Assignment);

	// Clear a view model that's been set on a widget
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Clear View Model", meta = (DefaultToSelf = "Widget", AutoCreateRefTerm = "Assignment"))
	static void BP_ClearViewModel(UPARAM(meta = (VMAssignment = "Assignment", DisplayName = "Object")) UObject* Widget, const FMDViewModelAssignmentReference& Assignment);

	// Create a view model and immediately set it on a widget
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Set View Model of Class", meta = (DefaultToSelf = "Object", WorldContext="WorldContextObject", AutoCreateRefTerm = "Assignment,ViewModelSettings", BlueprintInternalUseOnly = "true"))
	static UMDViewModelBase* BP_SetViewModelOfClass(const UObject* WorldContextObject, UPARAM(meta = (VMAssignment = "Assignment")) UObject* Object, UObject* ContextObject, const FMDViewModelAssignmentReference& Assignment, const FInstancedStruct& ViewModelSettings);
	
	// Get a view model instance that's been assigned to a widget
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Get View Model", meta = (DefaultToSelf = "Object", ExpandBoolAsExecs = "IsValid", AutoCreateRefTerm = "Assignment", BlueprintInternalUseOnly = "true"))
	static UMDViewModelBase* BP_GetViewModel(UPARAM(meta = (VMAssignment = "Assignment")) UObject* Object, const FMDViewModelAssignmentReference& Assignment, bool& IsValid);

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (AutoCreateRefTerm = "ViewModelSettings", DeterminesOutputType="ViewModelClass", WorldContext="WorldContextObject"))
	static UMDViewModelBase* FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, const FInstancedStruct& ViewModelSettings, TSubclassOf<UMDViewModelBase> ViewModelClass, FName CachedViewModelKey = TEXT("Default"));
	
	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (AutoCreateRefTerm = "ViewModelSettings", DeterminesOutputType="ViewModelClass", WorldContext="WorldContextObject"))
	static UMDViewModelBase* FindCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName CachedViewModelKey = TEXT("Default"));
	
	// Bind to a specific view model changing
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Bind View Model Changed Event", meta = (DefaultToSelf = "Widget", AutoCreateRefTerm = "Assignment"))
	static void BP_BindViewModelChangedEvent(UPARAM(meta = (VMAssignment = "Assignment", DisplayName = "Object")) UObject* Widget, FMDVMOnViewModelSetDynamic Delegate, const FMDViewModelAssignmentReference& Assignment);

	// Unbind from the view model changed Event
	UFUNCTION(BlueprintCallable, Category = "View Model", DisplayName = "Unbind View Model Changed Event", meta = (DefaultToSelf = "Widget", AutoCreateRefTerm = "Assignment"))
	static void BP_UnbindViewModelChangedEvent(UPARAM(meta = (VMAssignment = "Assignment", DisplayName = "Object")) UObject* Widget, FMDVMOnViewModelSetDynamic Delegate, const FMDViewModelAssignmentReference& Assignment);

	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget"))
	static void UnbindAllViewModelChangedEvent(UPARAM(meta = (DisplayName = "Object")) UObject* Widget);

	template<typename T>
	static T* GetViewModel(UObject* Object, const FMDViewModelAssignmentReference& Assignment);
	
	template<typename T>
	static T* SetViewModelOfClass(const UObject* WorldContextObject, UObject* Object, UObject* ContextObject, const FMDViewModelAssignmentReference& Assignment, const FInstancedStruct& ViewModelSettings = {});

	template<typename T>
	static T* FindCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), FName CachedViewModelKey = MDViewModelUtils::DefaultViewModelName);

	template<typename T>
	static T* FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass = T::StaticClass(), FName CachedViewModelKey = MDViewModelUtils::DefaultViewModelName, const FInstancedStruct& ViewModelSettings = {});

#pragma region Deprecated
	UE_DEPRECATED(All, "SetViewModel is deprecated, please use BP_SetViewModel instead")
	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Set View Model node."))
	static UMDViewModelBase* SetViewModel(UUserWidget* Widget, UMDViewModelBase* ViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));

	UE_DEPRECATED(All, "ClearViewModel is deprecated, please use BP_ClearViewModel instead")
	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Clear View Model node."))
	static void ClearViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));

	UE_DEPRECATED(All, "SetViewModelOfClass is deprecated, please use BP_SetViewModelOfClass instead")
	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeterminesOutputType="ViewModelClass", WorldContext="WorldContextObject", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Set View Model of Class node."))
	static UMDViewModelBase* SetViewModelOfClass(const UObject* WorldContextObject, UUserWidget* Widget, UObject* ContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings, FName ViewModelName = TEXT("Default"));

	UE_DEPRECATED(All, "GetViewModel is deprecated, please use BP_GetViewModel instead")
	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeterminesOutputType="ViewModelClass", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Get View Model node."))
	static UMDViewModelBase* GetViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));

	UE_DEPRECATED(All, "BindViewModelChangedEvent is deprecated, please use BP_BindViewModelChangedEvent instead")
	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Bind View Model Changed Event node."))
	static void BindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));

	UE_DEPRECATED(All, "UnbindViewModelChangedEvent is deprecated, please use BP_UnbindViewModelChangedEvent instead")
	UFUNCTION(BlueprintCallable, Category = "View Model", meta = (DefaultToSelf = "Widget", DeprecatedFunction, DeprecationMessage = "This function is deprecated, replace it with the new Unbind View Model Changed Event node."))
	static void UnbindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName = TEXT("Default"));
#pragma endregion Deprecated
};

template <typename T>
T* UMDViewModelFunctionLibrary::GetViewModel(UObject* Object, const FMDViewModelAssignmentReference& Assignment)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	bool bIsValid = false;
	return Cast<T>(BP_GetViewModel(Object, Assignment, bIsValid));
}

template <typename T>
T* UMDViewModelFunctionLibrary::SetViewModelOfClass(const UObject* WorldContextObject, UObject* Object, UObject* ContextObject, const FMDViewModelAssignmentReference& Assignment, const FInstancedStruct& ViewModelSettings)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(BP_SetViewModelOfClass(WorldContextObject, Object, ContextObject, Assignment, ViewModelSettings));
}

template <typename T>
T* UMDViewModelFunctionLibrary::FindCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName CachedViewModelKey)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(FindCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey));
}

template <typename T>
T* UMDViewModelFunctionLibrary::FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName CachedViewModelKey, const FInstancedStruct& ViewModelSettings)
{
	static_assert(TIsDerivedFrom<T, UMDViewModelBase>::Value, "ViewModels must derive from UMDViewModelBase");
	return Cast<T>(FindOrCreateCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey, ViewModelSettings));
}

#include "Util/MDViewModelFunctionLibrary.h"

#include "Blueprint/UserWidget.h"
#include "UObject/Package.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Util/MDViewModelUtils.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProvider_Cached.h"

UMDViewModelBase* UMDViewModelFunctionLibrary::SetViewModel(UUserWidget* Widget, UMDViewModelBase* ViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = ViewModelClass;
	AssignmentReference.ViewModelName = ViewModelName;
	
	return BP_SetViewModel(Widget, ViewModel, AssignmentReference);
}

UMDViewModelBase* UMDViewModelFunctionLibrary::BP_SetViewModel(UObject* Object, UMDViewModelBase* ViewModel, const FMDViewModelAssignmentReference& Assignment)
{
	if (IMDViewModelRuntimeInterface* Interface = MDViewModelUtils::GetOrCreateViewModelRuntimeInterface(Object))
	{
		return Interface->SetViewModel(ViewModel, Assignment);
	}

	return nullptr;
}

void UMDViewModelFunctionLibrary::ClearViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = ViewModelClass;
	AssignmentReference.ViewModelName = ViewModelName;

	BP_ClearViewModel(Widget, AssignmentReference);
}

void UMDViewModelFunctionLibrary::BP_ClearViewModel(UObject* Object, const FMDViewModelAssignmentReference& Assignment)
{
	if (IMDViewModelRuntimeInterface* Interface = MDViewModelUtils::GetViewModelRuntimeInterface(Object))
	{
		Interface->ClearViewModel(Assignment);
	}
}

UMDViewModelBase* UMDViewModelFunctionLibrary::SetViewModelOfClass(const UObject* WorldContextObject, UUserWidget* Widget, UObject* ContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings, FName ViewModelName)
{
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = ViewModelClass;
	AssignmentReference.ViewModelName = ViewModelName;
	
	return BP_SetViewModelOfClass(WorldContextObject, Widget, ContextObject, AssignmentReference, ViewModelSettings);
}

UMDViewModelBase* UMDViewModelFunctionLibrary::BP_SetViewModelOfClass(const UObject* WorldContextObject, UObject* Object, UObject* ContextObject, const FMDViewModelAssignmentReference& Assignment, const FInstancedStruct& ViewModelSettings)
{
	if (IMDViewModelRuntimeInterface* Interface = MDViewModelUtils::GetOrCreateViewModelRuntimeInterface(Object))
	{
		return Interface->SetViewModelOfClass(WorldContextObject, ContextObject, Assignment, ViewModelSettings);
	}

	return nullptr;
}

UMDViewModelBase* UMDViewModelFunctionLibrary::GetViewModel(UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = ViewModelClass;
	AssignmentReference.ViewModelName = ViewModelName;

	bool IsValid = false;
	return BP_GetViewModel(Widget, AssignmentReference, IsValid);
}

UMDViewModelBase* UMDViewModelFunctionLibrary::BP_GetViewModel(UObject* Object, const FMDViewModelAssignmentReference& Assignment, bool& bIsValid)
{
	if (const IMDViewModelRuntimeInterface* Interface = MDViewModelUtils::GetViewModelRuntimeInterface(Object))
	{
		UMDViewModelBase* ViewModel = Interface->GetViewModel(Assignment);
		bIsValid = IsValid(ViewModel);
		return ViewModel;
	}
	
	bIsValid = false;
	return nullptr;
}

UMDViewModelBase* UMDViewModelFunctionLibrary::FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, const FInstancedStruct& ViewModelSettings, TSubclassOf<UMDViewModelBase> ViewModelClass, FName CachedViewModelKey)
{
	return UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey, ViewModelSettings);
}

UMDViewModelBase* UMDViewModelFunctionLibrary::FindCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName CachedViewModelKey)
{
	return UMDViewModelProvider_Cached::FindCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey);
}

void UMDViewModelFunctionLibrary::BindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = ViewModelClass;
	AssignmentReference.ViewModelName = ViewModelName;

	BP_BindViewModelChangedEvent(Widget, Delegate, AssignmentReference);
}

void UMDViewModelFunctionLibrary::BP_BindViewModelChangedEvent(UObject* Object, FMDVMOnViewModelSetDynamic Delegate, const FMDViewModelAssignmentReference& Assignment)
{
	if (IMDViewModelRuntimeInterface* Interface = MDViewModelUtils::GetOrCreateViewModelRuntimeInterface(Object))
	{
		Interface->ListenForChanges(MoveTemp(Delegate), Assignment);
	}
}

void UMDViewModelFunctionLibrary::UnbindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = ViewModelClass;
	AssignmentReference.ViewModelName = ViewModelName;

	BP_UnbindViewModelChangedEvent(Widget, Delegate, AssignmentReference);
}

void UMDViewModelFunctionLibrary::BP_UnbindViewModelChangedEvent(UObject* Object, FMDVMOnViewModelSetDynamic Delegate, const FMDViewModelAssignmentReference& Assignment)
{
	if (IMDViewModelRuntimeInterface* Interface = MDViewModelUtils::GetViewModelRuntimeInterface(Object))
	{
		Interface->StopListeningForChanges(MoveTemp(Delegate), Assignment);
	}
}

void UMDViewModelFunctionLibrary::UnbindAllViewModelChangedEvent(UObject* Object)
{
	if (IMDViewModelRuntimeInterface* Interface = MDViewModelUtils::GetViewModelRuntimeInterface(Object))
	{
		Interface->StopListeningForAllDynamicViewModelsChanged(Object);
	}
}

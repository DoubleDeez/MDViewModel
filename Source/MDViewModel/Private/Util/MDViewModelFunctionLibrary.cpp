#include "Util/MDViewModelFunctionLibrary.h"

#include "Util/MDViewModelUtils.h"
#include "Blueprint/UserWidget.h"
#include "UObject/Package.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "ViewModel/MDViewModelBase.h"
#include "ViewModelProviders/MDViewModelProvider_Cached.h"
#include "WidgetExtensions/MDViewModelWidgetExtension.h"

UMDViewModelBase* UMDViewModelFunctionLibrary::SetViewModel(UUserWidget* Widget, UMDViewModelBase* ViewModel, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = ViewModelClass;
	AssignmentReference.ViewModelName = ViewModelName;
	
	return BP_SetViewModel(Widget, ViewModel, AssignmentReference);
}

UMDViewModelBase* UMDViewModelFunctionLibrary::BP_SetViewModel(UUserWidget* Widget, UMDViewModelBase* ViewModel, const FMDViewModelAssignmentReference& Assignment)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		return Extension->SetViewModel(ViewModel, Assignment.ViewModelClass.Get(), Assignment.ViewModelName);
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

void UMDViewModelFunctionLibrary::BP_ClearViewModel(UUserWidget* Widget, const FMDViewModelAssignmentReference& Assignment)
{
	if (IsValid(Widget))
	{
		UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			Extension->ClearViewModel(Assignment.ViewModelClass.Get(), Assignment.ViewModelName);
		}
	}
}

UMDViewModelBase* UMDViewModelFunctionLibrary::SetViewModelOfClass(const UObject* WorldContextObject, UUserWidget* Widget, UObject* ContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, const FInstancedStruct& ViewModelSettings, FName ViewModelName)
{
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = ViewModelClass;
	AssignmentReference.ViewModelName = ViewModelName;
	
	return BP_SetViewModelOfClass(WorldContextObject, Widget, ContextObject, AssignmentReference, ViewModelSettings);
}

UMDViewModelBase* UMDViewModelFunctionLibrary::BP_SetViewModelOfClass(const UObject* WorldContextObject, UUserWidget* Widget, UObject* ContextObject, const FMDViewModelAssignmentReference& Assignment, const FInstancedStruct& ViewModelSettings)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		return Extension->SetViewModelOfClass(WorldContextObject, ContextObject, Assignment.ViewModelClass.Get(), ViewModelSettings, Assignment.ViewModelName);
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

UMDViewModelBase* UMDViewModelFunctionLibrary::BP_GetViewModel(UUserWidget* Widget, const FMDViewModelAssignmentReference& Assignment, bool& bIsValid)
{
	if (IsValid(Widget))
	{
		const UMDViewModelWidgetExtension* Extension = Widget->GetExtension<UMDViewModelWidgetExtension>();
		if (IsValid(Extension))
		{
			UMDViewModelBase* ViewModel = Extension->GetViewModel(Assignment.ViewModelClass.Get(), Assignment.ViewModelName);
			bIsValid = IsValid(ViewModel);
			return ViewModel;
		}
	}

	bIsValid = false;
	return nullptr;
}

UMDViewModelBase* UMDViewModelFunctionLibrary::FindOrCreateCachedViewModel(const UObject* WorldContextObject, UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName CachedViewModelKey, const FInstancedStruct& ViewModelSettings)
{
	return UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey, ViewModelSettings);
}

UMDViewModelBase* UMDViewModelFunctionLibrary::FindCachedViewModel(const UObject* WorldContextObject, const UObject* CacheContextObject, TSubclassOf<UMDViewModelBase> ViewModelClass, FName CachedViewModelKey)
{
	return UMDViewModelProvider_Cached::FindCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey);
}

bool UMDViewModelFunctionLibrary::DoesWidgetHaveViewModelClassAssigned(const UUserWidget* Widget, TSubclassOf<UMDViewModelBase> ViewModelClass, TSubclassOf<UMDViewModelBase>& OutAssignedViewModelClass, bool bIncludeChildClasses)
{
	if (IsValid(Widget))
	{
		TMap<FMDViewModelAssignment, FMDViewModelAssignmentData> Assignments;
		MDViewModelUtils::GetViewModelAssignmentsForWidgetClass(Widget->GetClass(), Assignments);

		for (const auto& Pair : Assignments)
		{
			TSubclassOf<UMDViewModelBase> AssignedClass = Pair.Key.ViewModelClass;
			if (AssignedClass == ViewModelClass)
			{
				OutAssignedViewModelClass = ViewModelClass;
				return true;
			}
			else if (bIncludeChildClasses && ViewModelClass->IsChildOf(AssignedClass))
			{
				OutAssignedViewModelClass = AssignedClass;
				return true;
			}
		}
	}

	OutAssignedViewModelClass = nullptr;
	return false;
}

void UMDViewModelFunctionLibrary::BindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = ViewModelClass;
	AssignmentReference.ViewModelName = ViewModelName;

	BP_BindViewModelChangedEvent(Widget, Delegate, AssignmentReference);
}

void UMDViewModelFunctionLibrary::BP_BindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, const FMDViewModelAssignmentReference& Assignment)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		Extension->ListenForChanges(MoveTemp(Delegate), Assignment.ViewModelClass.Get(), Assignment.ViewModelName);
	}
}

void UMDViewModelFunctionLibrary::UnbindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, TSubclassOf<UMDViewModelBase> ViewModelClass, FName ViewModelName)
{
	FMDViewModelAssignmentReference AssignmentReference;
	AssignmentReference.ViewModelClass = ViewModelClass;
	AssignmentReference.ViewModelName = ViewModelName;

	BP_UnbindViewModelChangedEvent(Widget, Delegate, AssignmentReference);
}

void UMDViewModelFunctionLibrary::BP_UnbindViewModelChangedEvent(UUserWidget* Widget, FMDVMOnViewModelSetDynamic Delegate, const FMDViewModelAssignmentReference& Assignment)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		Extension->StopListeningForChanges(Delegate, Assignment.ViewModelClass.Get(), Assignment.ViewModelName);
	}
}

void UMDViewModelFunctionLibrary::UnbindAllViewModelChangedEvent(UUserWidget* Widget)
{
	UMDViewModelWidgetExtension* Extension = UMDViewModelWidgetExtension::GetOrCreate(Widget);
	if (IsValid(Extension))
	{
		Extension->StopListeningForAllDynamicViewModelsChanged(Widget);
	}
}

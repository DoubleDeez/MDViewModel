#include "Util/MDViewModelFunctionLibrary.h"
#include "Runtime/Launch/Resources/Version.h"

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 4
#include "Blueprint/BlueprintExceptionInfo.h"
#endif
#include "Blueprint/UserWidget.h"
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
#include "Logging/StructuredLog.h"
#endif
#include "UObject/Package.h"
#include "Util/MDViewModelAssignmentData.h"
#include "Util/MDViewModelAssignmentReference.h"
#include "Util/MDViewModelLog.h"
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
		if (!Interface->CanManuallySetViewModelForAssignment(Assignment))
		{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
			UE_LOGFMT(LogMDViewModel, Error, "Manually setting a View Model assignment [{Assignment}] with a non-manual provider (Object [{ObjectName}])",
				("Assignment", Assignment),
				("ObjectName", GetPathNameSafe(Object)));
#else
			UE_LOG(LogMDViewModel, Error, TEXT("Manually setting View Model assignment [%s (%s)] with a non-manual provider (Object [%s])"),
				*GetNameSafe(Assignment.ViewModelClass.Get()),
				*Assignment.ViewModelName.ToString(),
				*GetPathNameSafe(Object));
#endif
		}

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

DEFINE_FUNCTION(UMDViewModelFunctionLibrary::execFindOrCreateCachedViewModel)
{
	P_GET_OBJECT(const UObject, WorldContextObject);
	P_GET_OBJECT(UObject, CacheContextObject);
	P_GET_STRUCT_REF(FInstancedStruct, ViewModelSettings);
	P_GET_OBJECT(UClass, ViewModelClass);
	P_GET_PROPERTY(FNameProperty, CachedViewModelKey);

	P_FINISH;

	if (CacheContextObject == nullptr)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			INVTEXT("A valid Context Object must be passed in to Find or Create Cached View Model.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		*StaticCast<UMDViewModelBase**>(RESULT_PARAM) = nullptr;
		return;
	}

	UMDViewModelBase* ViewModel = nullptr;
	P_NATIVE_BEGIN
	ViewModel = UMDViewModelProvider_Cached::FindOrCreateCachedViewModel(WorldContextObject, CacheContextObject, ViewModelClass, CachedViewModelKey, ViewModelSettings);
	P_NATIVE_END

	*StaticCast<UMDViewModelBase**>(RESULT_PARAM) = ViewModel;
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

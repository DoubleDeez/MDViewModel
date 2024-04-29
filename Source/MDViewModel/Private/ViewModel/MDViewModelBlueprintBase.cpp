#include "ViewModel/MDViewModelBlueprintBase.h"

#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"

void UMDViewModelBlueprintBase::BeginDestroy()
{
	OnBeginDestroy.Broadcast();

	Super::BeginDestroy();

	// Work around issue where ~FInstancedStruct would crash when calling DestroyStruct on partially destroyed UserDefinedStructs
	Assignments.Empty();
}

UWorld* UMDViewModelBlueprintBase::GetWorld() const
{
	const UObject* WorldContext = CDOWorldContextObjectPtr.Get();
	if (IsValid(GEngine) && IsValid(WorldContext))
	{
		return GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);;
	}

	return Super::GetWorld();
}

void UMDViewModelBlueprintBase::PostInitProperties()
{
	Super::PostInitProperties();

	bImplements_RedirectContextObject = GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UMDViewModelBlueprintBase, BP_RedirectContextObject));
	bImplements_CDORedirectCachedContextObject = GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UMDViewModelBlueprintBase, BP_CDORedirectCachedContextObject));
	bImplements_InitializeViewModel = GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UMDViewModelBlueprintBase, BP_InitializeViewModel));
	bImplements_ShutdownViewModel = GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UMDViewModelBlueprintBase, BP_ShutdownViewModel));
}

const UE::FieldNotification::IClassDescriptor& UMDViewModelBlueprintBase::GetFieldNotificationDescriptor() const
{
	static FFieldNotificationClassDescriptor Local;
	return Local;
}

void UMDViewModelBlueprintBase::FFieldNotificationClassDescriptor::ForEachField(const UClass* Class, TFunctionRef<bool(UE::FieldNotification::FFieldId FieldId)> Callback) const
{
	const UBlueprintGeneratedClass* BPGC = Cast<const UBlueprintGeneratedClass>(Class);
	if (IsValid(BPGC))
	{
#if MDVM_WITH_BLUEPRINT_FIELD_NOTIFY
		BPGC->ForEachFieldNotify(Callback, true);
#else
		// TODO - Support 5.1 and 5.2 FieldNotify properties in BP
#endif
	}
}

#if WITH_EDITOR && WITH_EDITORONLY_DATA
void UMDViewModelBlueprintBase::GetStoredContextObjectTypes(const FInstancedStruct& ViewModelSettings, UBlueprint* Blueprint, TArray<TSubclassOf<UObject>>& OutClasses) const
{
	if (bImplements_RedirectContextObject || bImplements_CDORedirectCachedContextObject)
	{
		OutClasses = StoredContextObjectTypes;
	}
	else
	{
		Super::GetStoredContextObjectTypes(ViewModelSettings, Blueprint, OutClasses);
	}
}

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
EDataValidationResult UMDViewModelBlueprintBase::IsDataValid(FDataValidationContext& Context) const
{
	const bool Implements_RedirectContextObject = GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UMDViewModelBlueprintBase, BP_RedirectContextObject));
	const bool Implements_CDORedirectCachedContextObject = GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UMDViewModelBlueprintBase, BP_CDORedirectCachedContextObject));

	EDataValidationResult Result = EDataValidationResult::Valid;
	if (SupportedContextObjectTypes.IsEmpty())
	{
		Context.AddWarning(INVTEXT("SupportedContextObjectTypes is empty. Specify at least 1 supported context object type or add a 'None' entry if a context object is not required."));
	}

	if (StoredContextObjectTypes.IsEmpty() && (Implements_RedirectContextObject || Implements_CDORedirectCachedContextObject))
	{
		Context.AddWarning(INVTEXT("StoredContextObjectTypes is empty and 'Redirect Context Object' and/or 'Redirect Cached Context Object' are implemented. Specify the Context Object type(s) that may be stored on this view model."));
	}

	return CombineDataValidationResults(Result, Super::IsDataValid(Context));
}
#else
EDataValidationResult UMDViewModelBlueprintBase::IsDataValid(TArray<FText>& ValidationErrors)
{
	const bool Implements_RedirectContextObject = GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UMDViewModelBlueprintBase, BP_RedirectContextObject));
	const bool Implements_CDORedirectCachedContextObject = GetClass()->IsFunctionImplementedInScript(GET_FUNCTION_NAME_CHECKED(UMDViewModelBlueprintBase, BP_CDORedirectCachedContextObject));

	EDataValidationResult Result = EDataValidationResult::Valid;

	if (SupportedContextObjectTypes.IsEmpty())
	{
		ValidationErrors.Add(INVTEXT("SupportedContextObjectTypes is empty. Specify at least 1 supported context object type or add a 'None' entry if a context object is not required."));
		Result = EDataValidationResult::Invalid;
	}

	if (StoredContextObjectTypes.IsEmpty() && (Implements_RedirectContextObject || Implements_CDORedirectCachedContextObject))
	{
		ValidationErrors.Add(INVTEXT("StoredContextObjectTypes is empty and 'Redirect Context Object' and/or 'Redirect Cached Context Object' are implemented. Specify the Context Object type(s) that may be stored on this view model."));
		Result = EDataValidationResult::Invalid;
	}

	return CombineDataValidationResults(Result, Super::IsDataValid(ValidationErrors));
}
#endif
#endif

UObject* UMDViewModelBlueprintBase::RedirectContextObject(UObject* ProvidedContextObject) const
{
	if (bImplements_RedirectContextObject)
	{
		return BP_RedirectContextObject(ProvidedContextObject);
	}

	return Super::RedirectContextObject(ProvidedContextObject);
}

UObject* UMDViewModelBlueprintBase::CDORedirectCachedContextObject(const UObject* WorldContextObject, UObject* ProvidedCacheContextObject, const FInstancedStruct& ViewModelSettings) const
{
	if (bImplements_CDORedirectCachedContextObject)
	{
		TGuardValue<TWeakObjectPtr<const UObject>> CDOWorldContextGuard(CDOWorldContextObjectPtr, WorldContextObject);
		return BP_CDORedirectCachedContextObject(ProvidedCacheContextObject, ViewModelSettings);
	}

	return Super::CDORedirectCachedContextObject(WorldContextObject, ProvidedCacheContextObject, ViewModelSettings);
}

void UMDViewModelBlueprintBase::InitializeViewModel()
{
	InitializeViewModelSupport();

	PopulateViewModels();

	Super::InitializeViewModel();

	if (bImplements_InitializeViewModel)
	{
		BP_InitializeViewModel();
	}
}

void UMDViewModelBlueprintBase::ShutdownViewModel()
{
	if (bImplements_ShutdownViewModel)
	{
		BP_ShutdownViewModel();
	}

	ShutdownSubViewModels(SubViewModels);
	SubViewModels.Reset();

	CleanUpViewModels();

	Super::ShutdownViewModel();
}

UMDViewModelBase* UMDViewModelBlueprintBase::CreateSubViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, UObject* InContextObject, const FInstancedStruct& ViewModelSettings) const
{
	UMDViewModelBase* SubViewModel = Super::CreateSubViewModel(ViewModelClass, InContextObject, ViewModelSettings);

	SubViewModels.Add(SubViewModel);

	return SubViewModel;
}

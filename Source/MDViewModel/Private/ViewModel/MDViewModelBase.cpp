#include "ViewModel/MDViewModelBase.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InstancedStruct.h"

#if WITH_EDITOR
bool GIsInDebugViewModelContext = false;
#endif

void UMDViewModelBase::BeginDestroy()
{
	if (!ensureAlwaysMsgf(!bIsInitialized, TEXT("View Model [%s] is being destroyed without having been shutdown."), *GetName()))
	{
		ShutdownViewModelFromProvider();
	}

	Super::BeginDestroy();
}

void UMDViewModelBase::InitializeViewModelWithContext(const FInstancedStruct& ViewModelSettings, UObject* InContextObject, const UObject* WorldContext)
{
	if (!ensureAlwaysMsgf(!bIsInitialized, TEXT("View Model [%s] is already initialized but InitializeViewModelWithContext is being called again."), *GetName()))
	{
		return;
	}

	WorldContextObjectPtr = WorldContext;
	CachedViewModelSettings = ViewModelSettings;
	ContextObject = RedirectContextObject(InContextObject);

	ensureAlwaysMsgf(IsValid(GetWorld()), TEXT("View Model [%s]'s WorldContext [%s] and ContextObject [%s] could not reach the world. Either the World Context Object or the Context Object are expected to have a valid GetWorld() result."), *GetName(), *GetNameSafe(WorldContextObjectPtr.Get()), *GetNameSafe(GetContextObject()));

	InitializeViewModel();

	bIsInitialized = true;
}

void UMDViewModelBase::InitializeViewModelWithContext(const FInstancedStruct& ViewModelSettings, const UObject* InContextObject, const UObject* WorldContext)
{
	// TODO - Determine a way to support both const and non-const context objects
	InitializeViewModelWithContext(ViewModelSettings, const_cast<UObject*>(InContextObject), WorldContext);
}

void UMDViewModelBase::InitializeViewModelWithContext(UObject* InContextObject, const UObject* WorldContext)
{
	InitializeViewModelWithContext({}, InContextObject, WorldContext);
}

void UMDViewModelBase::InitializeViewModelWithContext(const UObject* InContextObject, const UObject* WorldContext)
{
	InitializeViewModelWithContext({}, InContextObject, WorldContext);
}

void UMDViewModelBase::ShutdownViewModelFromProvider()
{
	ShutdownViewModel();

	OnViewModelShutDown.Broadcast();

	bIsInitialized = false;
}

UWorld* UMDViewModelBase::GetWorld() const
{
	// Try the world context object first
	if (IsValid(GEngine))
	{
		const UObject* WorldContext = WorldContextObjectPtr.Get();
		if (IsValid(WorldContext))
		{
			UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
			if (IsValid(World))
			{
				return World;
			}
		}
	}

	// Fallback to the context object
	const UObject* Context = GetContextObject<UObject>();
	if (IsValid(Context))
	{
		return Context->GetWorld();
	}

	return nullptr;
}

FDelegateHandle UMDViewModelBase::AddFieldValueChangedDelegate(UE::FieldNotification::FFieldId InFieldId, FFieldValueChangedDelegate InNewDelegate)
{
	FDelegateHandle Result;
	if (InFieldId.IsValid())
	{
		Result = FieldNotifyDelegates.Add(this, InFieldId, MoveTemp(InNewDelegate));
		if (Result.IsValid())
		{
			EnabledFieldNotifications.PadToNum(InFieldId.GetIndex() + 1, false);
			EnabledFieldNotifications[InFieldId.GetIndex()] = true;
		}
	}
	return Result;
}

bool UMDViewModelBase::RemoveFieldValueChangedDelegate(UE::FieldNotification::FFieldId InFieldId, FDelegateHandle InHandle)
{
	bool bResult = false;
	if (InFieldId.IsValid() && InHandle.IsValid() && EnabledFieldNotifications.IsValidIndex(InFieldId.GetIndex()) && EnabledFieldNotifications[InFieldId.GetIndex()])
	{
		const UE::FieldNotification::FFieldMulticastDelegate::FRemoveFromResult RemoveResult = FieldNotifyDelegates.RemoveFrom(this, InFieldId, InHandle);
		bResult = RemoveResult.bRemoved;
		EnabledFieldNotifications[InFieldId.GetIndex()] = RemoveResult.bHasOtherBoundDelegates;
	}
	return bResult;
}

int32 UMDViewModelBase::RemoveAllFieldValueChangedDelegates(const void* InUserObject)
{
	int32 bResult = 0;
	if (InUserObject)
	{
		const UE::FieldNotification::FFieldMulticastDelegate::FRemoveAllResult RemoveResult = FieldNotifyDelegates.RemoveAll(this, InUserObject);
		bResult = RemoveResult.RemoveCount;
		EnabledFieldNotifications = RemoveResult.HasFields;
	}
	return bResult;
}

int32 UMDViewModelBase::RemoveAllFieldValueChangedDelegates(UE::FieldNotification::FFieldId InFieldId, const void* InUserObject)
{
	int32 bResult = 0;
	if (InUserObject)
	{
		const UE::FieldNotification::FFieldMulticastDelegate::FRemoveAllResult RemoveResult = FieldNotifyDelegates.RemoveAll(this, InFieldId, InUserObject);
		bResult = RemoveResult.RemoveCount;
		EnabledFieldNotifications = RemoveResult.HasFields;
	}
	return bResult;
}

const UE::FieldNotification::IClassDescriptor& UMDViewModelBase::GetFieldNotificationDescriptor() const
{
	static FFieldNotificationClassDescriptor Local;
	return Local;
}

void UMDViewModelBase::K2_AddFieldValueChangedDelegate(FFieldNotificationId InFieldId, FFieldValueChangedDynamicDelegate InDelegate)
{
	if (InFieldId.IsValid())
	{
		const UE::FieldNotification::FFieldId FieldId = GetFieldNotificationDescriptor().GetField(GetClass(), InFieldId.FieldName);
		if (ensureMsgf(FieldId.IsValid(), TEXT("The field should be compiled correctly.")))
		{
			if (FieldNotifyDelegates.Add(this, FieldId, InDelegate).IsValid())
			{
				EnabledFieldNotifications.PadToNum(FieldId.GetIndex() + 1, false);
				EnabledFieldNotifications[FieldId.GetIndex()] = true;
			}
		}
	}
}

void UMDViewModelBase::K2_RemoveFieldValueChangedDelegate(FFieldNotificationId InFieldId, FFieldValueChangedDynamicDelegate InDelegate)
{
	if (InFieldId.IsValid())
	{
		const UE::FieldNotification::FFieldId FieldId = GetFieldNotificationDescriptor().GetField(GetClass(), InFieldId.FieldName);
		if (ensureMsgf(FieldId.IsValid(), TEXT("The field should be compiled correctly.")))
		{
			if (EnabledFieldNotifications.IsValidIndex(FieldId.GetIndex()) && EnabledFieldNotifications[FieldId.GetIndex()])
			{
				const UE::FieldNotification::FFieldMulticastDelegate::FRemoveFromResult RemoveResult = FieldNotifyDelegates.RemoveFrom(this, FieldId, InDelegate);
				EnabledFieldNotifications[FieldId.GetIndex()] = RemoveResult.bHasOtherBoundDelegates;
			}
		}
	}
}

UObject* UMDViewModelBase::GetContextObject() const
{
	return GetContextObject<UObject>();
}

UObject* UMDViewModelBase::GetContextObjectAsType(UClass* ContextType) const
{
	checkf(false, TEXT("Calling a CustomThunk 'GetContextObjectAsType'"));
	return nullptr;
}

DEFINE_FUNCTION(UMDViewModelBase::execGetContextObjectAsType)
{
	P_GET_OBJECT(UClass, ContextType);

	P_FINISH;

	if (ContextType == nullptr)
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			INVTEXT("A valid Context Type must be passed in to Get Context Object as Type.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		*StaticCast<UObject**>(RESULT_PARAM) = nullptr;
		return;
	}

	UObject* ContextObject = P_THIS->ContextObject.Get();

	if (!IsValid(ContextObject))
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			INVTEXT("The Context Object is Invalid.")
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		*StaticCast<UObject**>(RESULT_PARAM) = nullptr;
		return;
	}

	if (!ContextObject->IsA(ContextType))
	{
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			FText::Format(INVTEXT("The Context Object [{0}] is not of Type [{1}]"), FText::FromName(ContextObject->GetFName()), FText::FromName(ContextType->GetFName()))
		);
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		*StaticCast<UObject**>(RESULT_PARAM) = nullptr;
		return;
	}

	*StaticCast<UObject**>(RESULT_PARAM) = ContextObject;
}

UMDViewModelBase* UMDViewModelBase::CreateSubViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, UObject* InContextObject, const FInstancedStruct& ViewModelSettings, const UObject* WorldContextObject) const
{
	checkf(IsValid(ViewModelClass), TEXT("UMDViewModelBase::CreateSubViewModel required a valid ViewModelClass"));

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const FName NameBase = *FString::Printf(TEXT("SubVM_%s_From_%s"), *ViewModelClass->GetName(), *GetName());
	const FName VMObjectName = MakeUniqueObjectName(GetTransientPackage(), ViewModelClass, NameBase);
#else
	const FName VMObjectName = NAME_None;
#endif

	// Use the provided WorldContextObject if set, otherwise try the Context Object, then fallback to whatever this view model uses as its context object
	const bool bDoesContextHaveValidWorld = IsValid(InContextObject) && IsValid(GEngine) && IsValid(GEngine->GetWorldFromContextObject(InContextObject, EGetWorldErrorMode::ReturnNull));
	const UObject* WorldContext = IsValid(WorldContextObject) ? WorldContextObject : (bDoesContextHaveValidWorld ? InContextObject : GetEffectiveWorldContextObject());

	UMDViewModelBase* ViewModel = NewObject<UMDViewModelBase>(GetTransientPackage(), ViewModelClass, VMObjectName);
	ViewModel->InitializeViewModelWithContext(ViewModelSettings, InContextObject, WorldContext);
	return ViewModel;
}

UMDViewModelBase* UMDViewModelBase::CreateSubViewModel(TSubclassOf<UMDViewModelBase> ViewModelClass, const UObject* InContextObject, const FInstancedStruct& ViewModelSettings, const UObject* WorldContextObject) const
{
	return CreateSubViewModel(ViewModelClass, const_cast<UObject*>(InContextObject), ViewModelSettings, WorldContextObject);
}

void UMDViewModelBase::ShutdownSubViewModel(UMDViewModelBase*& ViewModel)
{
	if (IsValid(ViewModel))
	{
		ViewModel->ShutdownViewModelFromProvider();
	}

	ViewModel = nullptr;
}

void UMDViewModelBase::ShutdownSubViewModel(TWeakObjectPtr<UMDViewModelBase>& ViewModelPtr)
{
	if (UMDViewModelBase* ViewModel = ViewModelPtr.Get())
	{
		ViewModel->ShutdownViewModelFromProvider();
	}

	ViewModelPtr.Reset();
}

void UMDViewModelBase::BroadcastFieldValueChanged(UE::FieldNotification::FFieldId InFieldId)
{
	if (InFieldId.IsValid() && EnabledFieldNotifications.IsValidIndex(InFieldId.GetIndex()) && EnabledFieldNotifications[InFieldId.GetIndex()])
	{
		FieldNotifyDelegates.Broadcast(this, InFieldId);
	}
}

bool UMDViewModelBase::SetFieldNotifyValue(FText& Value, const FText& NewValue, UE::FieldNotification::FFieldId FieldId)
{
	if (Value.EqualTo(NewValue))
	{
		return false;
	}

	Value = NewValue;
	BroadcastFieldValueChanged(FieldId);
	return true;
}

void UMDViewModelBase::K2_BroadcastFieldValueChanged(FFieldNotificationId InFieldId)
{
	if (InFieldId.IsValid())
	{
		const UE::FieldNotification::FFieldId FieldId = GetFieldNotificationDescriptor().GetField(GetClass(), InFieldId.FieldName);
		if (ensureMsgf(FieldId.IsValid(), TEXT("The field should be compiled correctly.")))
		{
			BroadcastFieldValueChanged(FieldId);
		}
	}
}

const UObject* UMDViewModelBase::GetEffectiveWorldContextObject() const
{
	if (IsValid(GEngine))
	{
		const UObject* WorldContext = WorldContextObjectPtr.Get();
		if (IsValid(WorldContext))
		{
			UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
			if (IsValid(World))
			{
				return WorldContext;
			}
		}
	}

	// Fallback to the context object
	return GetContextObject<UObject>();
}
